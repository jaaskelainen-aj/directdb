/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef DDB_POSTGRE_H_FILE
#define DDB_POSTGRE_H_FILE

#include <libpq-fe.h>

namespace ddb {

// -------------------------------------------------------------------------------------------------
//! Class defines PostgreSQL specific implementation to Database-interface.
class Postgre : public Database
{
  public:
    Postgre();
    ~Postgre();

    // Database interface
    RDBM GetType() { return RDBM::POSTGRES; }
    bool Connect(const char* constr);
    bool Disconnect();
    bool IsConnectOK();
    bool ResetConnection();
    //
    RowSet* CreateRowSet();
    bool CreateRowSet(RSInterface*);
    //
    bool StartTransaction();
    bool Commit();
    bool RollBack();
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
    bool FindSchemaItem(ST stype, const char* name) { return false; } // TODO

    // Unique interface
    PGconn* GetPGConn();
    bool IsConnected() { return connection == 0 ? false : true; }

    // Admin commands
    bool CreateUser(const std::string& uid, const std::string& pwd);
    bool CreateDatabase(const std::string& dbname, const std::string& owner);
    bool CreateTable(const std::string& name, const std::string& sql);

    static Postgre* getAdminConn()
    {
        Postgre* pg = new Postgre();
        pg->Connect("host=/var/run dbname=postgres user=postgres connect_timeout=10");
        return pg;
    }

  protected:
    PGconn* connection;
};

void
PQNoticeProcessor(void*, const char* message);

// -------------------------------------------------------------------------------------------------
//! Class defines PostgreSQL specific implementation to RowSet-interface.
class PostgreRowSet : public RowSet
{
    friend class Postgre;

  public:
    ~PostgreRowSet();

    bool Query();
    int GetNext();
    void Reset();

  protected:
    PostgreRowSet(Database*);

    Postgre* db;          //!< Pointer to databse object.
    size_t max_rows;      //!< Total number of records in the current query.
    PGresult* result;     //!< Pointer to the result structure.
    bool result_complete; //!< True if the results have been retrieved..
};

inline PGconn*
Postgre::GetPGConn()
{
    return connection;
}

}; // namespace ddb

#endif
