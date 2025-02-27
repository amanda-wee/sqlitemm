#ifndef SQLITEMM_20200528_H_
#define SQLITEMM_20200528_H_

/************************************************************************************************************
 * SQLitemm header file
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

#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "sqlite3.h"

/**
 * sqlitemm library.
 */
namespace sqlitemm
{
    class Backup;
    class Blob;
    class Connection;
    class Parameter;
    class Result;
    class ResultField;
    template<typename T>
    class ResultIterator;
    class Statement;
    class Transaction;

    /**
     * Base class for sqlitemm exceptions that wrap SQLite errors.
     */
    class Error : public std::exception
    {
    public:
        /**
         * Constructs an Error with the given message and error code.
         */
        Error(const std::string& message, int code) : what_arg(std::make_shared<std::string>(message)), error_code(code) {}

        /**
         * Returns the full error message.
         */
        virtual const char* what() const noexcept override
        {
            return what_arg->c_str();
        }

        /**
         * Returns the error code.
         */
        virtual int code() const noexcept
        {
            return error_code;
        }
    private:
        std::shared_ptr<std::string> what_arg; // full error message
        int error_code; // error code from SQLite
    };

    /**
     * Models a SQLite database connection.
     */
    class Connection
    {
    public:
        Connection(const Connection& other) = delete;
        void operator=(const Connection& other) = delete;

        /**
         * Constructs an empty database connection, i.e., one that is not
         * connected to a database.
         */
        Connection() = default;

        /**
         * Constructs a database connection by connecting to the database
         * specified by filename.
         */
        explicit Connection(const std::string& filename)
        {
            open(filename);
        }

        /**
         * Constructs a database connection by connecting to the database
         * specified by filename.
         */
        explicit Connection(const std::u16string& filename)
        {
            open(filename);
        }

        /**
         * Constructs a database connection by connecting to the database
         * specified by filename, with the given flags and optional VFS module
         * name.
         */
        Connection(const std::string& filename, int flags, const std::string& vfs = std::string{})
        {
            open(filename, flags, vfs);
        }

        /**
         * Move constructs the database connection.
         */
        Connection(Connection&& other) noexcept : db(other.db)
        {
            other.db = nullptr;
        }

        /**
         * Move assigns the database connection.
         */
        Connection& operator=(Connection&& other) noexcept
        {
            using std::swap;
            swap(db, other.db);
            swap(stmt_ptrs, other.stmt_ptrs);
            return *this;
        }

        /**
         * Destroys the database connection by closing it if it is open.
         */
        ~Connection()
        {
            close();
        }

        /**
         * Begins a transaction and returns it.
         */
        Transaction begin_transaction();

        /**
         * Returns the number of rows modified, inserted or deleted by the
         * most recently completed INSERT, UPDATE or DELETE statement on
         * the database connection.
         */
        size_t changes() const noexcept;

        /**
         * Closes the database connection if it is open.
         */
        void close() noexcept;

        /**
         * Creates a SQL aggregate function by forwarding the provided
         * arguments to a SQLite function creation routine for SQL aggregate
         * functions.
         */
        void create_aggregate_function(const std::string& function_name,
                                       int num_args,
                                       int text_encoding,
                                       void* app_user_data,
                                       void (*step_callback)(sqlite3_context*, int, sqlite3_value**),
                                       void (*final_callback)(sqlite3_context*),
                                       void (*destroy_callback)(void*) = nullptr);

        /**
         * Creates a SQL collation by forwarding the provided arguments to a
         * SQLite collation creation routine.
         */
        void create_collation(const std::string& collation_name,
                              int text_encoding,
                              void* app_user_data,
                              int (*compare_callback)(void*, int, const void*, int, const void*),
                              void (*destroy_callback)(void*) = nullptr);

        /**
         * Creates a SQL scalar function by forwarding the provided arguments
         * to a SQLite function creation routine for SQL scalar functions.
         */
        void create_scalar_function(const std::string& function_name,
                                    int num_args,
                                    int text_encoding,
                                    void* app_user_data,
                                    void (*func_callback)(sqlite3_context*, int, sqlite3_value**),
                                    void (*destroy_callback)(void*) = nullptr);

        /**
         * Creates a SQL window function by forwarding the provided arguments
         * to a SQLite function creation routine for SQL window functions.
         */
        void create_window_function(const std::string& function_name,
                                    int num_args,
                                    int text_encoding,
                                    void* app_user_data,
                                    void (*step_callback)(sqlite3_context*, int, sqlite3_value**),
                                    void (*final_callback)(sqlite3_context*),
                                    void (*value_callback)(sqlite3_context*),
                                    void (*inverse_callback)(sqlite3_context*, int, sqlite3_value**),
                                    void (*destroy_callback)(void*) = nullptr);

        /**
         * Executes zero or more UTF-8 encoded, semicolon-separate SQL
         * statements specified by the sql parameter.
         */
        void execute(const std::string& sql);

        /**
         * Returns the most recent result code associated with the database
         * connection.
         */
        int get_last_error_code() const noexcept;

