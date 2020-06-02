/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Connection
 *
 * Copyright 2020 Amanda Wee
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

TEST_CASE("in-memory database can be opened and closed")
{
    SECTION("using an empty database connection")
    {
        sqlitemm::Connection conn;

        REQUIRE_NOTHROW(conn.open(":memory:"));
        REQUIRE_NOTHROW(conn.close());
    }

    SECTION("using a constructor")
    {
        REQUIRE_NOTHROW([]() {
            sqlitemm::Connection conn(":memory:");
        }());
    }
}

TEST_CASE("extended result codes are enabled")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"
                                 "INSERT INTO person (name) VALUES ('Alice');"));
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

TEST_CASE("execute")
{
    SECTION("using valid SQL")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("SELECT DATE('2001-01-01');"));
    }

    SECTION("using invalid SQL")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_THROWS(conn.execute("SELECT;"));
    }
}

TEST_CASE("last_insert_rowid")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"));

    SECTION("no successful inserts")
    {
        REQUIRE(conn.last_insert_rowid() == 0);
    }

    SECTION("one successful insert")
    {
        conn.execute("INSERT INTO person (name) VALUES ('Alice');");
        REQUIRE(conn.last_insert_rowid() == 1);
    }
}
