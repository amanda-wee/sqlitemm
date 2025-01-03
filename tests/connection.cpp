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
