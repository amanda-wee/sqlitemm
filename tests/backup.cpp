/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Backup
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

SCENARIO("a database can be backed up to another database")
{
    GIVEN("a database with a table created and populated, and another database")
    {
        const int num_rows = 100; // selected to populate the source database to have two pages
        sqlitemm::Connection source(":memory:");
        REQUIRE_NOTHROW(source.execute("CREATE TABLE notes (id INTEGER PRIMARY KEY, content TEXT);"));
        {
            auto stmt = source.prepare("INSERT INTO notes (id, content) VALUES (:id, :content);");
            for (int row_count = 0; row_count < num_rows; ++row_count)
            {
                stmt[":id"] = row_count + 1;
                stmt[":content"] = "sample";
                stmt.execute();
                stmt.reset();
            }
        }
        sqlitemm::Connection destination(":memory:");

        WHEN("a backup is initiated and a page is backed up")
        {
            auto backup = sqlitemm::Backup(source, "main", destination, "main");
            REQUIRE(backup.step(1));

            THEN("the page count and pages remaining can be determined")
            {
                REQUIRE(backup.page_count() == 2);
                REQUIRE(backup.pages_remaining() == 1);
            }

            WHEN("the backup is complete")
            {
                REQUIRE_FALSE(backup.step(-1));

                THEN("the backup page count does not change but there are no pages remaining")
                {
                    REQUIRE(backup.page_count() == 2);
                    REQUIRE(backup.pages_remaining() == 0);
                    backup.close();
                }

                THEN("the destination database has the same data as the source database")
                {
                    auto stmt = destination.prepare("SELECT id, content FROM notes;");
                    auto result = stmt.execute_query();
                    int row_count = 0;
                    while (result.step() && row_count <= num_rows)
                    {
                        REQUIRE(static_cast<int>(result[0]) == row_count + 1);
                        REQUIRE(static_cast<std::string>(result[1]) == "sample");
                        ++row_count;
                    }
                    REQUIRE(row_count == num_rows);
                }
            }
        }
    }
}
