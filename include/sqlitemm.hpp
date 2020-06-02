#ifndef SQLITEMM_20200528_H_
#define SQLITEMM_20200528_H_

/************************************************************************************************************
 * SQLitemm header file
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

#include <cassert>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <vector>
#include "sqlite3.h"

/**
 * sqlitemm library.
 */
namespace sqlitemm
{
    class Statement;
    class Transaction;

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
         * Closes the database connection if it is open.
         */
        void close() noexcept;

        /**
         * Executes zero or more UTF-8 encoded, semicolon-separate SQL
         * statements specified by the sql parameter.
         */
        void execute(const std::string& sql);

        /**
         * Returns a prepared statement for the single SQL statement
         * specified by the sql parameter.
         */
        Statement prepare(const std::string& sql);

        /**
         * Begins a transaction and returns it.
         */
        Transaction begin_transaction();
    private:
        sqlite3* db = nullptr; // database connection handle
        // prepared statements that were prepared via this database connection
        std::vector<std::weak_ptr<sqlite3_stmt*>> stmt_ptrs;

        void clean_stmt_ptrs();
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
     * sqlitemm implementation detail.
     */
    namespace detail
    {
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
             * Binds value to the named parameter.
             */
            void operator=(long long value);

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
            void operator=(const sqlitemm::BlobValue& value);

            /**
             * Binds value to the named parameter. The content will be destroyed
             * by the provided destructor.
             */
            void operator=(const sqlitemm::TextValue& value);
        private:
            sqlite3_stmt* stmt; // prepared statement handle
            int index;          // index of the named parameter in the prepared statement

            Parameter(sqlite3_stmt* stmt, const char* name);
            friend class sqlitemm::Statement;
        };
    }

    class Result;

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
        Statement(Statement&& other) noexcept : stmt_ptr(std::move(other.stmt_ptr)), column_counter(other.column_counter) {}

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
         * Finalizes the prepared statement.
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
         * Binds value to the current parameter, then advances to the next
         * parameter.
         * Returns a reference to this prepared statement object.
         */
        Statement& operator<<(long long value);

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
         * Allows for parameter binding by name as a null-terminated string.
         */
        detail::Parameter operator[](const char* name)
        {
            return detail::Parameter(*stmt_ptr, name);
        }

        /**
         * Allows for parameter binding by name as a std::string.
         */
        detail::Parameter operator[](const std::string& name)
        {
            return (*this)[name.c_str()];
        }

        /**
         * Executes the prepared statement without returning a result set.
         */
        void execute();

        /**
         * Executes the prepared statement, returning the corresponding
         * result set as a Result object.
         */
        Result execute_query();

        /**
         * Resets the prepared statement for future execution.
         */
        void reset();
    private:
        std::shared_ptr<sqlite3_stmt*> stmt_ptr; // prepared statement handle
        int column_counter = 1;                  // parameter counter for binding

        explicit Statement(std::shared_ptr<sqlite3_stmt*> stmt_ptr) : stmt_ptr(stmt_ptr) {}

        friend Statement Connection::prepare(const std::string& sql);
    };

    /**
     * Models a result set from executing a query through a prepared statement.
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
         * Reads and converts the current field in the result row to long long
         * and stores it in value, then advances to the next field, if any.
         * Returns a reference to this Result object.
         */
        Result& operator>>(long long& value);

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
         * Reads the current field as UTF-8 encoded text, done by invoking the
         * function object argument with two arguments:
         *   * the current field in the result row as a const unsigned char*
         *   * the number of bytes of the current field in the result row,
         *     excluding the terminating null character
         * Advances to the next field, if any.
         *
         * The function object is expected to copy the string.
         */
        template<typename F>
        void as_text(F retrieval_func)
        {
            assert(counter < column_count);
            auto result = sqlite3_column_text(stmt, counter);
            retrieval_func(result, sqlite3_column_bytes(stmt, counter));
            ++counter;
        }

        /**
         * Reads the current field as UTF-16 encoded text, done by invoking the
         * function object argument with two arguments:
         *   * the current field in the result row as a const unsigned void*
         *   * the number of bytes of the current field in the result row,
         *     excluding the terminating null character
         * Advances to the next field, if any.
         *
         * The function object is expected to copy the string.
         */
        template<typename F>
        void as_text16(F retrieval_func)
        {
            assert(counter < column_count);
            auto result = sqlite3_column_text16(stmt, counter);
            retrieval_func(result, sqlite3_column_bytes16(stmt, counter));
            ++counter;
        }

        /**
         * Reads the current field as a BLOB, done by invoking the function
         * object argument with two arguments:
         *   * the current field in the result row as a const unsigned void*
         *   * the number of bytes of the current field in the result row
         * Advances to the next field, if any.
         *
         * The function object is expected to copy the bytes of the BLOB.
         */
        template<typename F>
        void as_blob(F retrieval_func)
        {
            assert(counter < column_count);
            auto result = sqlite3_column_blob(stmt, counter);
            retrieval_func(result, sqlite3_column_bytes(stmt, counter));
            ++counter;
        }

        /**
         * Steps through the result set to advance to the next result row.
         *
         * Must be called before reading any fields in the result row.
         * Returns true if there is a result row to read from.
         */
        bool step();
    private:
        sqlite3_stmt* stmt;   // prepared statement handle
        int counter = 0;      // field counter for each result row
        int column_count = 0; // number of columns in the result set

        explicit Result(sqlite3_stmt* stmt) : stmt(stmt) {}

        friend Result Statement::execute_query();
    };

    /**
     * Models a database transaction.
     */
    class Transaction
    {
    public:
        Transaction() = delete;
        Transaction(const Transaction& other) = delete;
        void operator=(const Transaction& other) = delete;
        void operator=(const Transaction&& other) = delete;

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
        virtual const char* what() const noexcept
        {
            return what_arg->c_str();
        }

        /**
         * Returns the error code.
         */
        int code() const noexcept
        {
            return error_code;
        }
    private:
        std::shared_ptr<std::string> what_arg; // full error message
        int error_code; // error code from SQLite
    };
}

#endif
