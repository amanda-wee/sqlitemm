/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Statement
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

SCENARIO("prepared statement can bind parameters using stream operators")
{
    GIVEN("a database connection with a table created")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE result (id INTEGER PRIMARY KEY, name TEXT, games INTEGER, score REAL)"));

        WHEN("rows are inserted")
        {
            auto insert_statement = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
            REQUIRE_NOTHROW(insert_statement << "Alice" << 20 << 12.3);
            REQUIRE_NOTHROW(insert_statement.execute());
            REQUIRE_NOTHROW(insert_statement.reset());
            REQUIRE_NOTHROW(insert_statement << "Bob" << 25 << 11.5);
            REQUIRE_NOTHROW(insert_statement.execute());
            REQUIRE(insert_statement.finalize());

            THEN("the same rows can be retrieved")
            {
                auto select_statement = conn.prepare("SELECT name, games, score FROM result");
                auto result = select_statement.execute_query();
                std::string name;
                int games;
                double score;
                REQUIRE(result.step());
                REQUIRE_NOTHROW(result >> name >> games >> score);
                REQUIRE(name == "Alice");
                REQUIRE(games == 20);
                REQUIRE(score == Approx(12.3));
                REQUIRE(result.step());
                REQUIRE_NOTHROW(result >> name >> games >> score);
                REQUIRE(name == "Bob");
                REQUIRE(games == 25);
                REQUIRE(score == Approx(11.5));
                REQUIRE_FALSE(result.step());
            }
        }
    }
}

SCENARIO("prepared statement can bind parameters using indexed assignment")
{
    GIVEN("a database connection with a table created")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE result (id INTEGER PRIMARY KEY, name TEXT, games INTEGER, score REAL)"));

        WHEN("rows are inserted")
        {
            auto insert_statement = conn.prepare("INSERT INTO result (name, games, score) VALUES (:name, :age, :score)");
            auto transaction = conn.begin_transaction();
            insert_statement[":name"] = "Alice";
            insert_statement[":age"] = 20;
            insert_statement[":score"] = 12.3;
            REQUIRE_NOTHROW(insert_statement.execute());
            REQUIRE_NOTHROW(insert_statement.reset());
            insert_statement[":name"] = "Bob";
            insert_statement[":age"] = 25;
            insert_statement[":score"] = 11.5;
            REQUIRE_NOTHROW(insert_statement.execute());
            REQUIRE_NOTHROW(transaction.commit());
            REQUIRE(insert_statement.finalize());

            THEN("the same rows can be retrieved")
            {
                auto select_statement = conn.prepare("SELECT name, games, score FROM result");
                auto result = select_statement.execute_query();
                std::string name;
                int games;
                double score;
                REQUIRE(result.step());
                REQUIRE_NOTHROW(result >> name >> games >> score);
                REQUIRE(name == "Alice");
                REQUIRE(games == 20);
                REQUIRE(score == Approx(12.3));
                REQUIRE(result.step());
                REQUIRE_NOTHROW(result >> name >> games >> score);
                REQUIRE(name == "Bob");
                REQUIRE(games == 25);
                REQUIRE(score == Approx(11.5));
                REQUIRE_FALSE(result.step());
            }
        }
    }
}
