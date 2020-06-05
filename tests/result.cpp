/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Result and sqlitemm::ResultIterator
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

#include <string>
#include <vector>
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

        WHEN("there is a row with null values, another without null values, and another that is a mix")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO item (name, quantity, price) VALUES "
                                         "(NULL, NULL, NULL), ('ball', 2, 1.23), (NULL, 3, NULL);"));

            THEN("the rows can be retrieved using result fields to std::optional")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                result.step();
                auto name = result[0].to_optional<std::string>();
                REQUIRE_FALSE(name.has_value());
                auto quantity = result[1].to_optional<int>();
                REQUIRE_FALSE(quantity.has_value());
                auto price = result[2].to_optional<double>();
                REQUIRE_FALSE(quantity.has_value());
                result.step();
                name = result[0].to_optional<std::string>();
                REQUIRE(*name == "ball");
                REQUIRE(name.has_value());
                quantity = result[1].to_optional<int>();
                REQUIRE(*quantity == 2);
                REQUIRE(quantity.has_value());
                price = result[2].to_optional<double>();
                REQUIRE(*price == Approx(1.23));
                REQUIRE(price.has_value());
                result.step();
                name = result[0].to_optional<std::string>();
                REQUIRE_FALSE(name.has_value());
                quantity = result[1].to_optional<int>();
                REQUIRE(*quantity == 3);
                REQUIRE(quantity.has_value());
                price = result[2].to_optional<double>();
                REQUIRE_FALSE(price.has_value());
            }
        }
    }
}

SCENARIO("results can be retrieved using stream operators")
{
    GIVEN("a database connection with a table created")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

        WHEN("there is one row in the table")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO item (name, quantity, price) VALUES ('ball', 2, 1.23);"));

            THEN("the row can be retrieved using stream operators")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                result.step();
                std::string name;
                int quantity;
                double price;
                result >> name >> quantity >> price;
                REQUIRE(name == "ball");
                REQUIRE(quantity == 2);
                REQUIRE(price == Approx(1.23));
            }
        }

        WHEN("there is a row with null values, another without null values, and another that is a mix")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO item (name, quantity, price) VALUES "
                                         "(NULL, NULL, NULL), ('ball', 2, 1.23), (NULL, 3, NULL);"));

            THEN("the rows can be retrieved using result fields to std::optional")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                result.step();
                std::optional<std::string> name;
                std::optional<int> quantity;
                std::optional<double> price;
                result >> name >> quantity >> price;
                REQUIRE_FALSE(name.has_value());
                REQUIRE_FALSE(quantity.has_value());
                REQUIRE_FALSE(quantity.has_value());
                result.step();
                result >> name >> quantity >> price;
                REQUIRE(*name == "ball");
                REQUIRE(name.has_value());
                REQUIRE(*quantity == 2);
                REQUIRE(quantity.has_value());
                REQUIRE(*price == Approx(1.23));
                REQUIRE(price.has_value());
                result.step();
                result >> name >> quantity >> price;
                REQUIRE_FALSE(name.has_value());
                REQUIRE(*quantity == 3);
                REQUIRE(quantity.has_value());
                REQUIRE_FALSE(price.has_value());
            }
        }
    }
}

class Item
{
public:
    Item() = default;
    Item(const std::string& name, int quantity, double price) : name(name), quantity(quantity), price(price) {}
    Item(const sqlitemm::Result& result)
    {
        name = std::string(result[0]);
        quantity = result[1];
        price = result[2];
    }

    std::string name;
    int quantity;
    double price;
};

SCENARIO("results can be retrieved using result iterators")
{
    GIVEN("a database connection with a table created")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

        WHEN("there are two rows in the table")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO item (name, quantity, price) VALUES "
                                         "('ball', 2, 1.23), ('cup', 5, 2.05);"));

            THEN("the rows can be retrieved using result iterators")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                std::vector<Item> items(sqlitemm::ResultIterator<Item>(result), sqlitemm::ResultIterator<Item>{});
                REQUIRE(items.size() == 2);
                REQUIRE(items[0].name == "ball");
                REQUIRE(items[0].quantity == 2);
                REQUIRE(items[0].price == Approx(1.23));
                REQUIRE(items[1].name == "cup");
                REQUIRE(items[1].quantity == 5);
                REQUIRE(items[1].price == Approx(2.05));
            }

            THEN("the rows can be retrieved using result iterators obtained from begin() and end()")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                std::vector<Item> items(result.begin<Item>(), result.end<Item>());
                REQUIRE(items.size() == 2);
                REQUIRE(items[0].name == "ball");
                REQUIRE(items[0].quantity == 2);
                REQUIRE(items[0].price == Approx(1.23));
                REQUIRE(items[1].name == "cup");
                REQUIRE(items[1].quantity == 5);
                REQUIRE(items[1].price == Approx(2.05));
            }

            THEN("the rows can be retrieved using a range based for loop")
            {
                auto statement = conn.prepare("SELECT name, quantity, price FROM item;");
                auto result = statement.execute_query();
                std::vector<Item> items{Item("ball", 2, 1.23), Item("cup", 5, 2.05)};
                std::size_t i = 0;
                for (auto&& item : result.begin<Item>())
                {
                    REQUIRE(item.name == items[i].name);
                    REQUIRE(item.quantity == items[i].quantity);
                    REQUIRE(item.price == Approx(items[i].price));
                    ++i;
                }
                REQUIRE(i == 2);
            }
        }
    }
}