        /**
         * Returns the most recent error message associated with the database
         * connection, or an empty string if there is none.
         */
        std::string get_last_error_message() const;

        /**
         * Causes any pending database operation to abort and return at its
         * earliest opportunity.
         */
        void interrupt() noexcept;

        /**
         * Returns true if an interrupt is in effect.
         */
        bool is_interrupted() const noexcept;

        /**
         * Returns the last insert row id, or 0 if there has not been a successful
         * insertion.
         */
        long long last_insert_rowid() const noexcept;

        /**
         * Loads an SQLite extension library from the named file.
         *
         * If the entry point name is an empty string, SQLite will try to come
         * up with an entry point name on its own.
         */
        void load_extension(const std::string& filename, const std::string entry_point_name = std::string{});

        /**
         * Connects to the database given by filename.
         */
        void open(const std::string& filename);

        /**
         * Connects to the database given by filename.
         */
        void open(const std::u16string& filename);

        /**
         * Connects to the database given by filename, with the given flags and
         * optional VFS module name.
         */
        void open(const std::string& filename, int flags, const std::string& vfs = std::string{});

        /**
         * Opens a blob for incremental I/O and returns the blob object.
         */
        Blob open_blob(
            const std::string& database, const std::string& table, const std::string& column, size_t row, int flags
        );

        /**
         * Returns a prepared statement for the single SQL statement
         * specified by the sql parameter.
         */
        Statement prepare(const std::string& sql);

        /**
         * Sets a busy handler that sleeps multiple times until at least ms
         * milliseconds of sleeping have accumulated when a table is locked.
         * If the table remains locked after sleeping, BusyError will be thrown
         * by the relevant execute or step function.
         */
        void set_busy_timeout(int ms) noexcept;

        /**
         * Sets the database configuration option denoted by config_option.
         */
        template<typename... Ts>
        void set_config(int config_option, Ts... args)
        {
            int result_code = sqlite3_db_config(db, config_option, args...);
            if (result_code != SQLITE_OK)
            {
                throw Error("could not set database configuration", result_code);
            }
        }

        /**
         * Returns the total number of rows inserted, modified or deleted by
         * all INSERT, UPDATE or DELETE statements completed since the
         * database connection was opened, including those executed as part of
         * trigger programs.
         */
        size_t total_changes() const noexcept;
    private:
        sqlite3* db = nullptr; // database connection handle
        // prepared statements that were prepared via this database connection
        std::vector<std::weak_ptr<sqlite3_stmt*>> stmt_ptrs;

        void clean_stmt_ptrs();
        friend class Backup;
    };

    /**
     * Attach another database to the database connection, identifying the
     * objects from the other database using a separate schema name.
     */
    void attach(Connection& connection, const std::string& filename, const std::string& schema_name);

    /**
     * Detach the database identified with the schema name from the database
     * connection.
     */
    void detach(Connection& connection, const std::string& schema_name);

    /**
     * Models an online database backup.
     */
    class Backup
    {
    public:
        Backup() = delete;
        Backup(const Backup& other) = delete;
        void operator=(const Backup& other) = delete;

        /**
         * Create backup to backup from the source database using the source
         * database connection and database name to the destination database
         * using the destination database connection and database name.
         *
         * As per SQLite documentation: The database name is "main" for the
         * main database, "temp" for the temporary database, or the name
         * specified after the AS keyword in an ATTACH statement for an
         * attached database.
         */
        Backup(Connection& source, const std::string& source_database,
               Connection& destination, const std::string& destination_database);

        /**
         * Move constructs the backup object.
         */
        Backup(Backup&& other) noexcept : db(other.db), backup(other.backup)
        {
            other.db = nullptr;
            other.backup = nullptr;
        }

        /**
         * Move assigns the backup object.
         */
        Backup& operator=(Backup&& other) noexcept
        {
            using std::swap;
            swap(db, other.db);
            swap(backup, other.backup);
            return *this;
        }

        /**
         * Destroys the backup object by closing the backup handle.
         */
        ~Backup()
        {
            close();
        }

        /**
         * Copies up to num_pages pages between the source and destination
         * databases.
         *
         * Returns true if copying is successful and there are more pages to
         * be copied.
         */
        bool step(int num_pages);

        /**
         * Returns the number of pages remaining after the most recent call to
         * step().
         */
        int pages_remaining() const noexcept;

        /**
         * Returns the total number of pages after the most recent call to
         * step().
         */
        int page_count() const noexcept;

        /**
         * Closes the backup handle and releases all resources associated with
         * the backup operation.
         */
        void close() noexcept;
    private:
        sqlite3* db = nullptr; // database connection handle for error reporting
        sqlite3_backup* backup = nullptr; // backup handle
    };

    /**
     * Models a BLOB value for parameter binding that handles its own
     * memory allocation, providing a destructor for SQLite to deallocate
     * the memory accordingly.
     */
    struct BlobValue
    {
        /**
         * Initialises the BLOB value.
         */
        BlobValue(const void* content, size_t num_bytes, sqlite3_destructor_type destructor = nullptr) :
            content(content), num_bytes(num_bytes), destructor(destructor) {}

