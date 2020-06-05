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

#include <limits>
#include <string>
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
            insert_statement[":name"] = "Alice";
            insert_statement[":age"] = 20;
            insert_statement[":score"] = 12.3;
            REQUIRE_NOTHROW(insert_statement.execute());
            REQUIRE_NOTHROW(insert_statement.reset());
            insert_statement[":name"] = "Bob";
            insert_statement[":age"] = 25;
            insert_statement[":score"] = 11.5;
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

namespace
{
    template<typename T>
    void check_integer(sqlitemm::Connection& conn, T value)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (quantity) VALUES (:quantity)");
        insert_statement << value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT quantity FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(T(result[0]) == value);
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
        insert_statement << value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT price FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(T(result[0]) == Approx(value));
    }

    template<typename T>
    void check_integer(sqlitemm::Connection& conn, T value, const char* parameter)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (quantity) VALUES (:quantity)");
        insert_statement[parameter] = value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT quantity FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(T(result[0]) == value);
    }

    template<typename T>
    void check_signed(sqlitemm::Connection& conn, const char* parameter)
    {
        check_integer(conn, std::numeric_limits<T>::min(), parameter);
    }

    template<typename T>
    void check_unsigned(sqlitemm::Connection& conn, const char* parameter)
    {
        check_integer(conn, std::numeric_limits<T>::max(), parameter);
    }

    template<typename T>
    void check_float(sqlitemm::Connection& conn, T value, const char* parameter)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (price) VALUES (:price)");
        insert_statement[parameter] = value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT price FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(T(result[0]) == Approx(value));
    }

    template<typename T>
    void check_integer(sqlitemm::Connection& conn, T value, const std::string& parameter)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (quantity) VALUES (:quantity)");
        insert_statement[parameter] = value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT quantity FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(T(result[0]) == value);
    }

    template<typename T>
    void check_signed(sqlitemm::Connection& conn, const std::string& parameter)
    {
        check_integer(conn, std::numeric_limits<T>::min(), parameter);
    }

    template<typename T>
    void check_unsigned(sqlitemm::Connection& conn, const std::string& parameter)
    {
        check_integer(conn, std::numeric_limits<T>::max(), parameter);
    }

    template<typename T>
    void check_float(sqlitemm::Connection& conn, T value, const std::string& parameter)
    {
        auto insert_statement = conn.prepare("INSERT INTO item (price) VALUES (:price)");
        insert_statement[parameter] = value;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT price FROM item");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(T(result[0]) == Approx(value));
    }
}

TEST_CASE("Statement::operator<<")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

    SECTION("NULL")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (price) VALUES (:price)");
        insert_statement << nullptr;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT 1 FROM item WHERE price IS NULL");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(int(result[0]) == 1);
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

        SECTION("const char*")
        {
            const char* value = "Alice";
            insert_statement << value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }

        SECTION("std::string")
        {
            std::string value = "Alice";
            insert_statement << value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }

        SECTION("std::u16string")
        {
            std::u16string value = u"Alice";
            insert_statement << value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::u16string(result[0]) == value);
        }

        SECTION("TextValue")
        {
            std::string value = "Alice";
            insert_statement << sqlitemm::TextValue(value.c_str(), value.length(), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }
    }

    SECTION("BLOB")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (name) VALUES (:name)");

        SECTION("BlobValue")
        {
            std::array<int, 4> value{0, 1, 2, 3};
            insert_statement << sqlitemm::BlobValue(&value, sizeof(value), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            decltype(value) field_result;
            result.as_text([&field_result](auto value, int num_bytes) {
                REQUIRE(num_bytes == sizeof(field_result));
                auto p = reinterpret_cast<const char*>(value);
                std::copy(p, p + num_bytes, reinterpret_cast<char*>(&field_result));
            });
        }
    }
}

