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

#include <algorithm>
#include <array>
#include <optional>
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

namespace
{
    template<typename T>
    void check_integer(sqlitemm::Connection& conn, T value)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (quantity) VALUES (:quantity)");
        insert_statement[":quantity"] = value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT quantity FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        T field_result;
        result >> field_result;
        REQUIRE(field_result == value);
    }

    template<typename T>
    void check_signed(sqlitemm::Connection& conn)
    {
        check_integer(conn, std::numeric_limits<T>::min());
    }

    template<typename T>
    void check_unsigned(sqlitemm::Connection& conn)
    {
        check_integer(conn, std::numeric_limits<T>::max());
    }

    template<typename T>
    void check_float(sqlitemm::Connection& conn, T value)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (price) VALUES (:price)");
        insert_statement[":price"] = value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT price FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        T field_result;
        result >> field_result;
        REQUIRE(field_result == Approx(value));
    }
}

TEST_CASE("Result conversions")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

    SECTION("NULL")
    {
        conn.execute("INSERT INTO item (price) VALUES (NULL)");

        auto select_statement = conn.prepare("SELECT price FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());

        SECTION("optional")
        {
            std::optional<double> field_result;
            result >> field_result;
            REQUIRE_FALSE(field_result.has_value());
        }

        SECTION("without strict typing")
        {
            double field_result;
            result >> field_result;
            REQUIRE(field_result == Approx(0));
        }
    }

    SECTION("INTEGER")
    {
        SECTION("bool")
        {
            check_integer(conn, true);
        }

        SECTION("char")
        {
            check_integer(conn, 'A');
        }

        SECTION("signed char")
        {
            check_signed<signed char>(conn);
        }

        SECTION("unsigned char")
        {
            check_unsigned<unsigned char>(conn);
        }

        SECTION("short")
        {
            check_signed<short>(conn);
        }

        SECTION("unsigned short")
        {
            check_unsigned<unsigned short>(conn);
        }

        SECTION("int")
        {
            check_signed<int>(conn);
        }

        SECTION("unsigned int")
        {
            check_unsigned<unsigned int>(conn);
        }

        SECTION("long")
        {
            check_signed<long>(conn);
        }

        SECTION("unsigned long")
        {
            check_unsigned<unsigned long>(conn);
        }

        SECTION("long long")
        {
            check_signed<long long>(conn);
        }

        SECTION("unsigned long long")
        {
            check_unsigned<unsigned long long>(conn);
        }
    }

    SECTION("FLOAT")
    {
        std::string parameter = ":price";

        SECTION("float")
        {
            check_float(conn, 4.56f);
        }

        SECTION("double")
        {
            check_float(conn, 5.67);
        }
    }

    SECTION("TEXT")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (name) VALUES (:name)");

        SECTION("std::string")
        {
            std::string value = "Alice";
            insert_statement[":name"] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            std::string field_result;
            result >> field_result;
            REQUIRE(field_result == value);
        }

        SECTION("std::u16string")
        {
            std::u16string value = u"Alice";
            insert_statement[":name"] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            std::u16string field_result;
            result >> field_result;
            REQUIRE(field_result == value);
        }

        SECTION("as_text")
        {
            std::string value = "Alice";
            insert_statement[":name"] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            std::string field_result;
            result.as_text([&field_result](auto value, int num_bytes) {
                auto p = reinterpret_cast<const char*>(value);
                field_result = std::string(p, p + num_bytes);
            });
            REQUIRE(field_result == value);
        }

        SECTION("as_text16")
        {
            std::u16string value = u"Alice";
            insert_statement[":name"] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            std::u16string field_result;
            result.as_text16([&field_result](auto value, int num_bytes) {
                auto p = static_cast<const char16_t*>(value);
                field_result = std::u16string(p, p + num_bytes / sizeof(char16_t));
            });
            REQUIRE(field_result == value);
        }
    }

    SECTION("BLOB")
    {
        SECTION("as_blob")
        {
            auto insert_statement = conn.prepare("INSERT INTO item (quantity) VALUES (:quantity)");
            std::array<int, 4> value{0, 1, 2, 3};
            insert_statement[":quantity"] = sqlitemm::BlobValue(&value, sizeof(value), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT quantity FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            decltype(value) field_result;
            result.as_text([&field_result](auto value, int num_bytes) {
                REQUIRE(num_bytes == sizeof(field_result));
                auto p = reinterpret_cast<const char*>(value);
                std::copy(p, p + num_bytes, reinterpret_cast<char*>(&field_result));
            });
            REQUIRE(field_result == value);
        }
    }
}