        /// Content of the BLOB.
        const void* content;
        /// Number of bytes of the BLOB.
        size_t num_bytes;
        /// Destructor to be invoked by SQLite.
        sqlite3_destructor_type destructor;
    };

    /**
     * Models a text value for parameter binding that handles its own
     * memory allocation, providing a destructor for SQLite to deallocate
     * the memory accordingly.
     */
    struct TextValue
    {
        /**
         * Initialises the text value.
         */
        TextValue(const char* content, int num_bytes, sqlite3_destructor_type destructor = nullptr) :
            content(content), num_bytes(num_bytes), destructor(destructor) {}

        /// Content of the text.
        const char* content;
        /// Number of bytes of the text.
        int num_bytes;
        /// Destructor to be invoked by SQLite.
        sqlite3_destructor_type destructor;
    };

    /**
     * Models a zero blob placeholder for parameter binding that allows for
     * binding a blob of num_bytes length that is filled with zeroes.
     */
    struct ZeroBlob
    {
        /**
         * Initialises the zero blob placeholder.
         */
        explicit ZeroBlob(size_t num_bytes) : num_bytes(num_bytes) {}

        /// Number of bytes of the zero blob.
        size_t num_bytes;
    };

    /**
     * Models a blob for incremental I/O.
     */
    class Blob
    {
    public:
        /// Flag to open the blob for reading only.
        static constexpr int READ_ONLY = 0;
        /// Flag to open the blob for both reading and writing.
        static constexpr int READ_WRITE = 1;

        Blob() = delete;
        Blob(const Blob& other) = delete;
        void operator=(const Blob& other) = delete;

        /**
         * Move constructs the blob object.
         */
        Blob(Blob&& other) noexcept : db(other.db), blob(other.blob)
        {
            other.db = nullptr;
            other.blob = nullptr;
        }

        /**
         * Move assigns the blob object.
         */
        Blob& operator=(Blob&& other) noexcept
        {
            using std::swap;
            swap(db, other.db);
            swap(blob, other.blob);
            return *this;
        }

        /**
         * Destroys the blob object by closing the blob handle.
         */
        ~Blob()
        {
            close();
        }

        /**
         * Closes the blob handle if it is open.
         */
        void close() noexcept;

        /**
         * Reads num_bytes bytes from the blob into buffer starting at
         * blob_offset.
         */
        void read(void* buffer, size_t num_bytes, size_t blob_offset);

        /**
         * Writes num_bytes bytes from the buffer into the blob starting at
         * blob_offset.
        */
        void write(const void* buffer, size_t num_bytes, size_t blob_offset);

        /**
         * Returns the size of the blob in bytes.
         */
        size_t size() const noexcept;

        /**
         * Reopens the blob to the specified row in the original table and
         * column.
         */
        void reopen(size_t row);
    private:
        sqlite3* db = nullptr; // database connection handle for error reporting
        sqlite3_blob* blob = nullptr; // blob handle

        explicit Blob(sqlite3* db, sqlite3_blob* blob) : db(db), blob(blob) {}

        friend Blob Connection::open_blob(
            const std::string& database, const std::string& table, const std::string& column, size_t row, int flags
        );
    };

    /**
     * Models a named parameter in a prepared statement.
     */
    class Parameter
    {
    public:
        Parameter() = delete;
        Parameter(const Parameter& other) = delete;
        void operator=(const Parameter& other) = delete;
        void operator=(Parameter&& other) = delete;

        /**
         * Move constructs the named parameter.
         */
        Parameter(Parameter&& other) noexcept : stmt(other.stmt), index(other.index)
        {
            other.stmt = nullptr;
            other.index = 0;
        }

        /**
         * Binds NULL to the named parameter.
         */
        void operator=(std::nullptr_t value);

        /**
         * Converts value to int and binds the result to the named parameter.
         */
        void operator=(bool value);

        /**
         * Converts value to int and binds the result to the named parameter.
         */
        void operator=(char value);

        /**
         * Converts value to int and binds the result to the named parameter.
         */
        void operator=(signed char value);

        /**
         * Converts value to int and binds the result to the named parameter.
         */
        void operator=(unsigned char value);

        /**
         * Converts value to int and binds the result to the named parameter.
         */
        void operator=(short value);

        /**
         * Converts value to int and binds the result to the named parameter.
         */
        void operator=(unsigned short value);

        /**
         * Binds value to the named parameter.
         */
        void operator=(int value);

        /**
         * Converts value to long long and binds the result to the named
         * parameter.
         */
        void operator=(unsigned int value);

        /**
         * Converts value to int or long long, depending on whether a long will
         * fit in an int or long long, and binds the result to the named
         * parameter.
         */
        void operator=(long value);

        /**
         * Converts value to long long and binds the result to the named
         * parameter.
         */
        void operator=(unsigned long value);

        /**
         * Binds value to the named parameter.
         */
        void operator=(long long value);

        /**
         * Converts value to long long and binds the result to the named
         * parameter.
         */
        void operator=(unsigned long long value);

