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

#define __DDB_SQLITE3__
#include "directdb.hpp"

using namespace std;

namespace ddb {

// -------------------------------------------------------------------------------------------------
SqliteRowSet::SqliteRowSet(Sqlite* db_in)
  : RowSet()
  , db(db_in)
/*!
    Initializes member variables to default values.
    \param db_in Pointer to database object.
*/
{
    stmt = 0;
    result_complete = true;
}

// -------------------------------------------------------------------------------------------------
SqliteRowSet::~SqliteRowSet()
/*!
    Relases the PostGre result if it still exists.
*/
{
    if (stmt)
        sqlite3_finalize(stmt);
}

// -------------------------------------------------------------------------------------------------
bool
SqliteRowSet::Query()
{
    if (!fieldRoot) {
        CS_PRINT_NOTE("SqliteRowSet::Query - Query called without binding variables.");
        db->SetErrorId(9);
        return false;
    }
    if (query.tellp() < 1) {
        CS_PRINT_WARN("SqliteRowSet::Query - Empty query string. Aborted.");
        return false;
    }
    if (!result_complete)
        Reset();
    int rv = sqlite3_prepare_v3(db->GetConnection(), /* Database handle */
                                query.str().c_str(), /* SQL statement, UTF-8 encoded */
                                query.tellp(),       /* Maximum length of zSql in bytes. */
                                SQLITE_PREPARE_PERSISTENT, &stmt, /* OUT: Statement handle */
                                0 /* OUT: Pointer to unused portion of zSql */
    );
    if (rv != SQLITE_OK) {
        CS_VAPRT_ERRO("SqliteRowSet::Query - prepare failed: %s", sqlite3_errstr(rv));
        stmt = 0;
        return false;
    }
    result_complete = false;
    row_count = 0;
    return true;
}
// -------------------------------------------------------------------------------------------------
int
SqliteRowSet::GetNext()
{
    int nField, col_type;
    BoundField* field;

    if (result_complete == true)
        return 0;

    int rv = sqlite3_step(stmt);
    if (rv == SQLITE_DONE) {
        sqlite3_reset(stmt);
        result_complete = true;
        return 0;
    }
    if (rv == SQLITE_BUSY) {
        CS_PRINT_WARN("SqliteRowSet::GetNext - BUSY");
        return 0;
    }
    if (rv != SQLITE_ROW) {
        sqlite3_reset(stmt);
        result_complete = true;
        CS_VAPRT_ERRO("SqliteRowSet::GetNext - %s", sqlite3_errstr(rv));
        return 0;
    }
    row_count++;
    bool trim = db->IsFeatureOn(FEATURE_AUTOTRIM);
    nField = 0;
    for (field = fieldRoot; field; field = field->next) {
        col_type = sqlite3_column_type(stmt, nField);
        switch (field->type) {
        case DT::INT:
            if (col_type == SQLITE_NULL)
                *(static_cast<int*>(field->data)) = 0;
            else
                *(static_cast<int*>(field->data)) = sqlite3_column_int(stmt, nField);
            break;

        case DT::LONG:
            if (col_type == SQLITE_NULL)
                *(static_cast<int*>(field->data)) = 0;
            else
                *(static_cast<int*>(field->data)) = sqlite3_column_int64(stmt, nField);
            break;

        case DT::STR:
            if (col_type == SQLITE_NULL)
                static_cast<string*>(field->data)->clear();
            else {
                *(static_cast<string*>(field->data)) =
                    (const char*)sqlite3_column_text(stmt, nField);
                if (trim)
                    Database::TrimTail(static_cast<std::string*>(field->data));
            }
            break;

        case DT::BOOL:
            if (col_type == SQLITE_NULL)
                *(static_cast<bool*>(field->data)) = false;
            else
                *(static_cast<bool*>(field->data)) =
                    sqlite3_column_int(stmt, nField) == 1 ? true : false;
            break;

        case DT::TIME:
        case DT::DAY:
            if (col_type == SQLITE_NULL)
                memset(field->data, 0, sizeof(tm));
            else
                Sqlite::ExtractTimestamp((const char*)sqlite3_column_text(stmt, nField),
                                         (tm*)field->data);
            break;

        case DT::NUM:
            if (col_type == SQLITE_NULL)
                *(static_cast<double*>(field->data)) = 0;
            else
                *(static_cast<double*>(field->data)) = sqlite3_column_double(stmt, nField);
            break;

        case DT::CHR:
        case DT::BIT:
            if (col_type == SQLITE_NULL)
                *(static_cast<char*>(field->data)) = 0;
            else
                *(static_cast<char*>(field->data)) = *sqlite3_column_text(stmt, nField);

            break;
        }
        nField++;
    }
    return nField;
}

// -------------------------------------------------------------------------------------------------
void
SqliteRowSet::Reset()
{
    if (result_complete)
        return;
    if (stmt) {
        sqlite3_finalize(stmt);
        stmt = 0;
    }
    result_complete = false;
    row_count = 0;
}

}; // namespace ddb
