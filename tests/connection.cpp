/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Connection
 *
 * Copyright 2025 Amanda Wee
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and limitations under the License.
 ************************************************************************************************************/

#include "sqlitemm.hpp"
#include "catch.hpp"

SCENARIO("in-memory database can be opened and closed")
{
    WHEN("using an empty database connection")
    {
        sqlitemm::Connection conn;

        THEN("open and close can be called without exceptions thrown")
        {
            REQUIRE_NOTHROW(conn.open(":memory:"));
            REQUIRE_NOTHROW(conn.close());
        }
    }

    WHEN("using a constructor")
    {
        THEN("constructor can be invoked without exception thrown")
        {
            REQUIRE_NOTHROW([]() {
                sqlitemm::Connection conn(":memory:");
            }());
        }
    }
}

SCENARIO("extended result codes are enabled")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("a table with a unique row")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"
                                     "INSERT INTO person (name) VALUES ('Alice');"));

        WHEN("a duplicate row is inserted")
        {
            THEN("sqlitemm::Error is thrown with the extended result code")
            {
                try
                {
                    conn.execute("INSERT INTO person (name) VALUES ('Alice');");
                    REQUIRE(!"exception must be thrown");
                }
                catch (const sqlitemm::Error& e)
                {
                    REQUIRE(e.code() == SQLITE_CONSTRAINT_UNIQUE);
                }
            }
        }
    }
}

SCENARIO("changes member function is called")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("an empty table")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT);"));

        WHEN("there are no changes")
        {
            THEN("changes member function reports 0 changes")
            {
                REQUIRE(conn.changes() == 0);
            }
        }

        WHEN("one row is inserted")
        {
            conn.execute("INSERT INTO person (name) VALUES ('Alice');");

            THEN("1 change is reported")
            {
                REQUIRE(conn.changes() == 1);
            }
        }

        WHEN("three rows are inserted")
        {
            conn.execute("INSERT INTO person (name) VALUES ('Alice'), ('Bob'), ('Charlie');");

            THEN("3 changes are reported")
            {
                REQUIRE(conn.changes() == 3);

                WHEN("two rows are deleted")
                {
                    conn.execute("DELETE FROM person WHERE name IN ('Alice', 'Bob');");

                    THEN("two rows are reported")
                    {
                        REQUIRE(conn.changes() == 2);
                    }
                }
            }
        }
    }
}

SCENARIO("execute member function is called")
{
    sqlitemm::Connection conn(":memory:");

    WHEN("valid SQL is used")
    {
        REQUIRE_NOTHROW(conn.execute("SELECT DATE('2001-01-01');"));
    }

    WHEN("invalid SQL is used")
    {
        REQUIRE_THROWS(conn.execute("SELECT;"));
    }
}

SCENARIO("last_insert_rowid member function is called")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("an empty table")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"));

        WHEN("there have been no successful inserts")
        {
            THEN("the last insert rowid is reported to be 0")
            {
                REQUIRE(conn.last_insert_rowid() == 0);
            }
            
        }

        WHEN("there has been one successful insert")
        {
            conn.execute("INSERT INTO person (name) VALUES ('Alice');");

            THEN("thr last insert rowid is reported to be 1")
            {
                REQUIRE(conn.last_insert_rowid() == 1);
            }
        }
    }
}

extern "C"
{
    void sqlitemm_inc(sqlite3_context*, int, sqlite3_value**);
    void sqlitemm_sum_step(sqlite3_context*, int, sqlite3_value**);
    void sqlitemm_sum_final(sqlite3_context*);
}

SCENARIO("SQL functions can be created")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("a populated table")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE game_results (id INTEGER PRIMARY KEY, name TEXT, score INTEGER);"
                                     "INSERT INTO game_results (id, name, score) VALUES "
                                     "(1, 'Alice', 20), (2, 'Bob', 30);"));

        WHEN("SQL scalar function is created")
        {
            conn.create_scalar_function("sqlitemm_inc", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, sqlitemm_inc);

            THEN("results can be retrieved using the SQL scalar function")
            {
                auto stmt = conn.prepare("SELECT name, sqlitemm_inc(score) from game_results order by id;");
                auto result = stmt.execute_query();
                REQUIRE(result.step());
                REQUIRE(static_cast<std::string>(result[0]) == "Alice");
                REQUIRE(static_cast<int>(result[1]) == 21);
                REQUIRE(result.step());
                REQUIRE(static_cast<std::string>(result[0]) == "Bob");
                REQUIRE(static_cast<int>(result[1]) == 31);
            }
        }

        WHEN("SQL aggregate function is created")
        {
            conn.create_aggregate_function(
                "sqlitemm_sum", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, sqlitemm_sum_step, sqlitemm_sum_final
            );

            THEN("results can be retrieved using the SQL aggregate function")
            {
                auto stmt = conn.prepare("SELECT sqlitemm_sum(score) as total from game_results;");
                auto result = stmt.execute_query();
                REQUIRE(result.step());
                REQUIRE(static_cast<int>(result[0]) == 50);
            }
        }

        WHEN("SQL window function is created")
        {
            // Strictly speaking this creates a non-window aggregate function,
            // but we simplify for initial testing:
            conn.create_window_function(
                "sqlitemm_sum",
                1,
                SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                nullptr,
                sqlitemm_sum_step,
                sqlitemm_sum_final,
                nullptr,
                nullptr
            );

            THEN("results can be retrieved using the SQL window function")
            {
                auto stmt = conn.prepare("SELECT sqlitemm_sum(score) as total from game_results;");
                auto result = stmt.execute_query();
                REQUIRE(result.step());
                REQUIRE(static_cast<int>(result[0]) == 50);
            }
        }
    }
}