        /**
         * Converts value to double and binds the result to the named
         * parameter.
         */
        void operator=(float value);

        /**
         * Binds value to the named parameter.
         */
        void operator=(double value);

        /**
         * Binds the content of value and its length to the named parameter,
         * without copying.
         */
        void operator=(const std::string& value);

        /**
         * Binds a copy of the content of value and its length to the named
         * parameter.
         */
        void operator=(std::string&& value);

        /**
         * Binds the content of value and its length to the named parameter,
         * without copying.
         */
        void operator=(const std::u16string& value);

        /**
         * Binds a copy of the content of value and its length to the named
         * parameter.
         */
        void operator=(std::u16string&& value);

        /**
         * Binds the content of value and its null-terminated length to the
         * named parameter, without copying.
         */
        void operator=(const char* value);

        /**
         * Binds value to the named parameter. The content will be destroyed
         * by the provided destructor.
         */
        void operator=(const BlobValue& value);

        /**
         * Binds value to the named parameter. The content will be destroyed
         * by the provided destructor.
         */
        void operator=(const TextValue& value);

        /**
         * Binds a zero-filled blob of the length of value.num_bytes to the
         * named parameter.
         */
        void operator=(const ZeroBlob& value);

        /**
         * Binds the value of value to the named parameter if it has a value,
         * otherwise binds NULL to the named parameter.
         */
        template<typename T>
        void operator=(const std::optional<T>& value)
        {
            if (value.has_value())
            {
                *this = *value;
            }
            else
            {
                *this = nullptr;
            }
        }

        /**
         * Binds the value of value to the named parameter if it has a value,
         * otherwise binds NULL to the named parameter.
         */
        template<typename T>
        void operator=(std::optional<T>&& value)
        {
            if (value.has_value())
            {
                *this = std::move(*value);
            }
            else
            {
                *this = nullptr;
            }
        }
    private:
        sqlite3_stmt* stmt; // prepared statement handle
        int index;          // index of the named parameter in the prepared statement

        Parameter(sqlite3_stmt* stmt, const char* name);
        friend class Statement;
    };

    /**
     * Represents a prepared statement.
     */
    class Statement
    {
    public:
        Statement() = delete;
        Statement(const Statement& other) = delete;
        void operator=(const Statement& other) = delete;

        /**
         * Move constructs the prepared statement.
         */
        Statement(Statement&& other) noexcept :
            stmt_ptr(std::move(other.stmt_ptr)), parameter_index(other.parameter_index) {}

        /**
         * Move assigns the prepared statement.
         */
        Statement& operator=(Statement&& other) noexcept;

        /**
         * Destroys the prepared statement by finalizing it if it has not been
         * finalized.
         */
        ~Statement()
        {
            finalize();
        }

        /**
         * Executes the prepared statement without returning a result set.
         */
        void execute();

        /**
         * Executes the prepared statement, returning the corresponding
         * result set as a Result object.
         */
        Result execute_query(bool strict_typing = false);

        /**
         * Resets the prepared statement for future execution.
         * If clear_bindings is true, the parameter bindings will also be cleared.
         */
        void reset(bool clear_bindings = false);

        /**
         * Clears the existing parameter bindings.
         */
        void clear_bindings();

        /**
         * Finalizes the prepared statement.
         * This effectively destroys the prepared statement.
         */
        bool finalize() noexcept;

        /**
         * Binds NULL to the current parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(std::nullptr_t value);

        /**
         * Converts value to int and binds the result to the current parameter,
         * then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(bool value);

        /**
         * Converts value to int and binds the result to the current parameter,
         * then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(char value);

        /**
         * Converts value to int and binds the result to the current parameter,
         * then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(signed char value);

        /**
         * Converts value to int and binds the result to the current parameter,
         * then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(unsigned char value);

        /**
         * Converts value to int and binds the result to the current parameter,
         * then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(short value);

        /**
         * Converts value to int and binds the result to the current parameter,
         * then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(unsigned short value);

        /**
         * Binds value to the current parameter, then advances to the next
         * parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(int value);

        /**
         * Converts value to long long and binds the result to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(unsigned int value);

        /**
         * Converts value to int or long long, depending on whether a long will
         * fit in an int or long long, and binds the result to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(long value);

        /**
         * Converts value to long long and binds the result to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(unsigned long value);

        /**
         * Binds value to the current parameter, then advances to the next
         * parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(long long value);

        /**
         * Converts value to long long and binds the result to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(unsigned long long value);

        /**
         * Converts value to double and binds the result to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(float value);

        /**
         * Binds value to the current parameter, then advances to the next
         * parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(double value);

        /**
         * Binds the content of value and its length to the current parameter,
         * without copying, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(const std::string& value);

        /**
         * Binds a copy of the content of value and its length to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(std::string&& value);

        /**
         * Binds the content of value and its length to the current parameter,
         * without copying, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(const std::u16string& value);

        /**
         * Binds a copy of the content of value and its length to the current
         * parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(std::u16string&& value);

        /**
         * Binds the content of value and its null-terminated length to the
         * current parameter, without copying, then advances to the next
         * parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(const char* value);

        /**
         * Binds the value to the current parameter, then advances to the next
         * parameter. The content will be destroyed by the provided destructor.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(const BlobValue& value);

        /**
         * Binds the value to the current parameter, then advances to the next
         * parameter. The content will be destroyed by the provided destructor.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(const TextValue& value);

        /**
         * Binds a zero-filled blob of the length of value.num_bytes to the
         * current parameter, then advances to the next parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(const ZeroBlob& value);

        /**
         * Allows for parameter binding by name as a null-terminated string.
         */
        Parameter operator[](const char* name)
        {
            return Parameter(*stmt_ptr, name);
        }

