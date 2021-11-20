/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <cpp4scripts.hpp>

#define __DDB_SQLITE3__
#include "directdb.hpp"

using namespace std;

namespace ddb {

// -------------------------------------------------------------------------------------------------
Sqlite::Sqlite()
/*!
  Constructs database object for the connection to the Sqlite databases.
*/
{
    feat_support |= FEATURE_AUTOTRIM;
    feat_on |= FEATURE_AUTOTRIM;
    flags |= FLAG_INITIALIZED;
    connection = 0;
    errval = 0;
}

// -------------------------------------------------------------------------------------------------
Sqlite::~Sqlite()
/*!
    Closes up the database connection and on Windows closes socket services.
*/
{
    if (flags & FLAG_CONNECTED)
        Disconnect();
}

// -------------------------------------------------------------------------------------------------
bool
Sqlite::Connect(const char* filename)
{
    if (!filename) {
        SetErrorId(3);
        return false;
    }

    errval = sqlite3_open_v2(filename, &connection, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    if (errval != SQLITE_OK) {
        sqlite3_close_v2(connection);
        connection = 0;
        SetErrorId(4);
        return false;
    }
    flags |= FLAG_CONNECTED;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Sqlite::Disconnect()
{
    if (connection)
        sqlite3_close_v2(connection);
    flags &= ~FLAG_CONNECTED;
    return true;
}
// -------------------------------------------------------------------------------------------------
RowSet*
Sqlite::CreateRowSet()
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return 0;
    }
    return new SqliteRowSet(this);
}
bool
Sqlite::CreateRowSet(RSInterface* cif)
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return false;
    }
    SqliteRowSet* rs = new SqliteRowSet(this);
    cif->PostCreate(rs);
    return true;
}

// -------------------------------------------------------------------------------------------------
string
Sqlite::GetErrorDescription(RowSet*)
{
    string errorMsg;

    errorMsg = GetLastError();
    if (connection) {
        errorMsg += "\n";
        errorMsg += sqlite3_errmsg(connection);
    }
    return errorMsg;
}

