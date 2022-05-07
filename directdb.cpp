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

    le_size = 0x400;
    last_error = new char[le_size];

    RowSet::InitQueryBuffer();
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
    delete[] last_error;
    RowSet::DeleteQueryBuffer();
}
// -------------------------------------------------------------------------------------------------
void 
Database::SetLastError(const char *text)
{
    if(strlen(text) >= le_size) {
        delete[] last_error;
        le_size = strlen(text);
        le_size += le_size/10;
        last_error = new char[le_size];
    }
    strcpy(last_error, text);
}
void 
Database::AppendLastError(const char *text)
{
    size_t clen = strlen(last_error);
    size_t tlen = strlen(text);
    if(clen + tlen >= le_size) {
        le_size = clen + tlen + tlen/10;
        char* new_le = new char[le_size];
        strcpy(new_le, last_error);
        delete[] last_error;
        last_error = new_le;
    }
    strcat(last_error, text);
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
    int bLen = sprintf(buffer, format, number);
    if (commaDecimal) {
        for (int i = 0; i < bLen; i++) {
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