        /**
         * Allows for parameter binding by name as a std::string.
         */
        Parameter operator[](const std::string& name)
        {
            return (*this)[name.c_str()];
        }
    private:
        std::shared_ptr<sqlite3_stmt*> stmt_ptr; // prepared statement handle
        int parameter_index = 1;                 // index of current parameter for binding

        explicit Statement(std::shared_ptr<sqlite3_stmt*> stmt_ptr) : stmt_ptr(stmt_ptr) {}

        friend Statement Connection::prepare(const std::string& sql);
    };

    /**
     * Binds the value to the current parameter of the prepared statement
     * object if it has a value, otherwise binds NULL to the current
     * parameter, then advances to the next parameter. Returns a reference to
     * the prepared statement object.
     */
    template<typename T>
    Statement& operator<<(Statement& statement, const std::optional<T>& value)
    {
        if (value.has_value())
        {
            statement << *value;
        }
        else
        {
            statement << nullptr;
        }
        return statement;
    }

    //! @copydoc operator<<(Statement&,const std::optional<T>&)
    template<typename T>
    Statement& operator<<(Statement& statement, std::optional<T>&& value)
    {
        if (value.has_value())
        {
            statement << std::move(*value);
        }
        else
        {
            statement << nullptr;
        }
        return statement;
    }

    /**
     * Binds the elements of the tuple argument in sequence to the current
     * and subsequent parameters of the prepared statement object, then
     * advances to the next parameter. Returns a reference to the prepared
     * statement object. The caller is responsible for ensuring that the
     * number of elements of the tuple do not exceed the number of remaining
     * parameters to be bound.
     */
    template<typename T1>
    Statement& operator<<(Statement& statement, const std::tuple<T1>& value)
    {
        return statement << std::get<0>(value);
    }

    //! @copydoc operator<<(Statement&,const std::tuple<T1>&)
    template<typename T1, typename T2>
    Statement& operator<<(Statement& statement, const std::tuple<T1, T2>& value)
    {
        using std::get;
        return statement << get<0>(value) << get<1>(value);
    }

    //! @copydoc operator<<(Statement&,const std::tuple<T1>&)
    template<typename T1, typename T2, typename T3>
    Statement& operator<<(Statement& statement, const std::tuple<T1, T2, T3>& value)
    {
        using std::get;
        return statement << get<0>(value) << get<1>(value) << get<2>(value);
    }

    //! @copydoc operator<<(Statement&,const std::tuple<T1>&)
    template<typename T1, typename T2, typename T3, typename T4>
    Statement& operator<<(Statement& statement, const std::tuple<T1, T2, T3, T4>& value)
    {
        using std::get;
        return statement << get<0>(value) << get<1>(value) << get<2>(value) << get<3>(value);
    }

    //! @copydoc operator<<(Statement&,const std::tuple<T1>&)
    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    Statement& operator<<(Statement& statement, const std::tuple<T1, T2, T3, T4, T5>& value)
    {
        using std::get;
        return statement << get<0>(value) << get<1>(value) << get<2>(value) << get<3>(value) << get<4>(value);
    }

    /**
     * Models a field in a result row.
     */
    class ResultField
    {
    public:
        ResultField() = delete;

        /**
         * Returns the result field as a bool.
         */
        operator bool() const;

        /**
         * Returns the result field as a char.
         */
        operator char() const;

        /**
         * Returns the result field as a signed char.
         */
        operator signed char() const;

        /**
         * Returns the result field as an unsigned char.
         */
        operator unsigned char() const;

        /**
         * Returns the result field as a short.
         */
        operator short() const;

        /**
         * Returns the result field as an unsigned short.
         */
        operator unsigned short() const;

        /**
         * Returns the result field as an int.
         */
        operator int() const;

        /**
         * Returns the result field as an unsigned int.
         */
        operator unsigned int() const;

        /**
         * Returns the result field as a long.
         */
        operator long() const;

        /**
         * Returns the result field as an unsigned long.
         */
        operator unsigned long() const;

        /**
         * Returns the result field as a long long.
         */
        operator long long() const;

        /**
         * Returns the result field as an unsigned long long.
         */
        operator unsigned long long() const;

        /**
         * Returns the result field as a float.
         */
        operator float() const;

        /**
         * Returns the result field as a double.
         */
        operator double() const;

