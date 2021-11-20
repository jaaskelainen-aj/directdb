/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef SQLITE_H_FILE
#define SQLITE_H_FILE

#include <sqlite3.h>

namespace ddb {

// -------------------------------------------------------------------------------------------------
//! Class defines Sqlite3 specific implementation to Database-interface.
class Sqlite : public Database
{
  public:
    Sqlite();
    ~Sqlite();

    // Database interface
    RDBM GetType() { return RDBM::SQLITE; }
    bool Connect(const char*);
    bool Disconnect();
    bool IsConnectOK() { return connection ? true : false; }
    bool ResetConnection() { return true; }
    //
    RowSet* CreateRowSet();
    bool CreateRowSet(RSInterface*);
    //
    bool StartTransaction() { return false; }
    bool Commit() { return false; }
    bool RollBack() { return false; }
    //
    bool ExecuteIntFunction(const std::string& query, int& val);
    bool ExecuteLongFunction(const std::string& query, long& val);
    bool ExecuteDoubleFunction(const std::string& query, double& val);
    bool ExecuteBoolFunction(const std::string& query, bool& val);
    bool ExecuteStrFunction(const std::string& query, std::string& result);
    bool ExecuteDateFunction(const std::string& query, tm& val);
    //
    int ExecuteModify(const std::string& query);
    unsigned long GetInsertId();
    bool UpdateStructure(const std::string& command);
    std::string GetErrorDescription(RowSet* rs);
    bool FindSchemaItem(ST, const char* name);

    // Unique interface
    sqlite3* GetConnection() { return connection; }

    // Passing data with Sqlite call backs
    struct ExecData
    {
        ExecData(void* v)
        {
            rows = 0;
            value = v;
        }
        unsigned int rows;
        void* value;
    };

  protected:
    sqlite3* connection;
    int errval;
};

// -------------------------------------------------------------------------------------------------
//! Class defines Sqlite specific implementation to RowSet-interface.
class SqliteRowSet : public RowSet
{
    friend class Sqlite;

  public:
    ~SqliteRowSet();

    bool Query();
    int GetNext();
    void Reset();

  protected:
    SqliteRowSet(Sqlite*);

    Sqlite* db;           //!< Pointer to databse object.
    sqlite3_stmt* stmt;   //!< Prepared statement
    bool result_complete; //!< True if the result has been queried or reset.
};

}; // namespace ddb

#endif