SCENARIO("attach and detach non-member functions are called")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("a table with one row")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"
                                     "INSERT INTO person (name) VALUES ('Alice');"));

        WHEN("attach is called")
        {
            attach(conn, ":memory:", "auxiliary");

            THEN("SQL statements can be run with both databases")
            {
                conn.execute("CREATE TABLE auxiliary.person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"
                             "INSERT INTO auxiliary.person (id, name) SELECT id, name FROM main.person;");
            }

            WHEN("detach is called")
            {
                detach(conn, "auxiliary");

                THEN("SQL statements cannot be run with the previously attached database")
                {
                    REQUIRE_THROWS_AS(
                        conn.execute("CREATE TABLE auxiliary.person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"),
                        sqlitemm::Error
                    );
                }
            }
        }
    }
}

SCENARIO("database configuration can be modified")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("a table with one row and a view of that table")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT);"
                                     "INSERT INTO person (name) VALUES ('Alice');"
                                     "CREATE VIEW person_view AS SELECT * FROM person;"));
        // Check that the view can be queried:
        REQUIRE_NOTHROW(conn.execute("SELECT COUNT(*) FROM person_view;"));

        WHEN("views are disabled")
        {
            conn.set_config(SQLITE_DBCONFIG_ENABLE_VIEW, 0, nullptr);

            THEN("the view cannot be queried")
            {
                REQUIRE_THROWS_AS(conn.execute("SELECT COUNT(*) FROM person_view;"), sqlitemm::Error);
            }
        }
    }
}

extern "C"
{
    int sqlitemm_reverse_nocase(void*, int, const void*, int, const void*);
}

SCENARIO("collations can be created")
{
    sqlitemm::Connection conn(":memory:");

    WHEN("a collation is created")
    {
        conn.create_collation("REVERSE_NOCASE", SQLITE_UTF8, nullptr, sqlitemm_reverse_nocase);

        THEN("a table can be created with a column having the new collation")
        {
            REQUIRE_NOTHROW(
                conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT COLLATE REVERSE_NOCASE);")
            );

            REQUIRE_NOTHROW(conn.execute("INSERT INTO person (id, name) VALUES (1, 'Alice'), (2, 'Bob');"));

            auto stmt = conn.prepare("SELECT name FROM person ORDER by name;");
            auto result = stmt.execute_query();
            REQUIRE(result.step());
            REQUIRE(static_cast<std::string>(result[0]) == "Bob");
            REQUIRE(result.step());
            REQUIRE(static_cast<std::string>(result[0]) == "Alice");
        }
    }
}

SCENARIO("most recent error code and message can be retrieved")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE(conn.get_last_error_code() == SQLITE_OK);
    REQUIRE(conn.get_last_error_message() == "not an error");

    WHEN("there is an error")
    {
        try
        {
            conn.execute("CREATE TABLE with a syntax error;");
            REQUIRE_FALSE("is not reached");
        }
        catch (const sqlitemm::Error& e)
        {
            REQUIRE(conn.get_last_error_code() == e.code());
            auto extended_errmsg = std::string{e.what()};
            REQUIRE(extended_errmsg.find(conn.get_last_error_message()) == 0);
        }
        catch (const std::exception& e)
        {
            REQUIRE_FALSE("other exception is not thrown");
        }
    }
}

SCENARIO("total number of changes can be counted")
{
    sqlitemm::Connection conn(":memory:");

    GIVEN("an empty table")
    {
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT);"));
        REQUIRE(conn.total_changes() == 0);

        WHEN("rows have been added")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO person (id, name) VALUES (1, 'Alice'), (2, 'Bob');"));

            THEN("those rows are in the count")
            {
                REQUIRE(conn.total_changes() == 2);

                WHEN("rows have been updated")
                {
                    REQUIRE_NOTHROW(conn.execute("UPDATE person SET name = 'Bobby' WHERE id = 2;"));

                    THEN("the total is updated for the count")
                    {
                        REQUIRE(conn.total_changes() == 3);
                    }
                }
            }
        }
    }
}

SCENARIO("interrupts can be started and checked")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_FALSE(conn.is_interrupted());
    // It is difficult to simulate an interrupt,
    // so we shall just run it to make sure it does not throw:
    REQUIRE_NOTHROW(conn.interrupt());
}

SCENARIO("SQLite extensions can be loaded")
{
    sqlitemm::Connection conn(":memory:");
    int load_extension_enabled = 0;
    conn.set_config(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, &load_extension_enabled);

    // It is difficult to simulate loading an extension,
    // so we will test failure:
    WHEN("an invalid extension is attempted to be loaded")
    {
        try
        {
            conn.load_extension("test_for_failure");
        }
        catch(const sqlitemm::Error& e)
        {
            std::string error_message = e.what();
            if (load_extension_enabled)
            {
                REQUIRE(error_message.find("(no such file)") != std::string::npos);
            }
            else
            {
                REQUIRE(error_message == "not authorized");
            }
        }
    }
}
