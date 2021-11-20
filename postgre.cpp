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

#define __DDB_POSTGRE__
#include "directdb.hpp"

using namespace std;

namespace ddb {

void
PQNoticeProcessor(void*, const char* message)
{
    CS_PRINT_NOTE(message);
}

// -------------------------------------------------------------------------------------------------
Postgre::Postgre()
/*!
  Constructs database object for the connection to the PostgreSQL databases.
  In Windows environment this intitializes sockets. On Linux the only the
  initialization flag is simply set to true.
*/
{
    feat_support |= FEATURE_TRANSACTIONS;
    feat_support |= FEATURE_AUTOTRIM;
    feat_on |= FEATURE_TRANSACTIONS;
    feat_on |= FEATURE_AUTOTRIM;
    flags |= FLAG_INITIALIZED;
    connection = 0;
}

// -------------------------------------------------------------------------------------------------
Postgre::~Postgre()
/*!
    Closes up the database connection and on Windows closes socket services.
*/
{
    if (flags & FLAG_CONNECTED)
        Disconnect();
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::Connect(const char* constr)
{
    // See section 30.1 in PostgreSQL manual
    // hostaddr=10.1.1.3 dbname=menacon user=app password=whatever connect_timeout=10
    // host=www.menacon.fi sslmode=require sslcert=client.crt sslkey=client.key
    if (!constr) {
        SetErrorId(3);
        return false;
    }
    connection = PQconnectdb(constr);

    // check to see that the backend connection was successfully made
    if (PQstatus(connection) == CONNECTION_BAD) {
        PQfinish(connection);
        connection = 0;
        SetErrorId(4);
        return false;
    }
    flags |= FLAG_CONNECTED;
    CS_VAPRT_INFO("Postgre client encoding id=%d", PQclientEncoding(connection));
    PQsetNoticeProcessor(connection, &PQNoticeProcessor, 0);
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::Disconnect()
{
    if (connection)
        PQfinish(connection);
    flags &= ~FLAG_CONNECTED;
    return true;
}
// -------------------------------------------------------------------------------------------------
bool
Postgre::IsConnectOK()
{
    if (PQstatus(connection) == CONNECTION_OK)
        return true;
    return false;
}
// -------------------------------------------------------------------------------------------------
bool
Postgre::ResetConnection()
{
    PQreset(connection);
    return IsConnectOK();
}
// -------------------------------------------------------------------------------------------------
RowSet*
Postgre::CreateRowSet()
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return 0;
    }
    return new PostgreRowSet(this);
}
bool
Postgre::CreateRowSet(RSInterface* cif)
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return false;
    }
    PostgreRowSet* rs = new PostgreRowSet(this);
    cif->PostCreate(rs);
    return true;
}

