/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <fstream>
#include <sstream>
#include <cpp4scripts.hpp>

#include "directdb.hpp"

using namespace std;
using namespace c4s;

namespace ddb {
// -------------------------------------------------------------------------------------------------
Database::Database()
/*!
  This constructor just initializes all strings to zero values. Initialize function
  should be called before attempting to connect into the database.
*/
{
    feat_support = 0;
    feat_on = 0;
    errorId = 0;
    flags = 0;
    port = 0;

    /* Depending on the client's I18N settings the numeric values use period or comma
       as decimal separator. By default databases use the period.
     */
    struct lconv* ldata = localeconv();
    if (*ldata->decimal_point == ',') {
        CS_PRINT_WARN("Database::Database - Current locale has , as decimal separator!");
        commaDecimal = true;
    } else
        commaDecimal = false;

    scratch_size = 0x200;
    scratch_buffer = new char[scratch_size];
}
// -------------------------------------------------------------------------------------------------
Database::~Database()
/*!
  Virtual destructor allows c++-system to call inherited destructor. This destructor
  has no code currently.
*/
{
    if (scratch_buffer)
        delete[] scratch_buffer;
}
// -------------------------------------------------------------------------------------------------
string
Database::GetLastError()
{
#define MAX_ERRORS 27
    const char* errorStr[MAX_ERRORS] = {
        /* 000 */ "Success",
        /* 001 */ "Undefined error number",
        /* 002 */ "Version 2.0 of Win socket was not found.",
        /* 003 */ "DB - Connect: empty or incorrect connections string.",
        /* 004 */ "DB - Connect: Connection failure. Check the initialization parameters.",
        /* 005 */ "DB - Attempt to use member functions without a connection to the database.",
        /* 006 */ "DB - Transaction start: Transaction is already on.",
        /* 007 */ "DB - Commit/RollBack: The transaction has not been started.",
        /* 008 */ "Rowset - Query function unsuccesfull.",
        /* 009 */ "Rowset - Query called without bound variables.",
        /* 010 */ "Unable to initialize Microsoft connection communication layer.",
        /* 011 */
        "DB - Connect: Attempt to connect when database has not been initialized successfully.",
        /* 012 */ "Microsoft - MSBind called without calling Query first.",
        /* 013 */ "Microsoft - bind function did not support desired conversion.",
        /* 014 */ "Rowset - Requested data conversion is not supported.",
        /* 015 */ "Rowset - Insert execution failure.",
        /* 016 */ "Postgresql - Fatal error.",
        /* 017 */ "Rowset - Too little memory allocated for the Insert data.",
        /* 018 */ "DB - Modify (INSERT, UPDATE or DELETE) function was unsuccesful.",
        /* 019 */ "DB - Execute query function was unsuccesful.",
        /* 020 */ "Rowset - GetNext function was unsuccesful.",
        /* 021 */
        "DB - Update structure (CREATE, DROP, ALTER TABLE or VIEW) command was unsuccessful.",
        /* 022 */
        "DB - GetInsertId failed. Operation not supported or last statement was not an INSERT "
        "command.",
        /* 023 */ "DB - Initialization failure.",
        /* 024 */ "DB - Transactions not supported.",
        /* 025 */ "DB - Transaction failed.",
        /* 026 */ "Sqlite - SQL/Transaction busy."
    };
    string str;
    if (errorId >= MAX_ERRORS)
        str = errorStr[1];
    str = errorStr[errorId];
    return str;
}

// -------------------------------------------------------------------------------------------------
const char*
Database::CleanStr(const char* str)
{
    size_t max = strlen(str) + 64;
    reallocateScratch(max);
    char* to = scratch_buffer;
    char* end = scratch_buffer + max;
    while (to < end && *str) {
        if (*str == '\'') {
            *to++ = '\'';
            *to++ = '\'';
        } else if (*str != '\r') // skip the carriage return
            *to++ = *str;
        str++;
    }
    *to = 0;
    return scratch_buffer;
}
string
Database::CleanStr(const std::string& str)
{
    string result;
    string::const_iterator chi, end;

    chi = str.begin();
    end = str.end();
    while (chi != end) {
        if (*chi == '\'') {
            result += "''";
        } else if (*chi != '\r') // skip the carriage return
            result += *chi;
        chi++;
    }
    return result;
}
// -------------------------------------------------------------------------------------------------
const char*
Database::GetCleanHtml(const string& str)
{
    reallocateScratch(str.length());
    char* to = scratch_buffer;
    const char* from = str.c_str();
    while (*from) {
        if (*from == '\'') {
            *to++ = '\'';
            *to++ = '\'';
        } else
            *to++ = *from;
        from++;
    }
    *to = 0;
    return scratch_buffer;
}

void
Database::CleanReverse(string& str, CLEANTYPE /*ct*/)
{
    reallocateScratch(str.length());
    char* to = scratch_buffer;
    const char* from = str.c_str();

    // Reverse the cleaning
    while (*from) {
        if (*from == '\\') {
            from++;
            if (*from == 'n')
                *to++ = '\n';
            else if (*from == 't')
                *to++ = '\t';
            else
                *to++ = *from;
        } else
            *to++ = *from++;
    }
    *to = 0;
    str = scratch_buffer;
}

// -------------------------------------------------------------------------------------------------
int
Database::PrintNumber(char* buffer, const char* format, double number)
{
    register int i;
    int bLen = 0;

    bLen = sprintf(buffer, format, number);
    if (commaDecimal) {
        for (i = 0; i < bLen; i++) {
            if (buffer[i] == ',') {
                buffer[i] = '.';
                break;
            }
        }
    }
    return bLen;
}

// -------------------------------------------------------------------------------------------------
const char*
Database::PrintNumber(double number)
{
    static char buffer[30]; // It is doubtful that this many numbers can be printed from double.
    char* ptr = buffer;
    char* end = buffer + sprintf(buffer, "%f", number);
    if (commaDecimal) {
        while (ptr < end) {
            if (*ptr == ',') {
                *ptr = '.';
                break;
            }
            ptr++;
        }
    }
    return buffer;
}

// ------------------------------------------------------------------------------------------
// Static functions

void
Database::TrimTail(std::string* target)
{
    std::string::reverse_iterator rit;
    for (rit = target->rbegin(); rit != target->rend(); rit++)
        if (*rit != ' ')
            break;
    if (rit == target->rend())
        target->clear();
    else
        target->erase(rit.base(), target->end());
}

bool
Database::ExtractTimestamp(const char* result, struct tm* tmPtr)
{
    char* dummy;
    unsigned int len = strlen(result);
    memset(tmPtr, 0, sizeof(tm));
    if (len >= 10) {
        tmPtr->tm_year = strtol(result, &dummy, 10) - 1900;
        tmPtr->tm_mon = strtol(result + 5, &dummy, 10) - 1;
        tmPtr->tm_mday = strtol(result + 8, &dummy, 10);
        if (len >= 19) {
            tmPtr->tm_hour = strtol(result + 11, &dummy, 10);
            tmPtr->tm_min = strtol(result + 14, &dummy, 10);
            tmPtr->tm_sec = strtol(result + 17, &dummy, 10);
            tmPtr->tm_isdst = -1;
        }
        return true;
    }
    CS_PRINT_NOTE("Database::ExtractDate - Empty date detected.");
    return false;
}

}; // namespace ddb
