SQLitemm
========
![C/C++ CI](https://github.com/amanda-wee/sqlitemm/workflows/C/C++%20CI/badge.svg)

SQLitemm is a C++ wrapper interface for [SQLite](https://www.sqlite.org/)'s C API. It provides a library of database resource objects and associated functionality to make it easy to write C++17 (and later) programs that rely on SQLite databases.

Features
--------
### Classes to wrap SQLite resource objects
* `Connection`: a database connection (wraps `sqlite3`)
* `Statement`: a prepared statement (wraps `sqlite3_stmt`)
* `Blob`: a blob object that enables incremental I/O with BLOBs (wraps `sqlite3_blob`)
* `Backup`: a backup object that enables support for online database backups (wraps `sqlite3_backup`)

### Classes to wrap SQLite concepts
* `Result`: a result object that abstracts out the result retrieval aspects of `sqlite3_stmt`
* `Transaction`: a transaction object to automatically rollback when a C++ exception is thrown or propagated should the transaction be not yet committed or already rolled back
* `Error`: an exception base class for SQLite error codes; derived classes are provided where they are likely to be useful to be handled separately

### Prepared statement parameter binding
* `statement << paramA << paramB;`: "stream" parameter values in sequence to bind them
* `statement[":paramA"] = paramA;`: bind parameters by name
* Bind `NULL` (as `nullptr`), `const char*`, `std::string`, `std::u16string`, and zero-filled blob parameters
* Bind arbitrary text and BLOB parameters through `TextValue` and `BlobValue` respectively
* Bind values of type `T` that may or may not be `NULL` by binding `std::optional<T>`
* Conveniently bind `std::tuple` objects having 1 to 5 elements inclusive into the same number of parameters

### Query result retrieval
* `ResultIterator`: an input iterator that allows for iterating over result rows into objects of arbitrary type as long as the type provides a constructor that processes a `Result` as a row, or the iterator is provided a callback function that converts a `Result` as a row into an object of the given type
* `result >> valueA >> valueB;`: "stream" the fields of a result row in sequence to their destination variables, implicitly performing type conversion, including conversion to `std::optional` for fields that might contain `NULL`
* `valueA = result[0];`: retrieve the fields of a result row by index, implicitly performing type conversion
* `valueA = result[0].as_optional<int>();`: convert the fields of a result row retrieved by index to `std::optional`, hence allowing for fields that might contain `NULL`
* Retrieve arbitrary UTF-8 text, UTF-16 text, and BLOB values by providing callback functions to perform the retrieval
* Conveniently retrieve 1 to 5 fields inclusive into `std::tuple` objects having the same number of elements

### Other SQLite features
* Create SQL functions (scalar, aggregate, and window)
* Create SQL collations
* Load SQLite extension libraries
* Use database operation interrupts
* Optional "strict typing" on a per-query basis, allowing for the prevention of SQLite automatic type conversions across the SQLite fundamental types when retrieving values
* Convenience functions for attaching and detaching databases

Installation
------------
The `sqlitemm.hpp` header file can be included in your project much like the `sqlite3.h` header file. Analogous to the SQLite amalgamation for C projects, the `sqlitemm.cpp` source file can be compiled along with the C++ source files of your project.

`sqlite3.h` and `sqlite3.c` (SQLite version 3.47.2, originally tested against version 3.32.1) are provided along with this project for ease of automated testing, but you are free to use your own copy of the SQLite header and source with other versions of SQLite.

### C++ Version
Due to the use of `std::optional` to handle binding parameters and retrieving fields that might contain `NULL`, SQLitemm must be compiled with respect to C++17 or later.

### Doxygen
If [Doxygen](https://www.doxygen.nl/) is available, it can be used to generate documentation by running `make docs`.

Example Usage
-------------
Suppose we want to retrieve some unspecified game results consisting of names and corresponding scores from a table where scores are greater than some threshold parameter, into a container of `GameResult` objects:
```cpp
std::vector<GameResult> retrieve_results(double threshold)
{
    auto connection = sqlitemm::Connection(DATABASE_FILENAME);
    auto statement = connection.prepare("SELECT name, score FROM result WHERE score > :score");
    statement[":score"] = threshold;
    auto result = statement.execute_query();
    return std::vector<GameResult>(
        result.begin<GameResult>([](auto& result_row) {
            return GameResult(result_row[0], result_row[1]);
        }),
        result.end()
    );
}
```
If there is a database-related error, an exception of type `sqlitemm::Error` will be thrown.

Additional usage examples can be found in the examples folder.

Background
----------
The SQLitemm of 2020 and later is a non-compatible rewrite of the 2005 SQLitemm project that aimed to provide "resource encapsulation and management while attempting to maintain minimal deviation from the original C interface". This rewrite has less concern about deviating from the original C interface, and makes extensive use of post-C++11 features such as the use of `std::optional` to model values that could contain `NULL`.

### Future work
* Augment `Transaction` to support SQLite savepoints
* Add miscellaneous functionality to the `sqlitemm` namespace
* Improve the automated test coverage
* Only forward declare the parts of the `sqlite3.h` header that are needed in `sqlitemm.h` rather than including the entire header

Legal
-----
Copyright &copy; 2025 Amanda Wee

SQLitemm is licensed under the Apache License, Version 2.0 (the "License"); you may not use the files of SQLitemm except in compliance with the License. You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
