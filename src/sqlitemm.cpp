/************************************************************************************************************
 * SQLitemm source file
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
#include <algorithm>
#include <cassert>
#include <exception>
#include <memory>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>

namespace sqlitemm
{
    namespace
    {
        void throw_error(sqlite3* db, const char* message, int result_code)
        {
            if (db)
            {
                const char* errmsg = sqlite3_errmsg(db);
                if (errmsg)
                {
                    message = errmsg;
                }
            }

            if (!message)
            {
                message = "SQLite database error";
            }

            std::ostringstream ss;
            ss << message << " (" << result_code << ")";
            auto what_arg = ss.str();

            switch (result_code & 0xff)
            {
            default:
                throw Error(what_arg, result_code);
            }
        }

        void throw_error(sqlite3* db, int result_code)
        {
            throw_error(db, nullptr, result_code);
        }

        void throw_error(const char* message, int result_code)
        {
            throw_error(nullptr, message, result_code);
        }

        void throw_open_error(sqlite3* db, int result_code)
        {
            if (db)
            {
                std::string error_message{sqlite3_errmsg(db)};
                sqlite3_close(db);
                throw_error(error_message.c_str(), result_code);
            }
            else
            {
                throw_error("unable to allocate memory for SQLite database connection handle", result_code);
            }
        }
    }

    void Connection::open(const std::string& filename)
    {
        assert(!db);
        int result_code = sqlite3_open(filename.c_str(), &db);
        if (result_code != SQLITE_OK)
        {
            throw_open_error(db, result_code);
        }
    }

    void Connection::open(const std::u16string& filename)
    {
        assert(!db);
        int result_code = sqlite3_open16(filename.c_str(), &db);
        if (result_code != SQLITE_OK)
        {
            throw_open_error(db, result_code);
        }
    }

    void Connection::open(const std::string& filename, int flags, const std::string& vfs)
    {
        assert(!db);
        const char *vfs_name = vfs.empty() ? nullptr : vfs.c_str();
        int result_code = sqlite3_open_v2(filename.c_str(), &db, flags, vfs_name);
        if (result_code != SQLITE_OK)
        {
            throw_open_error(db, result_code);
        }
    }

    void Connection::close() noexcept
    {
        if (!db)
        {
            return;
        }

        int result_code = sqlite3_close(db);
        if (result_code != SQLITE_OK)
        {
            assert(result_code == SQLITE_BUSY);

            for (auto&& stmt_weak_ptr : stmt_ptrs)
            {
                auto stmt_ptr = stmt_weak_ptr.lock();
                if (stmt_ptr && *stmt_ptr)
                {
                    sqlite3_finalize(*stmt_ptr);
                }
            }
            stmt_ptrs.clear();

            sqlite3_close(db);
        }
        db = nullptr;
    }

    void Connection::execute(const std::string& sql)
    {
        assert(db && "database connection must exist");

        int result_code = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        if (result_code != SQLITE_OK)
        {
            throw_error(db, result_code);
        }
    }

    Statement Connection::prepare(const std::string& sql)
    {
        assert(db && "database connection must exist");

        sqlite3_stmt* stmt;
        int result_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (result_code != SQLITE_OK)
        {
            throw_error(db, result_code);
        }

        auto stmt_ptr = std::make_shared<sqlite3_stmt*>(stmt);
        clean_stmt_ptrs();
        stmt_ptrs.push_back(stmt_ptr);
        return Statement(stmt_ptr);
    }

    Transaction Connection::begin_transaction()
    {
        return Transaction(db);
    }

    void Connection::clean_stmt_ptrs()
    {
        stmt_ptrs.erase(std::remove_if(stmt_ptrs.begin(), stmt_ptrs.end(), [](auto&& p) { return p.expired(); }),
                        stmt_ptrs.end());
    }

    namespace
    {
        void bind_parameter(sqlite3_stmt* stmt, int index, std::nullptr_t)
        {
            int result_code = sqlite3_bind_null(stmt, index);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, int value)
        {
            int result_code = sqlite3_bind_int(stmt, index, value);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, long long value)
        {
            int result_code = sqlite3_bind_int64(stmt, index, value);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, double value)
        {
            int result_code = sqlite3_bind_double(stmt, index, value);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const char* value)
        {
            int result_code = sqlite3_bind_text(stmt, index, value, -1, SQLITE_STATIC);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const std::string& value)
        {
            int result_code = sqlite3_bind_text(stmt, index, &value[0], static_cast<int>(value.length()), SQLITE_STATIC);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, std::string&& value)
        {
            int result_code = sqlite3_bind_text(stmt, index, &value[0], static_cast<int>(value.length()), SQLITE_TRANSIENT);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const std::u16string& value)
        {
            int result_code = sqlite3_bind_text16(stmt, index, &value[0], static_cast<int>(value.length()) * 2, SQLITE_STATIC);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, std::u16string&& value)
        {
            int result_code = sqlite3_bind_text16(stmt, index, &value[0], static_cast<int>(value.length()) * 2, SQLITE_TRANSIENT);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const BlobValue& value)
        {
            auto num_bytes = static_cast<sqlite3_uint64>(value.num_bytes);
            int result_code = sqlite3_bind_blob64(stmt, index, value.content, num_bytes, value.destructor);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const TextValue& value)
        {
            int result_code = sqlite3_bind_text(stmt, index, value.content, value.num_bytes, value.destructor);
            if (result_code != SQLITE_OK)
            {
                throw_error(sqlite3_db_handle(stmt), result_code);
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, char value)
        {
            bind_parameter(stmt, index, static_cast<int>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, signed char value)
        {
            bind_parameter(stmt, index, static_cast<int>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, unsigned char value)
        {
            bind_parameter(stmt, index, static_cast<int>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, short value)
        {
            bind_parameter(stmt, index, static_cast<int>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, unsigned short value)
        {
            if (sizeof(unsigned short) < sizeof(int))
            {
                bind_parameter(stmt, index, static_cast<int>(value));
            }
            else
            {
                bind_parameter(stmt, index, static_cast<long long>(value));
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, unsigned int value)
        {
            static_assert(sizeof(unsigned int) < sizeof(long long));
            bind_parameter(stmt, index, static_cast<long long>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, long value)
        {
            if (sizeof(long) == sizeof(int))
            {
                bind_parameter(stmt, index, static_cast<int>(value));
            }
            else
            {
                bind_parameter(stmt, index, static_cast<long long>(value));
            }
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, float value)
        {
            bind_parameter(stmt, index, static_cast<double>(value));
        }
    }

    namespace detail
    {
        Parameter::Parameter(sqlite3_stmt* stmt, const char* name) : stmt(stmt)
        {
            assert(stmt && "prepared statement must not be a null pointer");
            assert(name && "bind parameter name must not be a null pointer");

            index = sqlite3_bind_parameter_index(stmt, name);
            if (index == 0)
            {
                std::ostringstream ss;
                ss << "invalid bind parameter name \"" << name << "\"";
                throw_error(ss.str().c_str(), SQLITE_RANGE);
            }
        }

        void Parameter::operator=(std::nullptr_t value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(char value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(signed char value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(unsigned char value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(short value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(unsigned short value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(int value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(unsigned int value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(long value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(long long value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(float value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(double value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(const std::string& value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(std::string&& value)
        {
            bind_parameter(stmt, index, std::move(value));
        }

        void Parameter::operator=(const std::u16string& value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(std::u16string&& value)
        {
            bind_parameter(stmt, index, std::move(value));
        }

        void Parameter::operator=(const char* value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(const sqlitemm::BlobValue& value)
        {
            bind_parameter(stmt, index, value);
        }

        void Parameter::operator=(const sqlitemm::TextValue& value)
        {
            bind_parameter(stmt, index, value);
        }
    }

    Statement& Statement::operator=(Statement&& other) noexcept
    {
        using std::swap;
        swap(stmt_ptr, other.stmt_ptr);
        swap(column_counter, other.column_counter);
        return *this;
    }

    Statement& Statement::operator<<(std::nullptr_t value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(char value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(signed char value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(unsigned char value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(short value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(unsigned short value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(int value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(unsigned int value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(long value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(long long value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(float value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(double value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(const std::string& value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(std::string&& value)
    {
        bind_parameter(*stmt_ptr, column_counter, std::move(value));
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(const std::u16string& value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(std::u16string&& value)
    {
        bind_parameter(*stmt_ptr, column_counter, std::move(value));
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(const char* value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(const BlobValue& value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    Statement& Statement::operator<<(const TextValue& value)
    {
        bind_parameter(*stmt_ptr, column_counter, value);
        ++column_counter;
        return *this;
    }

    bool Statement::finalize() noexcept
    {
        if (stmt_ptr && *stmt_ptr)
        {
            int result_code = sqlite3_finalize(*stmt_ptr);
            *stmt_ptr = nullptr;
            return result_code == SQLITE_OK;
        }
        return true;
    }

    void Statement::execute()
    {
        assert(stmt_ptr && *stmt_ptr);
        int result_code = sqlite3_step(*stmt_ptr);
        if (result_code != SQLITE_DONE && result_code != SQLITE_ROW)
        {
            throw_error(sqlite3_db_handle(*stmt_ptr), result_code);
        }
    }

    Result Statement::execute_query()
    {
        assert(stmt_ptr && *stmt_ptr);
        return Result(*stmt_ptr);
    }

    void Statement::reset()
    {
        assert(stmt_ptr && *stmt_ptr);
        int result_code = sqlite3_reset(*stmt_ptr);
        if (result_code != SQLITE_OK)
        {
            throw_error(sqlite3_db_handle(*stmt_ptr), result_code);
        }
        column_counter = 1;
    }

    Result& Result::operator>>(char& value)
    {
        assert(counter < column_count);
        value = static_cast<char>(sqlite3_column_int(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(signed char& value)
    {
        assert(counter < column_count);
        value = static_cast<signed char>(sqlite3_column_int(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(unsigned char& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned char>(sqlite3_column_int(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(short& value)
    {
        assert(counter < column_count);
        value = static_cast<short>(sqlite3_column_int(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(unsigned short& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned short>(sqlite3_column_int(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(int& value)
    {
        assert(counter < column_count);
        value = sqlite3_column_int(stmt, counter++);
        return *this;
    }

    Result& Result::operator>>(unsigned int& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned int>(sqlite3_column_int64(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(long& value)
    {
        assert(counter < column_count);
        value = static_cast<long>(sqlite3_column_int64(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(long long& value)
    {
        assert(counter < column_count);
        value = static_cast<long long>(sqlite3_column_int64(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(float& value)
    {
        assert(counter < column_count);
        value = static_cast<float>(sqlite3_column_double(stmt, counter++));
        return *this;
    }

    Result& Result::operator>>(double& value)
    {
        assert(counter < column_count);
        value = sqlite3_column_double(stmt, counter++);
        return *this;
    }

    Result& Result::operator>>(std::string& value)
    {
        assert(counter < column_count);
        const unsigned char* result = sqlite3_column_text(stmt, counter);
        value = std::string(result, result + sqlite3_column_bytes(stmt, counter));
        ++counter;
        return *this;
    }

    Result& Result::operator>>(std::u16string& value)
    {
        assert(counter < column_count);
        auto result = static_cast<const unsigned char*>(sqlite3_column_text16(stmt, counter));
        value = std::u16string(result, result + sqlite3_column_bytes16(stmt, counter));
        ++counter;
        return *this;
    }

    bool Result::step()
    {
        assert(stmt);

        int result_code = sqlite3_step(stmt);
        switch (result_code)
        {
        case SQLITE_ROW:
            counter = 0;
            if (column_count == 0)
            {
                column_count = sqlite3_column_count(stmt);
            }
            return true;
        case SQLITE_DONE:
            return false;
        default:
            throw_error(sqlite3_db_handle(stmt), result_code);
            return false;
        }
    }

    Transaction::Transaction(sqlite3* db) : db(db)
    {
        begin();
    }

    void Transaction::begin()
    {
        int result_code = sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
        if (result_code != SQLITE_OK)
        {
            throw_error(db, result_code);
        }
        committed = false;
    }

    void Transaction::commit()
    {
        int result_code = sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
        if (result_code != SQLITE_OK)
        {
            throw_error(db, result_code);
        }
        committed = true;
    }

    void Transaction::rollback() noexcept
    {
        if (!sqlite3_get_autocommit(db))
        {
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }
}