TEST_CASE("Statement::operator[](const char*)")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

    SECTION("NULL")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (price) VALUES (:price)");
        insert_statement[":price"] = nullptr;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT 1 FROM item WHERE price IS NULL");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(int(result[0]) == 1);
    }

    SECTION("INTEGER")
    {
        const char* parameter = ":quantity";

        SECTION("bool")
        {
            check_integer(conn, true, parameter);
        }

        SECTION("char")
        {
            check_integer(conn, 'A', parameter);
        }

        SECTION("signed char")
        {
            check_signed<signed char>(conn, parameter);
        }

        SECTION("unsigned char")
        {
            check_unsigned<unsigned char>(conn, parameter);
        }

        SECTION("short")
        {
            check_signed<short>(conn, parameter);
        }

        SECTION("unsigned short")
        {
            check_unsigned<unsigned short>(conn, parameter);
        }

        SECTION("int")
        {
            check_signed<int>(conn, parameter);
        }

        SECTION("unsigned int")
        {
            check_unsigned<unsigned int>(conn, parameter);
        }

        SECTION("long")
        {
            check_signed<long>(conn, parameter);
        }

        SECTION("unsigned long")
        {
            check_unsigned<unsigned long>(conn, parameter);
        }

        SECTION("long long")
        {
            check_signed<long long>(conn, parameter);
        }

        SECTION("unsigned long long")
        {
            check_unsigned<unsigned long long>(conn, parameter);
        }
    }

    SECTION("FLOAT")
    {
        const char* parameter = ":price";

        SECTION("float")
        {
            check_float(conn, 4.56f, parameter);
        }

        SECTION("double")
        {
            check_float(conn, 5.67, parameter);
        }
    }

    SECTION("TEXT")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (name) VALUES (:name)");

        SECTION("const char*")
        {
            const char* value = "Alice";
            insert_statement[":name"] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }

        SECTION("std::string")
        {
            std::string value = "Alice";
            insert_statement[":name"] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
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
            REQUIRE(std::u16string(result[0]) == value);
        }

        SECTION("TextValue")
        {
            std::string value = "Alice";
            insert_statement[":name"] = sqlitemm::TextValue(value.c_str(), value.length(), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }
    }

    SECTION("BLOB")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (name) VALUES (:name)");

        SECTION("BlobValue")
        {
            std::array<int, 4> value{0, 1, 2, 3};
            insert_statement[":name"] = sqlitemm::BlobValue(&value, sizeof(value), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            decltype(value) field_result;
            result.as_text([&field_result](auto value, int num_bytes) {
                REQUIRE(num_bytes == sizeof(field_result));
                auto p = reinterpret_cast<const char*>(value);
                std::copy(p, p + num_bytes, reinterpret_cast<char*>(&field_result));
            });
        }
    }
}

TEST_CASE("Statement::operator[](const std::string&)")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE item (id INTEGER PRIMARY KEY, name TEXT, quantity INTEGER, price REAL);"));

    SECTION("NULL")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (price) VALUES (:price)");
        insert_statement[std::string(":price")] = nullptr;
        insert_statement.execute();
        insert_statement.finalize();

        auto select_statement = conn.prepare("SELECT 1 FROM item WHERE price IS NULL");
        auto result = select_statement.execute_query();
        REQUIRE(result.step());
        REQUIRE(int(result[0]) == 1);
    }

    SECTION("INTEGER")
    {
        std::string parameter = ":quantity";

        SECTION("bool")
        {
            check_integer(conn, true, parameter);
        }

        SECTION("char")
        {
            check_integer(conn, 'A', parameter);
        }

        SECTION("signed char")
        {
            check_signed<signed char>(conn, parameter);
        }

        SECTION("unsigned char")
        {
            check_unsigned<unsigned char>(conn, parameter);
        }

        SECTION("short")
        {
            check_signed<short>(conn, parameter);
        }

        SECTION("unsigned short")
        {
            check_unsigned<unsigned short>(conn, parameter);
        }

        SECTION("int")
        {
            check_signed<int>(conn, parameter);
        }

        SECTION("unsigned int")
        {
            check_unsigned<unsigned int>(conn, parameter);
        }

        SECTION("long")
        {
            check_signed<long>(conn, parameter);
        }

        SECTION("unsigned long")
        {
            check_unsigned<unsigned long>(conn, parameter);
        }

        SECTION("long long")
        {
            check_signed<long long>(conn, parameter);
        }

        SECTION("unsigned long long")
        {
            check_unsigned<unsigned long long>(conn, parameter);
        }
    }

    SECTION("FLOAT")
    {
        std::string parameter = ":price";

        SECTION("float")
        {
            check_float(conn, 4.56f, parameter);
        }

        SECTION("double")
        {
            check_float(conn, 5.67, parameter);
        }
    }

    SECTION("TEXT")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (name) VALUES (:name)");
        std::string parameter = ":name";

        SECTION("const char*")
        {
            const char* value = "Alice";
            insert_statement[parameter] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }

        SECTION("std::string")
        {
            std::string value = "Alice";
            insert_statement[parameter] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }

        SECTION("std::u16string")
        {
            std::u16string value = u"Alice";
            insert_statement[parameter] = value;
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::u16string(result[0]) == value);
        }

        SECTION("TextValue")
        {
            std::string value = "Alice";
            insert_statement[parameter] = sqlitemm::TextValue(value.c_str(), value.length(), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            REQUIRE(std::string(result[0]) == value);
        }
    }

    SECTION("BLOB")
    {
        auto insert_statement = conn.prepare("INSERT INTO item (name) VALUES (:name)");
        std::string parameter = ":name";

        SECTION("BlobValue")
        {
            std::array<int, 4> value{0, 1, 2, 3};
            insert_statement[parameter] = sqlitemm::BlobValue(&value, sizeof(value), SQLITE_STATIC);
            insert_statement.execute();
            insert_statement.finalize();

            auto select_statement = conn.prepare("SELECT name FROM item");
            auto result = select_statement.execute_query();
            REQUIRE(result.step());
            decltype(value) field_result;
            result.as_text([&field_result](auto value, int num_bytes) {
                REQUIRE(num_bytes == sizeof(field_result));
                auto p = reinterpret_cast<const char*>(value);
                std::copy(p, p + num_bytes, reinterpret_cast<char*>(&field_result));
            });
        }
    }
}
