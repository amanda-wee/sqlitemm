SQLitemm
========
![C/C++ CI](https://github.com/amanda-wee/sqlitemm/workflows/C/C++%20CI/badge.svg)

SQLitemm is a C++ wrapper interface for SQLite's C API. It is a non-compatible rewrite of the 2005 SQLitemm project that aimed to provide "resource encapsulation and management while attempting to maintain minimal deviation from the original C interface", except that the latter goal has less emphasis, with support for newer SQLite3 features in a post-C++11 world being the new additional goals.

### Classes to wrap SQLite resource objects:
* `Connection`: a database connection (wraps `sqlite3`)
* `Statement`: a prepared statement (wraps `sqlite3_stmt`)

### Classes to wrap SQLite concepts:
* `Result`: a result object that abstracts out the result retrieval aspects of `sqlite3_stmt`
* `Transaction`: a transaction object to automatically rollback when a C++ exception is thrown or propagated should the transaction be not yet committed or already rolled back
* `Error`: an exception base class for SQLite error codes; derived classes are provided where they are likely to be useful to be handled separately

### Notable features:
* `ResultIterator`: an input iterator that allows for iterating over result rows into objects of arbitrary type as long as the type provides a constructor that processes a `Result` as a row
* `statement << paramA << paramB;`: "stream" parameter values in sequence to bind them
* `statement[":paramA"] = paramA;`: bind parameters by name
* Support for binding `NULL` (as `nullptr`), `const char*`, `std::string`, `std::u16string`, and zero-filled blob parameters
* Support for binding arbitrary text and BLOB parameters through `TextValue` and `BlobValue` respectively
* `result >> valueA >> valueB;`: "stream" the fields of a result row in sequence to their destination values, implicitly performing type conversion
* `valueA = result[0];`: implicitly convert the fields of a result row by index to the desired type
* `auto valueA = result[0].to_optional<int>();`: convert the fields of a result row to `std::optional`, hence allowing for fields that might contain `NULL` (also available for "streaming" the fields of a result row)
* Support for retrieving arbitrary UTF-8 text, UTF-16 text, and BLOB values by providing function objects to perform the retrieval
* Optional "strict typing" on a per-query basis, allowing for the prevention of SQLite automatic type conversions across the SQLite fundamental types when retrieving values

### Future work:
* Support for creating SQL functions
* Support for BLOBs with incremental I/O
* `Savepoint` class along the same lines as the `Transaction` class
* Support for online database backups
* Adding other functionality associated with `sqlite3` to `Connection`
* Adding miscellaneous functionality to the `sqlitemm` namespace
* Only forward declare the parts of the `sqlite3.h` header that are needed in `sqlitemm.h` rather than including the entire header

Installation
------------
The `sqlitemm.hpp` header file can be included in your project much like the `sqlite3.h` header file. Analogous to the SQLite amalgamation for C projects, the `sqlitemm.cpp` source file can be compiled along with the C++ source files of your project.

`sqlite3.h` and `sqlite3.c` (SQLite version 3.47.2, originally tested against version 3.32.1) are provided along with this project for ease of automated testing, but you are free to use your own copy of the SQLite header and source with other versions of SQLite.

### C++ Version
Due to the use of `std::optional` to handle retrieving fields that might contain `NULL`, SQLitemm must be compiled with respect to C++17 or later.

Example Usage
-------------
If we imagine a one-off retrieval of some unspecified game results consisting of names and corresponding scores from a table where scores are greater than some threshold parameter:
```C++
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

Legal
-----
Copyright &copy; 2020 Amanda Wee

SQLitemm is licensed under the Apache License, Version 2.0 (the "License"); you may not use the files of SQLitemm except in compliance with the License. You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
