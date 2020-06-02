/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Transaction
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

SCENARIO("transactions are used to group statements")
{
    GIVEN("a database connection with a table created")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE result (id INTEGER PRIMARY KEY, name TEXT, games INTEGER, score REAL)"));

        WHEN("insert statements are executed without incident")
        {
            {
                auto transaction = conn.begin_transaction();
                auto insert_statement = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
                insert_statement << "Alice" << 20 << 12.3;
                insert_statement.execute();
                insert_statement.reset();
                insert_statement << "Bob" << 25 << 11.5;
                insert_statement.execute();
                insert_statement.finalize();
                transaction.commit();
            }

            THEN("all of the data will be inserted")
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

        WHEN("an exception is thrown before the transaction can be committed")
        {
            try
            {
                auto transaction = conn.begin_transaction();
                auto insert_statement = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
                insert_statement << "Alice" << 20 << 12.3;
                insert_statement.execute();
                insert_statement.reset();
                throw std::runtime_error("test exception");
                insert_statement << "Bob" << 25 << 11.5;
                insert_statement.execute();
                insert_statement.finalize();
                transaction.commit();
            }
            catch (const std::exception& e)
            {
                // do nothing
            }

            THEN("none of the data will be inserted")
            {
                auto select_statement = conn.prepare("SELECT name, games, score FROM result");
                auto result = select_statement.execute_query();
                REQUIRE_FALSE(result.step());
            }
        }

        WHEN("a transaction is reused without incident")
        {
            {
                auto transaction = conn.begin_transaction();
                auto insert_statement1 = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
                insert_statement1 << "Alice" << 20 << 12.3;
                insert_statement1.execute();
                insert_statement1.reset();
                insert_statement1 << "Bob" << 25 << 11.5;
                insert_statement1.execute();
                insert_statement1.finalize();
                transaction.commit();

                transaction.begin();
                auto insert_statement2 = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
                insert_statement2 << "Charlie" << 30 << 10.4;
                insert_statement2.execute();
                insert_statement2.reset();
                insert_statement2 << "Trent" << 35 << 9.6;
                insert_statement2.execute();
                insert_statement2.finalize();
                transaction.commit();
            }

            THEN("only the data that was committed will be inserted")
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
                REQUIRE(result.step());
                REQUIRE_NOTHROW(result >> name >> games >> score);
                REQUIRE(name == "Charlie");
                REQUIRE(games == 30);
                REQUIRE(score == Approx(10.4));
                REQUIRE(result.step());
                REQUIRE_NOTHROW(result >> name >> games >> score);
                REQUIRE(name == "Trent");
                REQUIRE(games == 35);
                REQUIRE(score == Approx(9.6));
                REQUIRE_FALSE(result.step());
            }
        }

        WHEN("a transaction is reused then an exception is thrown before the transaction can be committed")
        {
            try
            {
                auto transaction = conn.begin_transaction();
                auto insert_statement1 = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
                insert_statement1 << "Alice" << 20 << 12.3;
                insert_statement1.execute();
                insert_statement1.reset();
                insert_statement1 << "Bob" << 25 << 11.5;
                insert_statement1.execute();
                insert_statement1.finalize();
                transaction.commit();

                transaction.begin();
                auto insert_statement2 = conn.prepare("INSERT INTO result (name, games, score) VALUES (?, ?, ?)");
                insert_statement2 << "Charlie" << 30 << 10.4;
                insert_statement2.execute();
                insert_statement2.reset();
                insert_statement2 << "Trent" << 35 << 9.6;
                insert_statement2.execute();
                insert_statement2.finalize();
                throw std::runtime_error("test exception");
                transaction.commit();
            }
            catch (const std::exception& e)
            {
                // do nothing
            }

            THEN("only the data that was committed will be inserted")
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