// -------------------------------------------------------------------------------------------------
string
Postgre::GetErrorDescription(RowSet*)
{
    string errorMsg;

    errorMsg = GetLastError();
    if (connection) {
        errorMsg += "\n";
        errorMsg += PQerrorMessage(connection);
    }
    return errorMsg;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::StartTransaction()
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return false;
    }
    if (flags & FLAG_TRANSACT_ON) {
        SetErrorId(6);
        return false;
    }

    PGresult* result = PQexec(connection, "BEGIN");
    PQclear(result);

    flags |= FLAG_TRANSACT_ON;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::Commit()
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return false;
    }
    if (!(flags & FLAG_TRANSACT_ON)) {
        SetErrorId(7);
        return false;
    }

    PGresult* result = PQexec(connection, "COMMIT");
    PQclear(result);

    flags &= ~FLAG_TRANSACT_ON;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::RollBack()
{
    if (!(flags & FLAG_CONNECTED)) {
        SetErrorId(5);
        return false;
    }
    if (!(flags & FLAG_TRANSACT_ON)) {
        SetErrorId(7);
        return false;
    }

    PGresult* result = PQexec(connection, "ROLLBACK");
    PQclear(result);

    flags &= ~FLAG_TRANSACT_ON;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::ExecuteIntFunction(const string& query, int& val)
{
    if (query.length() == 0)
        return false;
    PGresult* result = PQexec(connection, query.c_str());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("Postgre::ExecuteIntFunction failed");
        errorId = 19;
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if (PQntuples(result) == 0 || PQgetisnull(result, 0, 0)) {
        PQclear(result);
        return false;
    }
    val = (uint32_t)strtol(PQgetvalue(result, 0, 0), 0, 0);
    PQclear(result);
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::ExecuteLongFunction(const string& query, long& val)
{
    if (query.length() == 0)
        return false;
    PGresult* result = PQexec(connection, query.c_str());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("Postgre::ExecuteLongFunction failed");
        errorId = 19;
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if (PQntuples(result) == 0 || PQgetisnull(result, 0, 0)) {
        PQclear(result);
        return false;
    }
    val = (uint64_t)strtol(PQgetvalue(result, 0, 0), 0, 0);
    PQclear(result);
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::ExecuteDoubleFunction(const string& query, double& val)
{
    if (query.length() == 0)
        return false;
    PGresult* result = PQexec(connection, query.c_str());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("Postgre::ExecuteDoubleFunction failed");
        errorId = 19;
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if (PQntuples(result) == 0 || PQgetisnull(result, 0, 0)) {
        PQclear(result);
        return false;
    }
    char* resultStr = PQgetvalue(result, 0, 0);
    if (resultStr) {
        val = strtod(resultStr, 0);
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::ExecuteBoolFunction(const string& query, bool& val)
{
    if (query.length() == 0)
        return false;
    PGresult* result = PQexec(connection, query.c_str());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("Postgre::ExecuteBoolFunction failed");
        errorId = 19;
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if (PQntuples(result) == 0 || PQgetisnull(result, 0, 0)) {
        PQclear(result);
        return false;
    }
    char* resultStr = PQgetvalue(result, 0, 0);
    if (resultStr) {
        val = resultStr[0] == 't' ? true : false;
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::ExecuteStrFunction(const string& query, string& answer)
{
    if (query.length() == 0)
        return false;
    PGresult* result = PQexec(connection, query.c_str());
    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("Postgre::ExecuteStrFunction failed.");
        errorId = 19;
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if (PQntuples(result) == 0 || PQgetisnull(result, 0, 0)) {
        PQclear(result);
        return false;
    }
    errorId = 0;
    char* resultStr = PQgetvalue(result, 0, 0);
    if (resultStr) {
        answer = resultStr;
        if ((feat_on & FEATURE_AUTOTRIM) > 0) {
            Database::TrimTail(&answer);
        }
        PQclear(result);
        return true;
    }
    answer.clear();
    PQclear(result);
    return false;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::ExecuteDateFunction(const string& query, tm& val)
{
    tm* tmPtr;
    if (query.length() == 0)
        return false;
    PGresult* result = PQexec(connection, query.c_str());

    if (!result || PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_PRINT_ERRO("Postgre::ExecuteDateFunction failed");
        errorId = 19;
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    if (PQntuples(result) == 0 || PQgetisnull(result, 0, 0)) {
        PQclear(result);
        return false;
    }
    char* resultStr = PQgetvalue(result, 0, 0);
    if (resultStr) {
        tmPtr = &val;
        Database::ExtractTimestamp(resultStr, tmPtr);
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

// -------------------------------------------------------------------------------------------------
int
Postgre::ExecuteModify(const string& modify)
{
    if (modify.length() == 0)
        return -1;
    PGresult* result = PQexec(connection, modify.c_str());
    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK) {
        CS_VAPRT_ERRO("Postgre::ExecuteModify - Failed. PQStatus=%d", PQresultStatus(result));
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return -1;
    }

    char* resultStr = PQcmdTuples(result);
    int retval = strtol(resultStr, 0, 10);
    PQclear(result);
    return retval;
}

// -------------------------------------------------------------------------------------------------
bool
Postgre::UpdateStructure(const string& command)
{
    if (command.length() == 0)
        return false;
    PGresult* result = PQexec(connection, command.c_str());

    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK) {
        CS_PRINT_ERRO("Postgre::UpdateStructure failed");
        if (result) {
            CS_PRINT_ERRO(PQresultErrorMessage(result));
            PQclear(result);
        }
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------------------------------
unsigned long
Postgre::GetInsertId()
{
    PGresult* result = PQexec(connection, "SELECT lastval()");
    if (!result) {
        CS_PRINT_WARN("Postgre::GetInsertId - unable to get result.");
        return 0;
    }
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        CS_VAPRT_ERRO("Postgre::GetInsertId failure:%s", PQresultErrorMessage(result));
        PQclear(result);
        return 0;
    }
    char* endptr;
    char* resultStr = PQgetvalue(result, 0, 0);
    errno = 0;
    unsigned long rv = strtoul(resultStr, &endptr, 10);
    if (errno)
        return 0;
    return rv;
}

// ==========================================================================================
// $$$$ ADMIN COMMANDS $$$
// ------------------------------------------------------------------------------------------
bool
Postgre::CreateUser(const string& uid, const string& pwd)
{
    char query[100];
    int oid;
    sprintf(query, "SELECT oid FROM pg_roles WHERE rolname='%s'", uid.c_str());
    if (!ExecuteIntFunction(query, oid)) {
        if (pwd.empty())
            sprintf(query, "CREATE ROLE %s CREATEROLE LOGIN", uid.c_str());
        else
            sprintf(query, "CREATE ROLE %s CREATEROLE LOGIN ENCRYPTED PASSWORD '%s'", uid.c_str(),
                    pwd.c_str());
        if (!UpdateStructure(query)) {
            CS_VAPRT_ERRO("Unable to create non-existing user '%s'", uid.c_str());
            return false;
        }
        CS_VAPRT_INFO("Postgre::CreateUser - created role %s", uid.c_str());
    }
    return true;
}
// ------------------------------------------------------------------------------------------
bool
Postgre::CreateDatabase(const string& dbname, const string& owner)
{
    string rsname;
    // Check the existence first.
    RowSet* rs = CreateRowSet();
    rs->Bind(DT::STR, &rsname);
    rs->query << "SELECT datname FROM pg_database";
    rs->Query();
    while (rs->GetNext()) {
        if (!rsname.compare(dbname))
            return true;
    }
    // Create database
    rs->query.str("");
    rs->query << "CREATE DATABASE " << dbname << " OWNER=" << owner;
    if (!UpdateStructure(rs->query.str())) {
        CS_VAPRT_ERRO("Unable to create %s database.", dbname.c_str());
        return false;
    }
    CS_VAPRT_INFO("Database %s created.", dbname.c_str());
    return true;
}

// ------------------------------------------------------------------------------------------
bool
Postgre::CreateTable(const string& name, const string& sql)
{
    string rstbl;
    ostringstream qry;
    qry << "SELECT relname FROM pg_class WHERE relname='" << name << "' AND relkind='r'";
    if (!ExecuteStrFunction(qry.str(), rstbl)) {
        if (errorId > 0)
            return false;
        if (!UpdateStructure(sql))
            return false;
    }
    return true;
}

}; // namespace ddb
