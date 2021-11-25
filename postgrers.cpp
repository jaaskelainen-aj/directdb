/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include <fstream>
#include <string.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <cstdarg>
#include <cpp4scripts.hpp>

#define __DDB_POSTGRE__
#include "directdb.hpp"

using namespace std;

namespace ddb {

// -------------------------------------------------------------------------------------------------
PostgreRowSet::PostgreRowSet(Database* db_in)
  : RowSet()
/*!
    Initializes member variables to default values.
    \param db_in Pointer to database object.
*/
{
    max_rows = 0;
    result = 0;
    result_complete = true;

    db = (Postgre*)db_in;
}

// -------------------------------------------------------------------------------------------------
PostgreRowSet::~PostgreRowSet()
/*!
    Relases the PostGre result if it still exists.
*/
{
    if (result_complete == false)
        PQclear(result);
}

// -------------------------------------------------------------------------------------------------
bool
PostgreRowSet::Query()
{
    if (!fieldRoot) {
        CS_PRINT_NOTE("PostgreRowSet::Query - Query called without binding variables.");
        db->SetErrorId(9);
        return false;
    }
    if (query.tellp() < 1) {
        CS_PRINT_WARN("PostgreRowSet::Query - Empty query. Aborted.");
        return false;
    }
    if (result_complete == false)
        Reset();

    result = PQexec(db->GetPGConn(), query.str().c_str());
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_VAPRT_ERRO("PostgreRowSet::Query failed: %s", PQresultErrorMessage(result));
        db->SetErrorId(8);
        Reset();
        return false;
    }
    result_complete = false;
    max_rows = PQntuples(result);
    row_count = 0;
    return true;
}

// -------------------------------------------------------------------------------------------------
int
PostgreRowSet::GetNext()
{
    int maxFields, nField, count;
    BoundField* field;
    char* resultStr;

    if (result_complete == true)
        return 0;

    // If the result of the query was empty:
    if (!max_rows) {
        // Clear the result and return false.
        Reset();
        return 0;
    }
    bool trim = db->IsFeatureOn(FEATURE_AUTOTRIM);
    maxFields = PQnfields(result);
    field = fieldRoot;
    nField = 0;
    count = 0;
    while (field) {
        resultStr = PQgetvalue(result, row_count, nField);
        if (resultStr) {
            switch (field->type) {
            case DT::INT:
                if (resultStr[0] == '\0')
                    *(static_cast<int*>(field->data)) = 0;
                else {
                    *(static_cast<int*>(field->data)) = strtol(resultStr, 0, 10);
                    count++;
                }
                break;
            case DT::LONG:
                if (resultStr[0] == '\0')
                    *(static_cast<long*>(field->data)) = 0;
                else {
                    *(static_cast<long*>(field->data)) = strtol(resultStr, 0, 10);
                    count++;
                }
                break;
            case DT::STR:
                if (resultStr[0] == '\0')
                    static_cast<string*>(field->data)->clear();
                else {
                    *(static_cast<std::string*>(field->data)) = resultStr;
                    if (trim)
                        Database::TrimTail(static_cast<std::string*>(field->data));
                    count++;
                }
                break;
            case DT::BOOL:
                if (resultStr[0] == '\0')
                    *(static_cast<bool*>(field->data)) = 0;
                else {
                    *(static_cast<bool*>(field->data)) = resultStr[0] == 't' ? true : false;
                    count++;
                }
                break;

            case DT::TIME:
            case DT::DAY:
                if (resultStr[0] == '\0')
                    memset(field->data, 0, sizeof(tm));
                else {
                    Postgre::ExtractTimestamp(resultStr, (tm*)field->data);
                    count++;
                }
                break;

            case DT::NUM:
                if (resultStr[0] == '\0')
                    *(static_cast<double*>(field->data)) = 0;
                else {
                    if (db->IsCommaDecimal()) {
                        char* commaPoint = strchr(resultStr, '.');
                        if (commaPoint)
                            *commaPoint = ',';
                    }
                    *(static_cast<double*>(field->data)) = strtod(resultStr, 0);
                    count++;
                }
                break;
            case DT::CHR:
            case DT::BIT:
                *(static_cast<char*>(field->data)) = resultStr[0];
                count++;
                break;
            }
        }

        field = field->next;
        nField++;
        if (!field || nField == maxFields)
            break;
    }
    row_count++;
    if (row_count == max_rows) {
        PQclear(result);
        result_complete = true;
    }
    return count;
}

// -------------------------------------------------------------------------------------------------
void
PostgreRowSet::Reset()
{
    if (result_complete)
        return;
    PQclear(result);
    max_rows = 0;
    row_count = 0;
    result_complete = true;
}

}; // namespace ddb
