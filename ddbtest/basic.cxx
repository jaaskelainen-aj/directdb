// g++ -std=c++11 -Wall -ggdb -o basic basic.cxx -I/usr/local/include/sqlite3 -L
// /usr/local/lib-d/cpp4scripts -L /usr/local/lib/sqlite3 -L ../debug -l directdb -l c4s -l sqlite3
// -l stdc++
#include <time.h>
#include <iostream>
#include <sstream>
#include <cpp4scripts/cpp4scripts.hpp>

using namespace std;
using namespace c4s;

#define __DDB_SQLITE3__
#include "../directdb.hpp"
using namespace ddb;

class SimpleRS : public RSInterface
{
  public:
    SimpleRS() { id = 0; }
    void PostCreate(RowSet* _rs)
    {
        rs = _rs;
        rs->Bind(DT::INT, &id);
        rs->Bind(DT::STR, &name);
    }
    bool Query()
    {
        string query("SELECT id, name FROM simple");
        return rs->Query(query);
    }
    int id;
    string name;
};

bool
useRSI(Database* db)
{
    cout << "# Test RSInterface query\n";
    if (!db->FindSchemaItem(ST::TABLE, "simple")) {
        cout << "'simple' table does not exists\n";
        return false;
    }
    SimpleRS rs;
    db->CreateRowSet(&rs);
    rs.Query();
    while (rs.GetNext()) {
        cout << "id=" << rs.id;
        cout << ";  " << rs.name << '\n';
    }
    return true;
}

bool
simple(Database* db)
{
    cout << "# Test table creation: simple(id, name)\n";
    if (db->FindSchemaItem(ST::TABLE, "simple")) {
        cout << "'simple' table exists\n";
        return true;
    }
    cout << "Creating 'simple' table.\n";
    string tbl1("CREATE TABLE simple("
                "id int, "
                "name varchar,"
                "constraint spk primary key (id)"
                ")");
    if (!db->UpdateStructure(tbl1)) {
        cout << "Unable to add 'simple' table\n";
        return false;
    }

    const char* names[6] = { "Mike", "Ben", "Jim", "Lisa", "Susan", "Kate" };
    ostringstream sql;
    for (int ndx = 1; ndx < 7; ndx++) {
        sql.str("");
        sql << "INSERT INTO simple(id, name) VALUES ";
        sql << "(" << ndx << ",'" << names[ndx - 1] << "')";
        if (db->ExecuteModify(sql.str()) < 0) {
            cout << "Insert failed at item: " << ndx << '\n';
            return false;
        }
    }
    cout << "Simple table filled\n";
    return true;
}

bool
useFreeBind(Database* db)
{
    int rsid;
    string rsname;

    cout << "# Test free bind query\n";
    RowSet* rs = db->CreateRowSet();
    rs->Bind(DT::INT, &rsid);
    rs->Bind(DT::STR, &rsname);

    string query("SELECT id, name from simple where id<4");
    if (!rs->Query(query)) {
        cout << "Query of 'simple' failed.\n";
        cout << db->GetErrorDescription(rs) << '\n';
    }
    while (rs->GetNext()) {
        cout << "id=" << rsid;
        cout << ";  " << rsname << '\n';
    }
    delete rs;
    return true;
}

bool
useTXInsert(Database* db, char cmd)
{
    ostringstream sql;
    time_t now = time(0) & 0xFFFF;

    cout << "# Test transactive insert\n";
    if (!db->StartTransaction()) {
        cout << "BEGIN failed: " << db->GetErrorDescription();
        return false;
    }
    for (int ndx = 1; ndx < 6; ndx++) {
        sql.str("");
        sql << "INSERT INTO simple(id,name) VALUES ";
        sql << "(" << now + ndx << ",'tx name " << ndx << cmd << "')";
        if (db->ExecuteModify(sql.str()) < 0) {
            cout << "Insert failed at " << ndx << '\n';
            db->RollBack();
            return false;
        }
        if (cmd == 'b' && ndx == 3)
            return true;
    }
    if (!db->Commit()) {
        cout << "COMMIT failed: " << db->GetErrorDescription();
        return false;
    }
    return true;
}

int
main(int argc, char** argv)
{
    if (argc < 2) {
        cout << "Please name database as parameter.\n";
        return 1;
    }
    c4s::logbase::init_log(c4s::LL_TRACE, new c4s::stderr_sink());

    Sqlite* db = new Sqlite();
    if (!db->Connect(argv[1])) {
        cout << "Unable to find/create " << argv[1] << '\n';
        return 2;
    }
    if (simple(db) && useRSI(db) && useFreeBind(db) &&
        useTXInsert(db, argc == 3 ? argv[2][0] : '-')) {
        cout << "--- finally in simple:\n";
        useRSI(db);
        cout << "\nOK\n";
    } else
        cout << "Failed\n";
    delete db;
    return 0;
}