        /**
         * Returns the result field as a std::string.
         */
        operator std::string() const;

        /**
         * Returns the result field as a std::u16string.
         */
        operator std::u16string() const;

        /**
         * Returns the result field as a std::optional<T>.
         */
        template<typename T>
        std::optional<T> as_optional() const
        {
            std::optional<T> result;
            if (column_type != SQLITE_NULL)
            {
                result.emplace(*this);
            }
            return result;
        }

        /**
         * Reads the field as UTF-8 encoded text, done by invoking the
         * function object argument with two arguments:
         *   * the field in the result row as a const unsigned char*
         *   * the number of bytes of the field in the result row,
         *     excluding the terminating null character
         *
         * The function object is expected to copy the string.
         */
        void as_text(std::function<void(const unsigned char*, int)> retrieval_func) const
        {
            auto value = sqlite3_column_text(stmt, index);
            retrieval_func(value, sqlite3_column_bytes(stmt, index));
        }

        /**
         * Reads the field as UTF-16 encoded text, done by invoking the
         * function object argument with two arguments:
         *   * the field in the result row as a const void*
         *   * the number of bytes of the field in the result row,
         *     excluding the terminating null character
         *
         * The function object is expected to copy the string.
         */
        void as_text16(std::function<void(const void*, int)> retrieval_func) const
        {
            auto value = sqlite3_column_text16(stmt, index);
            retrieval_func(value, sqlite3_column_bytes16(stmt, index));
        }

        /**
         * Reads the field as a BLOB, done by invoking the function object
         * argument with two arguments:
         *   * the field in the result row as a const void*
         *   * the number of bytes of the field in the result row
         *
         * The function object is expected to copy the bytes of the BLOB.
         */
        void as_blob(std::function<void(const void*, int)> retrieval_func) const
        {
            auto value = sqlite3_column_blob(stmt, index);
            retrieval_func(value, sqlite3_column_bytes(stmt, index));
        }
    private:
        sqlite3_stmt* stmt;
        int index;
        int column_type;
        bool strict_typing;

        ResultField(sqlite3_stmt* stmt, int index, bool strict_typing) noexcept :
            stmt(stmt), index(index), strict_typing(strict_typing)
        {
            column_type = sqlite3_column_type(stmt, index);
        }

        friend class Result;
    };

    /**
     * Models a result set, and a result row thereof, from executing a query
     * through a prepared statement.
     */
    class Result
    {
    public:
        Result() = delete;
        Result(const Result& other) = delete;
        void operator=(const Result& other) = delete;

        /**
         * Move constructs the result set.
         */
        Result(Result&& other) noexcept : stmt(other.stmt), counter(other.counter), column_count(other.column_count)
        {
            other.stmt = nullptr;
            other.counter = 0;
            other.column_count = 0;
        }

        /**
         * Move assigns the result set.
         */
        Result& operator=(Result&& other) noexcept
        {
            if (this != &other)
            {
                stmt = other.stmt;
                counter = other.counter;
                column_count = other.column_count;
                other.stmt = nullptr;
                other.counter = 0;
                other.column_count = 0;
            }
            return *this;
        }

        /**
         * Steps through the result set to advance to the next result row.
         *
         * Must be called before reading any fields in the result row.
         * Returns true if there is a result row to read from.
         */
        bool step();

        /**
         * Returns the result field corresponding to the index (starting from 0).
         */
        ResultField operator[](int index) const noexcept
        {
            return ResultField(stmt, index, strict_typing);
        }

        /**
         * Returns a result iterator for T that points to the first row of the
         * result set.
         * If the result row retrieval function is supplied, then that will be
         * used to retrieve the row as an object of type T. Otherwise, it will
         * default to initialising an object of type T with the result object.
         */
        template<typename T>
        ResultIterator<T> begin(
            std::function<T(Result&)> retrieval_func = [](Result& result) -> T { return T(result); }
        )
        {
            return ResultIterator<T>(*this, retrieval_func);
        }

        /**
         * Returns an empty result iterator for T.
         */
        template<typename T>
        ResultIterator<T> end()
        {
            return ResultIterator<T>();
        }

