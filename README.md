SQLitemm
========
SQLitemm is a C++ wrapper interface for SQLite's C API. It is a non-compatible rewrite of the 2005 SQLitemm project that aimed to provide "resource encapsulation and management while attempting to maintain minimal deviation from the original C interface", except that the latter goal has less emphasis, with support for newer SQLite3 features in a post-C++11 world being the new additional goals.

SQLitemm provides the following classes to wrap SQLite resource objects:
* `Connection`: a database connection (wraps `sqlite3`)
* `Statement`: a prepared statement (wraps `sqlite3_stmt`)

Additionally, SQLitemm provides the following classes to wrap SQLite concepts:
* `Result`: a result object that abstracts out the result retrieval aspects of `sqlite3_stmt`
* `Transaction`: a transaction object to automatically rollback when a C++ exception is thrown or propagated should the transaction be not yet committed or already rolled back

Installation
------------
The `sqlitemm.hpp` header file can be included in your project much like the `sqlite.h` header file. Analogous to the SQLite amalgamation for C projects, the `sqlitemm.cpp` source file can be compiled along with the C++ source files of your project.

`sqlite.h` and `sqlite3.c` (SQLite version 3.32.1) are provided along with this project for ease of automated testing, but you are free to use your own copy of the SQLite header and source.

Example Usage
-------------
If we imagine a one-off retrieval of some unspecified game results consisting of names and corresponding scores from a table where scores are greater than some threshold parameter:
```C++
std::vector<Result> retrieve_results(double threshold)
{
    std::vector<Result> results;
    try
    {
        sqlitemm::Connection connection(DATABASE_FILENAME);
        auto statement = connection.prepare("SELECT name, score FROM result WHERE score > :score");
        statement[":score"] = threshold;
        auto result = statement.execute_query();
        std::string name;
        double score;
        while (result.step())
        {
            result >> name >> score;
            results.emplace_back(name, score);
        }
    }
    catch (const sqlitemm::Error& e)
    {
        // Perhaps we just want to translate the exception:
        throw SomeException(e.what());
    }
    return results;
}
```

Legal
-----
Copyright &copy; 2020 Amanda Wee

SQLitemm is licensed under the Apache License, Version 2.0 (the "License"); you may not use the files of SQLitemm except in compliance with the License. You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
