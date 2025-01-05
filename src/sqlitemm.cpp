/************************************************************************************************************
 * SQLitemm source file
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
        void throw_error(const char* message, int result_code)
        {
            if (!message)
            {
                message = "SQLite database error";
            }

            std::ostringstream ss;
            ss << message << " (" << result_code << ")";
            auto what_arg = ss.str();

            switch (result_code & 0xff)
            {
            case SQLITE_BUSY:
                throw BusyError(what_arg, result_code);
            case SQLITE_CONSTRAINT:
                throw ConstraintError(what_arg, result_code);
            case SQLITE_LOCKED:
                throw LockedError(what_arg, result_code);
            default:
                throw Error(what_arg, result_code);
            }
        }

        void throw_error(sqlite3* db, int result_code)
        {
            throw_error(db ? sqlite3_errmsg(db) : nullptr, result_code);
        }

        void throw_error(sqlite3_stmt* stmt, int result_code)
        {
            throw_error(sqlite3_db_handle(stmt), result_code);
        }

        void check_open_ok(sqlite3* db, int result_code)
        {
            if (result_code != SQLITE_OK)
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

        void check_result_ok(sqlite3* db, int result_code)
        {
            if (result_code != SQLITE_OK)
            {
                throw_error(db, result_code);
            }
        }

        void check_result_ok(sqlite3_stmt* stmt, int result_code)
        {
            if (result_code != SQLITE_OK)
            {
                throw_error(stmt, result_code);
            }
        }

        std::string get_column_type_name(int column_type)
        {
            switch (column_type)
            {
            case SQLITE_NULL:
                return "NULL";
            case SQLITE_INTEGER:
                return "INTEGER";
            case SQLITE_FLOAT:
                return "FLOAT";
            case SQLITE_TEXT:
                return "TEXT";
            case SQLITE_BLOB:
                return "BLOB";
            default:
                return "UNKNOWN";
            }
        }

        void strict_type_check(bool strict_typing, int column_type, int expected_column_type)
        {
            if (strict_typing && column_type != expected_column_type)
            {
                std::ostringstream ss;
                ss << "expected result field to be of "
                   << get_column_type_name(expected_column_type)
                   << " type but the value was ";

                if (column_type == SQLITE_NULL)
                {
                    ss << "NULL";
                    throw NullTypeError(ss.str(), 0);
                }

                ss << "of " << get_column_type_name(column_type) << " type";
                throw TypeError(ss.str(), 0);
            }
        }
    }

    Transaction Connection::begin_transaction()
    {
        return Transaction(db);
    }

    size_t Connection::changes() const noexcept
    {
        return static_cast<size_t>(sqlite3_changes64(db));
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

    void Connection::create_aggregate_function(
        const std::string& function_name,
        int num_args,
        int text_encoding,
        void* app_user_data,
        void (*step_callback)(sqlite3_context*, int, sqlite3_value**),
        void (*final_callback)(sqlite3_context*),
        void (*destroy_callback)(void*)
    )
    {
        int result_code = sqlite3_create_function_v2(
            db,
            function_name.c_str(),
            num_args,
            text_encoding,
            app_user_data,
            nullptr,
            step_callback,
            final_callback,
            destroy_callback
        );
        check_result_ok(db, result_code);
    }

    void Connection::create_collation(
        const std::string& collation_name,
        int text_encoding,
        void* app_user_data,
        int (*compare_callback)(void*, int, const void*, int, const void*),
        void (*destroy_callback)(void*)
    )
    {
        int result_code = SQLITE_OK;
        if (destroy_callback)
        {
            result_code = sqlite3_create_collation_v2(
                db, collation_name.c_str(), text_encoding, app_user_data, compare_callback, destroy_callback
            );
        }
        else
        {
            result_code = sqlite3_create_collation(
                db, collation_name.c_str(), text_encoding, app_user_data, compare_callback
            );
        }
        check_result_ok(db, result_code);
    }

    void Connection::create_scalar_function(
        const std::string& function_name,
        int num_args,
        int text_encoding,
        void* app_user_data,
        void (*func_callback)(sqlite3_context*, int, sqlite3_value**),
        void (*destroy_callback)(void*)
    )
    {
        int result_code = sqlite3_create_function_v2(
            db,
            function_name.c_str(),
            num_args,
            text_encoding,
            app_user_data,
            func_callback,
            nullptr,
            nullptr,
            destroy_callback
        );
        check_result_ok(db, result_code);
    }

    void Connection::create_window_function(
        const std::string& function_name,
        int num_args,
        int text_encoding,
        void* app_user_data,
        void (*step_callback)(sqlite3_context*, int, sqlite3_value**),
        void (*final_callback)(sqlite3_context*),
        void (*value_callback)(sqlite3_context*),
        void (*inverse_callback)(sqlite3_context*, int, sqlite3_value**),
        void (*destroy_callback)(void*)
    )
    {
        int result_code = sqlite3_create_window_function(
            db,
            function_name.c_str(),
            num_args,
            text_encoding,
            app_user_data,
            step_callback,
            final_callback,
            value_callback,
            inverse_callback,
            destroy_callback
        );
        check_result_ok(db, result_code);
    }

    void Connection::execute(const std::string& sql)
    {
        assert(db && "database connection must exist");

        int result_code = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        check_result_ok(db, result_code);
    }

    int Connection::get_last_error_code() const noexcept
    {
        return sqlite3_extended_errcode(db);
    }

    std::string Connection::get_last_error_message() const
    {
        auto errmsg = sqlite3_errmsg(db);
        if (!errmsg)
        {
            return std::string{};
        }
        return std::string{errmsg};
    }

    long long Connection::last_insert_rowid() const noexcept
    {
        return static_cast<long long>(sqlite3_last_insert_rowid(db));
    }

    void Connection::open(const std::string& filename)
    {
        assert(!db);
        int result_code = sqlite3_open(filename.c_str(), &db);
        if (result_code != SQLITE_OK)
        {
            check_open_ok(db, result_code);
        }
        sqlite3_extended_result_codes(db, 1);
    }

    void Connection::open(const std::u16string& filename)
    {
        assert(!db);
        int result_code = sqlite3_open16(filename.c_str(), &db);
        check_open_ok(db, result_code);
        sqlite3_extended_result_codes(db, 1);
    }

    void Connection::open(const std::string& filename, int flags, const std::string& vfs)
    {
        assert(!db);
        const char *vfs_name = vfs.empty() ? nullptr : vfs.c_str();
        int result_code = sqlite3_open_v2(filename.c_str(), &db, flags, vfs_name);
        check_open_ok(db, result_code);
        sqlite3_extended_result_codes(db, 1);
    }

    Blob Connection::open_blob(
        const std::string& database, const std::string& table, const std::string& column, size_t row, int flags
    )
    {
        sqlite3_blob* blob = nullptr;
        int result_code = sqlite3_blob_open(db, database.c_str(), table.c_str(), column.c_str(), row, flags, &blob);
        check_result_ok(db, result_code);
        return Blob(db, blob);
    }

    Statement Connection::prepare(const std::string& sql)
    {
        assert(db && "database connection must exist");

        sqlite3_stmt* stmt;
        int result_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        check_result_ok(db, result_code);

        auto stmt_ptr = std::make_shared<sqlite3_stmt*>(stmt);
        clean_stmt_ptrs();
        stmt_ptrs.push_back(stmt_ptr);
        return Statement(stmt_ptr);
    }

    void Connection::set_busy_timeout(int ms) noexcept
    {
        sqlite3_busy_timeout(db, ms);
    }

    size_t Connection::total_changes() const noexcept
    {
        return static_cast<size_t>(sqlite3_total_changes64(db));
    }

    void Connection::clean_stmt_ptrs()
    {
        stmt_ptrs.erase(std::remove_if(stmt_ptrs.begin(), stmt_ptrs.end(), [](auto&& p) { return p.expired(); }),
                        stmt_ptrs.end());
    }

    void attach(Connection& connection, const std::string& filename, const std::string& schema_name)
    {
        std::ostringstream sql;
        sql << "ATTACH DATABASE :filename AS \"" << schema_name << "\";";
        auto stmt = connection.prepare(sql.str());
        stmt[":filename"] = filename;
        stmt.execute();
    }

    void detach(Connection& connection, const std::string& schema_name)
    {
        std::ostringstream sql;
        sql << "DETACH DATABASE \"" << schema_name << "\";";
        connection.execute(sql.str());
    }

    Backup::Backup(Connection& source, const std::string& source_database,
                   Connection& destination, const std::string& destination_database)
    {
        backup = sqlite3_backup_init(
            destination.db, destination_database.c_str(), source.db, source_database.c_str()
        );
    }

    bool Backup::step(int num_pages)
    {
        assert(backup);
        int result_code = sqlite3_backup_step(backup, num_pages);
        switch (result_code)
        {
        case SQLITE_OK:
            return true;
        case SQLITE_DONE:
            return false;
        default:
            throw_error("SQLite database backup error", result_code);
            return false;
        }
    }

    int Backup::pages_remaining() const noexcept
    {
        assert(backup);
        return sqlite3_backup_remaining(backup);
    }

    int Backup::page_count() const noexcept
    {
        assert(backup);
        return sqlite3_backup_pagecount(backup);
    }

    void Backup::close() noexcept
    {
        if (backup)
        {
            sqlite3_backup_finish(backup);
            backup = nullptr;
        }
    }

    void Blob::close() noexcept
    {
        sqlite3_blob_close(blob);
        blob = nullptr;
    }

    void Blob::read(void* buffer, size_t num_bytes, size_t blob_offset)
    {
        assert(blob);
        int result_code = sqlite3_blob_read(blob, buffer, num_bytes, blob_offset);
        check_result_ok(db, result_code);
    }

    void Blob::write(const void* buffer, size_t num_bytes, size_t blob_offset)
    {
        assert(blob);
        int result_code = sqlite3_blob_write(blob, buffer, num_bytes, blob_offset);
        check_result_ok(db, result_code);
    }

    size_t Blob::size() const
    {
        assert(blob);
        return sqlite3_blob_bytes(blob);
    }

    void Blob::reopen(size_t row)
    {
        assert(blob);
        int result_code = sqlite3_blob_reopen(blob, row);
        check_result_ok(db, result_code);
    }

    namespace
    {
        void bind_parameter(sqlite3_stmt* stmt, int index, std::nullptr_t)
        {
            int result_code = sqlite3_bind_null(stmt, index);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, int value)
        {
            int result_code = sqlite3_bind_int(stmt, index, value);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, long long value)
        {
            int result_code = sqlite3_bind_int64(stmt, index, value);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, bool value)
        {
            bind_parameter(stmt, index, (value ? 1 : 0));
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
            bind_parameter(stmt, index, static_cast<int>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, unsigned int value)
        {
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

        void bind_parameter(sqlite3_stmt* stmt, int index, unsigned long value)
        {
            bind_parameter(stmt, index, static_cast<long long>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, unsigned long long value)
        {
            bind_parameter(stmt, index, static_cast<long long>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, double value)
        {
            int result_code = sqlite3_bind_double(stmt, index, value);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, float value)
        {
            bind_parameter(stmt, index, static_cast<double>(value));
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const char* value)
        {
            int result_code = sqlite3_bind_text(stmt, index, value, -1, SQLITE_STATIC);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const std::string& value)
        {
            int result_code = sqlite3_bind_text(
                stmt, index, &value[0], static_cast<int>(value.length()), SQLITE_STATIC
            );
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, std::string&& value)
        {
            int result_code = sqlite3_bind_text(
                stmt, index, &value[0], static_cast<int>(value.length()), SQLITE_TRANSIENT
            );
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const std::u16string& value)
        {
            int result_code = sqlite3_bind_text16(stmt,
                                                  index,
                                                  &value[0],
                                                  static_cast<int>(value.length() * sizeof(value[0])),
                                                  SQLITE_STATIC);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, std::u16string&& value)
        {
            int result_code = sqlite3_bind_text16(stmt,
                                                  index,
                                                  &value[0],
                                                  static_cast<int>(value.length() * sizeof(value[0])),
                                                  SQLITE_TRANSIENT);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const TextValue& value)
        {
            int result_code = sqlite3_bind_text(stmt, index, value.content, value.num_bytes, value.destructor);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const BlobValue& value)
        {
            auto num_bytes = static_cast<sqlite3_uint64>(value.num_bytes);
            int result_code = sqlite3_bind_blob64(stmt, index, value.content, num_bytes, value.destructor);
            check_result_ok(stmt, result_code);
        }

        void bind_parameter(sqlite3_stmt* stmt, int index, const ZeroBlob& value)
        {
            auto num_bytes = static_cast<sqlite3_uint64>(value.num_bytes);
            int result_code = sqlite3_bind_zeroblob64(stmt, index, num_bytes);
            check_result_ok(stmt, result_code);
        }
    }

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

    void Parameter::operator=(bool value)
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

    void Parameter::operator=(unsigned long value)
    {
        bind_parameter(stmt, index, value);
    }

    void Parameter::operator=(long long value)
    {
        bind_parameter(stmt, index, value);
    }

    void Parameter::operator=(unsigned long long value)
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

    void Parameter::operator=(const BlobValue& value)
    {
        bind_parameter(stmt, index, value);
    }

    void Parameter::operator=(const TextValue& value)
    {
        bind_parameter(stmt, index, value);
    }

    void Parameter::operator=(const ZeroBlob& value)
    {
        bind_parameter(stmt, index, value);
    }

    Statement& Statement::operator=(Statement&& other) noexcept
    {
        using std::swap;
        swap(stmt_ptr, other.stmt_ptr);
        swap(parameter_index, other.parameter_index);
        return *this;
    }

    Statement& Statement::operator<<(std::nullptr_t value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(bool value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(char value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(signed char value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(unsigned char value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(short value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(unsigned short value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(int value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(unsigned int value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(long value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(unsigned long value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(long long value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(unsigned long long value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(float value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(double value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(const std::string& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(std::string&& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, std::move(value));
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(const std::u16string& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(std::u16string&& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, std::move(value));
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(const char* value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(const BlobValue& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(const TextValue& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
        return *this;
    }

    Statement& Statement::operator<<(const ZeroBlob& value)
    {
        bind_parameter(*stmt_ptr, parameter_index, value);
        ++parameter_index;
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
            throw_error(*stmt_ptr, result_code);
        }
    }

    Result Statement::execute_query(bool strict_typing)
    {
        assert(stmt_ptr && *stmt_ptr);
        return Result(*stmt_ptr, strict_typing);
    }

    void Statement::reset(bool clear_bindings)
    {
        assert(stmt_ptr && *stmt_ptr);
        int result_code = sqlite3_reset(*stmt_ptr);
        check_result_ok(*stmt_ptr, result_code);
        parameter_index = 1;
        if (clear_bindings)
        {
            this->clear_bindings();
        }
    }

    void Statement::clear_bindings()
    {
        sqlite3_clear_bindings(*stmt_ptr);
    }

    Result& Result::operator>>(bool& value)
    {
        assert(counter < column_count);
        value = static_cast<bool>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(char& value)
    {
        assert(counter < column_count);
        value = static_cast<char>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(signed char& value)
    {
        assert(counter < column_count);
        value = static_cast<signed char>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(unsigned char& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned char>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(short& value)
    {
        assert(counter < column_count);
        value = static_cast<short>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(unsigned short& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned short>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(int& value)
    {
        assert(counter < column_count);
        value = static_cast<int>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(unsigned int& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned int>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(long& value)
    {
        assert(counter < column_count);
        value = static_cast<long>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(unsigned long& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned long>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(long long& value)
    {
        assert(counter < column_count);
        value = static_cast<long long>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(unsigned long long& value)
    {
        assert(counter < column_count);
        value = static_cast<unsigned long long>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(float& value)
    {
        assert(counter < column_count);
        value = static_cast<float>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(double& value)
    {
        assert(counter < column_count);
        value = static_cast<double>((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(std::string& value)
    {
        assert(counter < column_count);
        value = std::string((*this)[counter++]);
        return *this;
    }

    Result& Result::operator>>(std::u16string& value)
    {
        assert(counter < column_count);
        value = std::u16string((*this)[counter++]);
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
            throw_error(stmt, result_code);
            return false;
        }
    }

    ResultField::operator bool() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return sqlite3_column_int(stmt, index) != 0;
    }

    ResultField::operator char() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<char>(sqlite3_column_int(stmt, index));
    }

    ResultField::operator signed char() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<signed char>(sqlite3_column_int(stmt, index));
    }

    ResultField::operator unsigned char() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<unsigned char>(sqlite3_column_int(stmt, index));
    }

    ResultField::operator short() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<short>(sqlite3_column_int(stmt, index));
    }

    ResultField::operator unsigned short() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<unsigned short>(sqlite3_column_int(stmt, index));
    }

    ResultField::operator int() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return sqlite3_column_int(stmt, index);
    }

    ResultField::operator unsigned int() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<unsigned int>(sqlite3_column_int64(stmt, index));
    }

    ResultField::operator long() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<long>(sqlite3_column_int64(stmt, index));
    }

    ResultField::operator unsigned long() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<unsigned long>(sqlite3_column_int64(stmt, index));
    }

    ResultField::operator long long() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return sqlite3_column_int64(stmt, index);
    }

    ResultField::operator unsigned long long() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_INTEGER);
        return static_cast<unsigned long long>(sqlite3_column_int64(stmt, index));
    }

    ResultField::operator float() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_FLOAT);
        return static_cast<float>(sqlite3_column_double(stmt, index));
    }

    ResultField::operator double() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_FLOAT);
        return sqlite3_column_double(stmt, index);
    }

    ResultField::operator std::string() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_TEXT);
        const unsigned char* value = sqlite3_column_text(stmt, index);
        return std::string(value, value + sqlite3_column_bytes(stmt, index));
    }

    ResultField::operator std::u16string() const
    {
        strict_type_check(strict_typing, column_type, SQLITE_TEXT);
        auto value = static_cast<const char16_t*>(sqlite3_column_text16(stmt, index));
        return std::u16string(value, value + sqlite3_column_bytes16(stmt, index) / sizeof(char16_t));
    }

    Transaction::Transaction(sqlite3* db) : db(db)
    {
        begin();
    }

    void Transaction::begin()
    {
        int result_code = sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
        check_result_ok(db, result_code);
        committed = false;
    }

    void Transaction::commit()
    {
        int result_code = sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
        check_result_ok(db, result_code);
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