        /**
         * Reads and converts the current field in the result row to bool and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(bool& value);

        /**
         * Reads and converts the current field in the result row to char and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(char& value);

        /**
         * Reads and converts the current field in the result row to signed char
         * and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(signed char& value);

        /**
         * Reads and converts the current field in the result row to unsigned
         * char and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(unsigned char& value);

        /**
         * Reads and converts the current field in the result row to short and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(short& value);

        /**
         * Reads and converts the current field in the result row to unsigned
         * short and stores it in value, then advances to the next field, if
         * any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(unsigned short& value);

        /**
         * Reads and converts the current field in the result row to int and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(int& value);

        /**
         * Reads and converts the current field in the result row to unsigned
         * int and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(unsigned int& value);

        /**
         * Reads and converts the current field in the result row to long and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(long& value);

        /**
         * Reads and converts the current field in the result row to unsigned
         * long and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(unsigned long& value);

        /**
         * Reads and converts the current field in the result row to long long
         * and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(long long& value);

        /**
         * Reads and converts the current field in the result row to unsigned
         * long long and stores it in value, then advances to the next field, if
         * any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(unsigned long long& value);

        /**
         * Reads and converts the current field in the result row to float and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(float& value);

        /**
         * Reads and converts the current field in the result row to double and
         * stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(double& value);

        /**
         * Reads and converts the current field in the result row to std::string
         * and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(std::string& value);

        /**
         * Reads and converts the current field in the result row to
         * std::u16string and stores it in value, then advances to the next
         * field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(std::u16string& value);

        /**
         * If the current field in the result row is NULL, this makes value an
         * empty std::optional<T>. Otherwise reads and converts the current
         * field to the type T and stores it in value. Always advances to the next
         * field, if any.
         * Returns a reference to this Result object.
         */
        template<typename T>
        Result& operator>>(std::optional<T>& value)
        {
            if (sqlite3_column_type(stmt, counter) != SQLITE_NULL)
            {
                T temp;
                *this >> temp;
                value = temp;
            }
            else
            {
                value.reset();
                ++counter;
            }
            return *this;
        }

        /**
         * Reads the current field as UTF-8 encoded text, done by invoking the
         * function object argument with two arguments:
         *   * the current field in the result row as a const unsigned char*
         *   * the number of bytes of the current field in the result row,
         *     excluding the terminating null character
         *
         * Advances to the next field, if any.
         *
         * The function object is expected to copy the string.
         */
        void as_text(std::function<void(const unsigned char*, int)> retrieval_func)
        {
            assert(counter < column_count);
            (*this)[counter++].as_text(retrieval_func);
        }

        /**
         * Reads the current field as UTF-16 encoded text, done by invoking the
         * function object argument with two arguments:
         *   * the current field in the result row as a const void*
         *   * the number of bytes of the current field in the result row,
         *     excluding the terminating null character
         *
         * Advances to the next field, if any.
         *
         * The function object is expected to copy the string.
         */
        void as_text16(std::function<void(const void*, int)> retrieval_func)
        {
            assert(counter < column_count);
            (*this)[counter++].as_text16(retrieval_func);
        }

        /**
         * Reads the current field as a BLOB, done by invoking the function
         * object argument with two arguments:
         *   * the current field in the result row as a const void*
         *   * the number of bytes of the current field in the result row
         *
         * Advances to the next field, if any.
         *
         * The function object is expected to copy the bytes of the BLOB.
         */
        void as_blob(std::function<void(const void*, int)> retrieval_func)
        {
            assert(counter < column_count);
            (*this)[counter++].as_blob(retrieval_func);
        }
    private:
        sqlite3_stmt* stmt;   // prepared statement handle
        int counter = 0;      // field counter for each result row
        int column_count = 0; // number of columns in the result set
        bool strict_typing = false; // true if SQLite automatic type conversions should be prevented

        explicit Result(sqlite3_stmt* stmt, bool strict_typing) : stmt(stmt), strict_typing(strict_typing) {}

        friend Result Statement::execute_query(bool strict_typing);
    };

    /**
     * Reads and converts the current and subsequent fields in the result row
     * to the corresponding elements of the tuple, then advances to the next
     * field, if any. Returns a reference to the Result object.
     */
    template<typename T1>
    Result& operator>>(Result& result, std::tuple<T1>& values)
    {
        return result >> std::get<0>(values);
    }

    //! @copydoc operator>>(Result&,std::tuple<T1>&)
    template<typename T1, typename T2>
    Result& operator>>(Result& result, std::tuple<T1, T2>& values)
    {
        using std::get;
        return result >> get<0>(values) >> get<1>(values);
    }

    //! @copydoc operator>>(Result&,std::tuple<T1>&)
    template<typename T1, typename T2, typename T3>
    Result& operator>>(Result& result, std::tuple<T1, T2, T3>& values)
    {
        using std::get;
        return result >> get<0>(values) >> get<1>(values) >> get<2>(values);
    }

    //! @copydoc operator>>(Result&,std::tuple<T1>&)
    template<typename T1, typename T2, typename T3, typename T4>
    Result& operator>>(Result& result, std::tuple<T1, T2, T3, T4>& values)
    {
        using std::get;
        return result >> get<0>(values) >> get<1>(values) >> get<2>(values) >> get<3>(values);
    }

    //! @copydoc operator>>(Result&,std::tuple<T1>&)
    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    Result& operator>>(Result& result, std::tuple<T1, T2, T3, T4, T5>& values)
    {
        using std::get;
        return result >> get<0>(values) >> get<1>(values) >> get<2>(values) >> get<3>(values) >> get<4>(values);
    }

    /**
     * Input iterator to the rows of a result set.
     *
     * To use a result iterator for some type T, either define a constructor
     * for T that takes a reference to Result, with which it reads the row
     * from result set that the iterator points to, or provide the iterator
     * with a result row retrieval callback function that takes a reference to
     * Result and returns an object of type T.
     */
    template<typename T>
    class ResultIterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using retrieval_function_type = std::function<T(Result&)>;

