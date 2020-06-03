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

SCENARIO("results can be retrieved using result fields")
{
    GIVEN("a database connection with a table created")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

        WHEN("there is one row in the table")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO item (name, quantity, price) VALUES ('ball', 2, 1.23);"));

            THEN("the row can be retrieved using result fields")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                result.step();
                std::string name = result[0];
                REQUIRE(name == "ball");
                int quantity = result[1];
                REQUIRE(quantity == 2);
                double price = result[2];
                REQUIRE(price == Approx(1.23));
            }
        }

        WHEN("there is a row with null values and another without null values")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO item (name, quantity, price) VALUES "
                                         "(NULL, NULL, NULL), ('ball', 2, 1.23);"));

            THEN("the rows can be retrieved using result fields to Nullable")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                result.step();
                sqlitemm::Nullable<std::string> name = result[0];
                REQUIRE(name.null);
                sqlitemm::Nullable<int> quantity = result[1];
                REQUIRE(quantity.null);
                sqlitemm::Nullable<double> price = result[2];
                REQUIRE(quantity.null);
                result.step();
                name = result[0];
                REQUIRE(name.value == "ball");
                REQUIRE_FALSE(name.null);
                quantity = result[1];
                REQUIRE(quantity.value == 2);
                REQUIRE_FALSE(quantity.null);
                price = result[2];
                REQUIRE(price.value == Approx(1.23));
                REQUIRE_FALSE(quantity.null);
            }
        }
    }
}
