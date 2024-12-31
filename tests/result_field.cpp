/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::ResultField
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
#include <string>
#include "sqlitemm.hpp"
#include "catch.hpp"

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
        T field_result = result[0];
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
        T field_result = result[0];
        REQUIRE(field_result == Approx(value));
    }
}

TEST_CASE("ResultField conversions")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(
        conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);")
    );

    SECTION("NULL")
    {
        conn.execute("INSERT INTO item (price) VALUES (NULL)");

        auto select_statement = conn.prepare("SELECT price FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());

        SECTION("optional")
        {
            auto field_result = result[0].to_optional<double>();
            REQUIRE_FALSE(field_result.has_value());
        }

        SECTION("without strict typing")
        {
            double field_result = result[0];
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
            std::string field_result = result[0];
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
            std::u16string field_result = result[0];
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
            result[0].as_text([&field_result](auto value, int num_bytes) {
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
            result[0].as_text16([&field_result](auto value, int num_bytes) {
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
            result[0].as_text([&field_result](auto value, int num_bytes) {
                REQUIRE(num_bytes == sizeof(field_result));
                auto p = reinterpret_cast<const char*>(value);
                std::copy(p, p + num_bytes, reinterpret_cast<char*>(&field_result));
            });
            REQUIRE(field_result == value);
        }
    }
}
