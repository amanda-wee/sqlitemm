/************************************************************************************************************
 * SQLitemm tests source file primarily for testing sqlitemm::Error and its derived classes
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
#include "catch.hpp"

TEST_CASE("ConstraintError")
{
    sqlitemm::Connection conn(":memory:");
    REQUIRE_NOTHROW(conn.execute("CREATE TABLE person (id INTEGER PRIMARY KEY, name TEXT UNIQUE);"));

    conn.execute("INSERT INTO person (name) VALUES ('Alice');");
    REQUIRE_THROWS_AS(conn.execute("INSERT INTO person (name) VALUES ('Alice');"), sqlitemm::ConstraintError);
}
