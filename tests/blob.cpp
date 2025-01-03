/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Blob
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
#include "catch.hpp"

SCENARIO("incremental I/O can be used for blobs")
{
    GIVEN("a database connection with a table created and populated")
    {
        sqlitemm::Connection conn(":memory:");
        REQUIRE_NOTHROW(conn.execute("CREATE TABLE notes (id INTEGER PRIMARY KEY, content BLOB);"));
        REQUIRE_NOTHROW(conn.execute("INSERT INTO notes (id, content) VALUES (1, NULL);"));

        WHEN("some text is written to the blob field")
        {
            REQUIRE_NOTHROW(conn.execute("UPDATE notes SET content = 'test content' WHERE id = 1;"));

            THEN("the same text can be read using incremental I/O")
            {
                auto blob = conn.open_blob("main", "notes", "content", 1, sqlitemm::Blob::READ_ONLY);
                std::string value(blob.size(), '\0');
                blob.read(&value[0], blob.size(), 0);
                REQUIRE(value == "test content");
            }

            THEN("the same text can be read using incremental I/O at an offset")
            {
                auto blob = conn.open_blob("main", "notes", "content", 1, sqlitemm::Blob::READ_ONLY);
                std::string value(blob.size() - 5, '\0');
                blob.read(&value[0], blob.size() - 5, 5);
                REQUIRE(value == "content");
            }
        }

        WHEN("the blob field is set to a zero-filled blob value")
        {
            REQUIRE_NOTHROW(conn.execute("UPDATE notes SET content = zeroblob(12) WHERE id = 1;"));

            THEN("the blob value can be written using incremental I/O")
            {
                auto blob = conn.open_blob("main", "notes", "content", 1, sqlitemm::Blob::READ_WRITE);
                std::string expected_value = "test content";
                blob.write(&expected_value[0], blob.size(), 0);
                blob.close();

                AND_THEN("the same text can be read using incremental I/O")
                {
                    blob = conn.open_blob("main", "notes", "content", 1, sqlitemm::Blob::READ_ONLY);
                    std::string value(blob.size(), '\0');
                    blob.read(&value[0], blob.size(), 0);
                    REQUIRE(value == expected_value);
                }
            }
        }

        WHEN("there is another row in the table")
        {
            REQUIRE_NOTHROW(conn.execute("INSERT INTO notes (id, content) VALUES (2, 'different test content');"));
            REQUIRE_NOTHROW(conn.execute("UPDATE notes SET content = 'test content' WHERE id = 1;"));

            auto blob = conn.open_blob("main", "notes", "content", 1, sqlitemm::Blob::READ_ONLY);

            THEN("the blob can be reopened to the other row")
            {
                blob.reopen(2);
                std::string value(blob.size(), '\0');
                blob.read(&value[0], blob.size(), 0);
                REQUIRE(value == "different test content");
            }
        }
    }
}