        /**
         * Constructs an empty result iterator, i.e., denoting the end of the
         * result set.
         */
        ResultIterator() = default;

        /**
         * Constructs an iterator to the first row of the result set.
         * If the result row retrieval function is supplied, then that will be
         * used to retrieve the row that the iterator points to as an object
         * of type T. Otherwise, it will default to initialising an object of
         * type T with the result object.
         *
         * Note that this means stepping through the result set once.
         */
        explicit ResultIterator(
            Result& result_,
            retrieval_function_type retrieval_func = [](Result& result) -> T { return T(result); }
        ) : result(&result_), retrieval_function(retrieval_func)
        {
            if (!result->step())
            {
                result = nullptr;
            }
        }

        /**
         * Returns an object of type T retrieved from the result row that this
         * iterator points to.
         */
        T operator*() const
        {
            return retrieval_function(*result);
        }

        /**
         * Pre-increments the iterator to step to the next row of the result set.
         */
        ResultIterator& operator++()
        {
            if (!result->step())
            {
                result = nullptr;
            }
            return *this;
        }

        /**
         * Post-increments the iterator to step to the next row of the result set.
         */
        ResultIterator operator++(int)
        {
            ResultIterator temp(*this);
            ++*this;
            return temp;
        }

        /**
         * Converts the iterator to bool by returning true if the iterator is not
         * empty.
         */
        explicit operator bool() const noexcept
        {
            return result != nullptr;
        }

        /**
         * Compares the iterator with another result iterator for equality. Two
         * result iterators are equal if they point to the same result set, or if
         * they are both empty result iterators.
         */
        bool operator==(const ResultIterator& other) const noexcept
        {
            return result == other.result;
        }
    private:
        Result* result = nullptr;
        retrieval_function_type retrieval_function = nullptr;
    };

    /**
     * Compares the iterator with another result iterator for inequality.
     * Two result iterators are not equal if they do not point to the same
     * result set, or if one of them is empty but the other is not empty.
     */
    template<typename T>
    bool operator!=(const ResultIterator<T>& lhs, const ResultIterator<T>& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    /**
     * Allows result iterators to be used in range-based for loops.
     */
    template<typename T>
    ResultIterator<T> begin(ResultIterator<T> iter)
    {
        return iter;
    }

    /**
     * Allows result iterators to be used in range-based for loops.
     */
    template<typename T>
    ResultIterator<T> end(ResultIterator<T> iter)
    {
        return ResultIterator<T>();
    }

    /**
     * Models a database transaction.
     */
    class Transaction
    {
    public:
        Transaction() = delete;
        Transaction(const Transaction& other) = delete;
        void operator=(const Transaction& other) = delete;
        void operator=(Transaction&& other) = delete;

        /**
         * Move constructs the transaction.
         */
        Transaction(Transaction&& other) noexcept : db(other.db), committed(other.committed)
        {
            other.db = nullptr;
            other.committed = false;
        }

        /**
         * Destroys the transaction by rolling back if it has not been committed
         * or rolled back.
         */
        ~Transaction()
        {
            if (!committed)
            {
                rollback();
            }
        }

        /**
         * Begins the transaction.
         *
         * This should only be called to reuse a transaction object.
         */
        void begin();

        /**
         * Commits the transaction.
         */
        void commit();

        /**
         * Rolls back the transaction if it has not been rolled back.
         */
        void rollback() noexcept;
    private:
        sqlite3* db;            // database connection handle
        bool committed = false; // true if the transaction has been committed

        explicit Transaction(sqlite3* db);

        friend Transaction Connection::begin_transaction();
    };

    /**
     * Intermediate base class for SQLite busy and locked errors so that they
     * can be easily handled in the same handler.
     */
    class BusyOrLockedError : public Error
    {
    public:
        using Error::Error;
    };

    /**
     * SQLite busy errors.
     */
    class BusyError : public BusyOrLockedError
    {
    public:
        using BusyOrLockedError::BusyOrLockedError;
    };

    /**
     * SQLite constraint errors.
     */
    class ConstraintError : public Error
    {
    public:
        using Error::Error;
    };

    /**
     * SQLite locked errors.
     */
    class LockedError : public BusyOrLockedError
    {
    public:
        using BusyOrLockedError::BusyOrLockedError;
    };

    /**
     * SQLite type errors when strict typing is enabled.
     *
     * SQLite normally does automatic type conversions between its
     * fundamental types, but if strict typing is enabled in SQLitemm, an
     * attempt to do an automatic type conversion will result in this
     * exception being thrown.
     */
    class TypeError : public Error
    {
    public:
        using Error::Error;
    };

    /**
     * SQLite null type errors when strict typing is enabled.
     *
     * SQLite normally does automatic type conversions, but if strict typing
     * is enabled in SQLitemm, an attempt to do an automatic type
     * conversion from NULL to another SQLite fundamental type without
     * using std::optional will result in this exception being thrown.
     */
    class NullTypeError : public TypeError
    {
    public:
        using TypeError::TypeError;
    };
}

#endif
