/************************************************************************************************************
 * SQLitemm tests source file for defining extern "C" callback functions
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

#include "sqlite3.h"

/* SQL scalar function */

/* Increment the argument by 1 */
void sqlitemm_inc(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    int arg_value = sqlite3_value_int(argv[0]);
    sqlite3_result_int(context, arg_value + 1);
}

/* SQL aggregate function */

typedef struct sqlitemmSumContext
{
    int sum;
} sqlitemmSumContext;

void sqlitemm_sum_step(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    sqlitemmSumContext* sum_context = sqlite3_aggregate_context(context, sizeof(*sum_context));
    if (sum_context)
    {
        sum_context->sum += sqlite3_value_int(argv[0]);
    }
}

void sqlitemm_sum_final(sqlite3_context* context)
{
    sqlitemmSumContext* sum_context = sqlite3_aggregate_context(context, 0);
    sqlite3_result_int(context, sum_context ? sum_context->sum : 0);
}

/* SQL collation */

int sqlitemm_reverse_nocase(void* app_user_data, int lhs_num_bytes, const void* lhs, int rhs_num_bytes, const void* rhs)
{
    int n = (lhs_num_bytes < rhs_num_bytes) ? lhs_num_bytes : rhs_num_bytes;
    int result = -sqlite3_strnicmp(lhs, rhs, n);
    if (result != 0)
    {
        return result;
    }
    if (lhs_num_bytes == rhs_num_bytes)
    {
        return 0;
    }
    return (lhs_num_bytes < rhs_num_bytes) ? 1 : -1;
}