// -------------------------------------------------------------------------------------------------
int
ExecIntCb(void* valptr, int cols, char** value, char** /*col name*/)
{
    Sqlite::ExecData* data = (Sqlite::ExecData*)valptr;
    int*& val = (int*&)data->value;
    if (cols) {
        *val = (int)strtol(value[0], 0, 0);
        data->rows++;
    }
    return 0;
}
bool
Sqlite::ExecuteIntFunction(const string& query, int& val)
{
    char* errmsg;
    ExecData data(&val);

    if (query.length() == 0)
        return false;
    if (sqlite3_exec(connection, query.c_str(), &ExecIntCb, &data, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteIntFunction - %s", errmsg);
        errorId = 19;
        return false;
    }
    if (data.rows == 0)
        return false;
    return true;
}
// -------------------------------------------------------------------------------------------------
int
ExecLongCb(void* valptr, int cols, char** value, char** /*col name*/)
{
    Sqlite::ExecData* data = (Sqlite::ExecData*)valptr;
    long*& val = (long*&)data->value;
    if (cols) {
        *val = (long)strtol(value[0], 0, 0);
        data->rows++;
    }
    return 0;
}
bool
Sqlite::ExecuteLongFunction(const string& query, long& val)
{
    char* errmsg;
    ExecData data(&val);
    if (query.length() == 0)
        return false;
    if (sqlite3_exec(connection, query.c_str(), &ExecLongCb, &data, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteLongFunction - %s", errmsg);
        errorId = 19;
        return false;
    }
    if (data.rows == 0)
        return false;
    return true;
}
// -------------------------------------------------------------------------------------------------
int
ExecDoubleCb(void* valptr, int cols, char** value, char** /*col name*/)
{
    Sqlite::ExecData* data = (Sqlite::ExecData*)valptr;
    double*& val = (double*&)data->value;
    if (cols) {
        *val = (double)strtol(value[0], 0, 0);
        data->rows++;
    }
    return 0;
}
bool
Sqlite::ExecuteDoubleFunction(const string& query, double& val)
{
    char* errmsg;
    ExecData data(&val);
    if (query.length() == 0)
        return false;
    if (sqlite3_exec(connection, query.c_str(), &ExecDoubleCb, &val, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteDoubleFunction - %s", errmsg);
        errorId = 19;
        return false;
    }
    if (data.rows == 0)
        return false;
    return true;
}
// -------------------------------------------------------------------------------------------------
int
ExecBoolCb(void* valptr, int cols, char** value, char** /*col name*/)
{
    Sqlite::ExecData* data = (Sqlite::ExecData*)valptr;
    int* val = (int*)data->value;
    if (cols) {
        *val = (int)strtol(value[0], 0, 0);
        data->rows++;
    }
    return 0;
}
bool
Sqlite::ExecuteBoolFunction(const string& query, bool& val)
{
    int sqlite_bool = 0;
    char* errmsg;
    ExecData data(&sqlite_bool);

    if (query.length() == 0)
        return false;
    if (sqlite3_exec(connection, query.c_str(), &ExecBoolCb, &data, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteBoolFunction - %s", errmsg);
        errorId = 19;
        return false;
    }
    if (data.rows == 0)
        return false;
    val = sqlite_bool ? true : false;
    return true;
}

// -------------------------------------------------------------------------------------------------
int
ExecStringCb(void* valptr, int cols, char** value, char** /*col name*/)
{
    Sqlite::ExecData* data = (Sqlite::ExecData*)valptr;
    string*& val = (string*&)data->value;
    if (cols) {
        *val = value[0];
        data->rows++;
    }
    return 0;
}
bool
Sqlite::ExecuteStrFunction(const string& query, string& answer)
{
    char* errmsg;
    ExecData data(&answer);

    if (query.length() == 0)
        return false;
    if (sqlite3_exec(connection, query.c_str(), &ExecStringCb, &data, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteStrFunction - %s", errmsg);
        errorId = 19;
        return false;
    }
    if (data.rows == 0)
        return false;
    if ((feat_on & FEATURE_AUTOTRIM) > 0) {
        Database::TrimTail(&answer);
    }
    return true;
}
// -------------------------------------------------------------------------------------------------
int
ExecDateTimeCb(void* valptr, int cols, char** value, char** /*col name*/)
{
    Sqlite::ExecData* data = (Sqlite::ExecData*)valptr;
    tm*& val = (tm*&)data->value;
    if (!cols)
        return 0;
    data->rows++;
    Database::ExtractTimestamp(value[0], val);
    return 0;
}
bool
Sqlite::ExecuteDateFunction(const string& query, tm& val)
{
    char* errmsg;
    ExecData data(&val);

    if (query.length() == 0)
        return false;
    if (sqlite3_exec(connection, query.c_str(), &ExecDateTimeCb, &data, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteDateFunction - %s", errmsg);
        errorId = 19;
        return false;
    }
    if (data.rows == 0)
        return false;
    return true;
}
// -------------------------------------------------------------------------------------------------
int
Sqlite::ExecuteModify(const string& modify)
{
    char* errmsg;
    if (modify.length() == 0)
        return -1;
    if (sqlite3_exec(connection, modify.c_str(), 0, 0, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::ExecuteModify - %s", errmsg);
        errorId = 18;
        return -1;
    }
    return sqlite3_changes(connection);
}
// -------------------------------------------------------------------------------------------------
bool
Sqlite::UpdateStructure(const string& command)
{
    char* errmsg;
    if (command.length() == 0)
        return -1;
    if (sqlite3_exec(connection, command.c_str(), 0, 0, &errmsg) != SQLITE_OK) {
        CS_VAPRT_ERRO("Sqlite::UpdateStructure - %s", errmsg);
        errorId = 21;
        return false;
    }
    return true;
}
// -------------------------------------------------------------------------------------------------
unsigned long
Sqlite::GetInsertId()
{
    return (unsigned long)sqlite3_last_insert_rowid(connection);
}
//------------------------------------------------------------------------------------------
bool
Sqlite::FindSchemaItem(ST stype, const char* name)
{
    int rowid;
    string query("SELECT rowid FROM sqlite_master WHERE type=");
    if (stype == ST::TABLE) {
        query += "'table' AND tbl_name='";
        query += name;
        query += "'";
    } else
        return false;
    if (!ExecuteIntFunction(query, rowid) || rowid == 0)
        return false;
    return true;
}

}; // namespace ddb
