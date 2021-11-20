/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef DDB_H_FILE
#define DDB_H_FILE

#include <string>
#include <fstream>
#include <stdint.h>
#include <sstream>

namespace ddb {

enum class RDBM
{
    POSTGRES,
    SQLITE
}; // MYSQL, ODBC, MSSQL

// Library data types
enum class DT
{
    INT, // 32bit int
    STR, // String wxString or STL string.
    BOOL,
    BIT,  // MSSQL: conversion to char.
    TIME, // Timestamp: Date and time
    NUM,  // Numeric (double)
    DAY,  // Date only
    CHR   // Single character
};

// Schema types to query with FindSchemaItem
enum class ST
{
    TABLE,
    VIEW
};

// Database features
const short int FEATURE_AUTOTRIM = 0x0001;     // Automatically right trim the strings.
const short int FEATURE_TRANSACTIONS = 0x0002; // Database transactions.

// Database flags
const short int FLAG_INITIALIZED = 0x0001;
const short int FLAG_CONNECTED = 0x0002;
const short int FLAG_TRANSACT_ON = 0x0004;

//! RowSet class uses this to store the bound variables.
/*!
  This class is for directDB internal use only (please).
*/
class BoundField
{
  public:
    BoundField(DT, void*);
    virtual ~BoundField();

    DT type;          //!< Field type. One of DDBT... constants
    void* data;       //!< Pointer to client data buffer.
    BoundField* next; //!< Pointer to next bound field. Null signifies end of the list.
};

class RowSet;
class RSInterface;

// -------------------------------------------------------------------------------------------------
//! Database class represents the connection to the database.
/*! This class wraps the connection functionality. Each database application must have at
  least one connection. Database creates RowSets (= query resut, record set, resutl set,
  etc.). Each row set created by this class therefore shares this connection to the
  database.

  This class has not been designed to be thread safe. Use from single thread only!
*/
class Database
{
  public:
    Database();
    Database(Database&){};
    void operator=(Database&){};
    virtual ~Database();

    //! Returns the database type, i.e. one of the constants defined in directdb.hpp.
    /*! Function can be used as primitive RTTI.
      \retval int Database type.     */
    virtual RDBM GetType() = 0;

    /*! Makes a connection to the database.
        \param constr Connection string. Postgresql style string that tells connection attributes.
        \retval bool True on success, otherwise false. Use GetErrorDescription-function to get
       further information on error. */
    virtual bool Connect(const char* constr) = 0;

    /*! Closes the database connection and releases the resources reserved for that
        purpose.
        \retval bool True always.       */
    virtual bool Disconnect() = 0;

    /*! Checks the current connection status.
      \retval bool True if status is OK. False if not.       */
    virtual bool IsConnectOK() = 0;

    /*! Resets the connection to database.
      \retval bool True if reset succeeds. False if not.       */
    virtual bool ResetConnection() = 0;

    /*! Creates a RowSet object for this database (i.e. connection). Row sets perform
        the actual work on the database, i.e. selects, inserts, updates and deletions.
        Database can have multiple row sets open.  Because each row set must have a
        connection this is the only way to create objects of that class. Please see
        RowSet for more information.
        Please note that the database class does not delete RowSet objects
        automatically.  Programmer must explicitly remove them with delete-command.
        \retval New RowSet object.     */
    virtual RowSet* CreateRowSet() = 0;
    virtual bool CreateRowSet(RSInterface*) = 0;

    /*! Function will start a new transaction. Transaction should be terminated with
        either Commit- or RollBack-function. Commit will accept and save all work done
        during the transaction. RollBack will cancel all database operations done during
        the transaction. (Please see your database manual for details on transactions).

        Not all databases support transactions. In those cases this function simply
        returns error value.

        Please note that transactions are generally connection based. Starting a
        transaction affects all RowSets currently open for the connection
        (=database). There can be only one transaction in a connection. Subsequent calls to
        this function simply return error value. Current transaction is not terminated.
        \retval bool True if transaction started succesfully.
    */
    virtual bool StartTransaction() = 0;

    /*! Stops the current transaction and sends 'commit' command to database. If there is
        no current transaction this function does nothing. (See Transaction-function or
        your database manual for details.
    */
    virtual bool Commit() = 0;

    /*! Stops the current transaction and sends 'roll back' command to database. If there is
        no current transaction this function does nothing. (See Transaction-function or
        your database manual for details.
    */
    virtual bool RollBack() = 0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as an
        integer.  This function supposes that SELECT has only one integer field specified.
        \param query SQL SELECT-statement to be executed.
        \param val Ref to interger where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is
       unchanged.*/
    virtual bool ExecuteIntFunction(const std::string& query, int& val) = 0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        long integer.  This function supposes that SELECT has only one integer field specified.
        \param query SQL SELECT-statement to be executed.
        \param val Ref to interger where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is
       unchanged.*/
    virtual bool ExecuteLongFunction(const std::string& query, long& val) = 0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        double.  This function supposes that SELECT has only one numeric field specified.

        \param query SQL SELECT-statement to be executed. Returns value is -1 on error.
        \param val Ref to double where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is
       unchanged.
     */
    virtual bool ExecuteDoubleFunction(const std::string& query, double& val) = 0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a
        boolean.  This function supposes that SELECT has only one boolean field specified.

        \param query SQL SELECT-statement to be executed. Returns value is -1 on error.
        \param val Ref to boolean where the value will be stored into.
        \retval bool True if query was successful, false if not. In latter case the val is
       unchanged.
     */
    virtual bool ExecuteBoolFunction(const std::string& query, bool& val) = 0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a string.
        This function supposes that SELECT has only one char or varchar field specified.
        If query returns NULL then the result is cleared/emptied.

        \param query SQL SELECT-statement to be executed.
        \param result Reference to string where the answer is copied into.
        \retval bool True if query is succesful. False if result is NULL or error occurred.
     */
    virtual bool ExecuteStrFunction(const std::string& query, std::string& result) = 0;

    /*! Exectes given SQL SELECT-statement in the database and returns the result as a timestamp.
        This function supposes that SELECT has only one char or varchar field specified.

        \param query SQL SELECT-statement to be executed.
        \param val Reference to date where the answer is copied into.
        \retval bool True if query is succesful, false if not.
     */
    virtual bool ExecuteDateFunction(const std::string& query, tm& val) = 0;

    /*! Function executes given INSERT, MODIFY or DELETE SQL-statement and returns the result
        as integer. Generally most databases return the number of fields affected by the
        modification.

       \param query SQL SELECT-statement to be executed.
       \retval int Number of rows modified if the database supports the feature.
                   -1 on error. Please note that MySQL can return zero on succesful update if
       nothing was changed.
    */
    virtual int ExecuteModify(const std::string& query) = 0;

    /*! Function returns automatically assigned field value (id) from the last insert statement.
        In PostgreSQL field type is SEQUENCE,
        in MySql the field type is AUTO INCREMENT.
        For other databases this function returns 0. Please see further details from your database
       manual. \retval unsigned int Auto increment field value from last insert.
     */
    virtual unsigned long GetInsertId() = 0;

    /*! Used to update database structure commands like CREATE, DROP and ALTER TABLE.
      \param command SQL command to execute.
      \retval bool True if command is successful, false otherwise.
     */
    virtual bool UpdateStructure(const std::string& command) = 0;

    /*! This function should be called right after error has occurred in any of the
        directDB functions. In most cases the library is able to return descriptive string
        from the proprietary database interface. If databas does not support this feature
        or if the information is not available this function returns DirectDB library
        specific description of error.

        \param rs Pointer to the rowset that caused the error. NULL if not applicable.
    */
    virtual std::string GetErrorDescription(RowSet* rs = 0) = 0;

    /*! Searches for schema item from the database */
    virtual bool FindSchemaItem(ST, const char* name) = 0;

    // END OF PURE VIRTUAL FUNCTIONS
    //------------------------------------------------------------------------------------------

    /*! Prints floating point numbers in a safe manner. Comma is swapped to dot to accommodate
        SQL standards if the current locale uses comma as decimal separator.
        \param buffer Pointer to resulting number string.
        \param format Printf-style format for the number.
        \double number The number that sould be printed.
        \retval int Number of characters printed into buffer.
    */
    virtual int PrintNumber(char* buffer, const char* format, double number);

    /*! Prints floating point numbers in a safe manner. Comma is swapped to dot to accommodate
        SQL standards if the current locale uses comma as decimal separator.
        \param number Number that should be printed.
        \retval std::string& Reference to text representation of the number.
     */
    virtual const char* PrintNumber(double number);
    /*! All strings are passed to the database as they are. Problems can occur if user editable
        fields are transferred directly into the database because hyphen ('), backslash (\\)
        newline (\\n) and carriage return (\\r) have special meaning in SQL statements. This
       function will clean up the given string so that no problems will occur. This is not called
        automatically to all strings because it might slow things down unnecessarily.
        \param to Resulting string after the conversion.
        \param from Original string.
     */

    /*! Returns true if comma is used as a decimal separator in running environment. This means that
       when floating point numbers are printed the comma should be changed to period.
       PrintNumber-function does this automatically depending on the running environemnt.
    */
    virtual bool IsCommaDecimal() { return commaDecimal; };

    /*! Returns true if the transaction has been started i.e. is current on.
     */
    bool IsTransaction() { return (flags & FLAG_TRANSACT_ON) != 0; }

    // --
    virtual const char* CleanStr(const char* str);
    virtual std::string CleanStr(const std::string& str);
    virtual const char* GetCleanHtml(const std::string& str);
    enum CLEANTYPE
    {
        CT_NORMAL,
        CT_HTML
    };
    virtual void CleanReverse(std::string& str, CLEANTYPE ct = CT_NORMAL);

    bool IsConnected();
    bool IsFeatureOn(const unsigned short int ft) { return (ft & feat_on) > 0 ? true : false; }
    virtual int IsFeatureSupported(const int);

    std::string GetServerName();
    std::string GetDbName();
    std::string GetLastError();
    //! Returns latest error code. Zero on success.
    int GetErrorID() { return errorId; }
    //! Returns the connection port number;
    int GetPort() { return port; }
    //! Turns on one of several library features.
    bool SetFeature(const int);
    void SetErrorId(int id);

    static void TrimTail(std::string*);
    static bool ExtractTimestamp(const char* result, struct tm*);

  protected:
    void reallocateScratch(size_t size)
    {
        if (size <= scratch_size)
            return;
        delete[] scratch_buffer;
        scratch_buffer = new char[size];
        scratch_size = size;
    }

    std::string srvName; //!< Name of the server machine or it's ip address.
    std::string dbName;  //!< Name of the database in the server.
    std::string userid;  //!< Name of the user who owns the connection.
    std::string pwd;     //!< Password for the user.
    int port;            //!< Database port number. If 0 then default is used.

    unsigned short feat_support; //!< Supported features. A bit field of feature bits.
    unsigned short feat_on;      //!< Currently selected features.
    unsigned long errorId;       //!< Error id from the last database operation. Zero if all OK.
    short int flags;             //!< Operation flags. Combination of DDBFLAG_...
    bool commaDecimal;           //!< True if comma is decimal separator, false otherwise.
    char* scratch_buffer;        //!< Buffer for the string cleaning
    size_t scratch_size;         //!< size for the current buffer.
};

// -------------------------------------------------------------------------------------------------
class RowSet
/*!
  This class is the work horse of the DirectDB library. It has functions to
  send SQL statements to the database and to retrieve the results from these
  statements.

  Use Bind - SetQueryStmt - Query - GetNext function sequence to perform queries (SELECT
  -SQL statements) on the database. Each field specified in select must be bound.

  Use ExecuteModify-function for INSERT, UPDATE, DELETE SQL-statements. This function
  does not require you to bind variable beforehand.

  Use other Execute...-functions to perform simple SELECTs where the result consists only
  one field.
*/
{
    friend class Database;

  public:
    virtual ~RowSet();

    virtual bool Bind(DT type, void* data);

    /*! Sends the query statement to the database and waits for it to execute.
        Caller should have bound all fields from the SELECT-clause. Please note that
        this function does not actually retreive the records from the query. Each call to
        GetNext-function will move one record from the result set into bound variables
        i.e. you have to call GetNext at least once.

        \retval bool True on success, false on error.
        \sa Bind, GetNext, Reset
      */
    virtual bool Query() = 0;

    /*! Gets the next row of information from the query result. This is called right after
        Query-function.  Field values from the row are stored into bound variables. Bound
        variables are written over the next time this function is called.

        This function should be called repeatedly until it returns zero, i.e. all results
        have been fetched. If partial result has been retrieved, Reset-function should be called
        to make sure row set is not left into unsyncronized state (problem with MySql especially)

        \retval Number of fields converted. Note that this can be less than was specified in
        the query since some of the field values could have been NULLs. In this case the bound
        data is cleared (actual operation depends on the data type). If return value is zero
        then there is no more rows in the result set (= no fields converted). Bound variables
        remain unaltered in this case.
        \sa Reset
      */
    virtual int GetNext() = 0;

    /*! Releases the query results. If partial result set is read this function should be called to
        make sure the result set is left into proper state (MySql needs this). Query calls this
        automatically if Query is repeated without calling this in between.
     */
    virtual void Reset() {}

    /*! Returns number of fields currently bound */
    size_t GetFieldCount() { return field_count; }
    /*! Returns current row count */
    size_t GetRowCount() { return row_count; }
    std::ostringstream query; //!< Query statement.

  protected:
    RowSet();
    bool InsertField(BoundField* newField);
    bool ValidateBind(DT type, void* data);

    BoundField* fieldRoot; //!< First field of the bound field list.
    size_t field_count;    //!< Number of fields bound for this row set.
    size_t row_count;
};

// -------------------------------------------------------------------------------------------------
// Inheritable interface for Row Set
class RSInterface
{
  public:
    RSInterface()
      : rs(0)
    {}
    virtual ~RSInterface()
    {
        if (rs)
            delete rs;
    }
    bool GetNext()
    {
        if (rs)
            return rs->GetNext();
        else
            return false;
    }
    void Reset()
    {
        if (rs)
            rs->Reset();
    }
    int GetFieldCount() { return rs ? rs->GetFieldCount() : 0; }
    virtual void PostCreate(RowSet* _rs) = 0;

  protected:
    RowSet* rs;
};

// =============================================================================
//  INLINE FUNCTIONS

inline std::string
Database::GetServerName()
/*!
    Returns server name if the connection is on. Otherwice
    returns '\<No connection\>'
    \retval std::string server name
*/
{
    if (flags & FLAG_CONNECTED)
        return srvName;
    else
        return std::string("<No connection>");
}

// -------------------------------------------------------------------------------------------------
inline std::string
Database::GetDbName()
/*!
    Returns database name if the connection is on. Otherwice
    returns '\<No connection\>'
    \retval std::string Database name.
*/
{
    if (flags & FLAG_CONNECTED)
        return dbName;
    else
        return std::string("<No connection>");
}

// -------------------------------------------------------------------------------------------------
inline void
Database::SetErrorId(int id)
/*!
    This protected function is used only by library functions. It is used
    to store the most recent error code into the database object.
    \param id Error code of the most recent error.
*/
{
    errorId = id;
}

// -------------------------------------------------------------------------------------------------
inline bool
Database::IsConnected()
/*!
    \retval bool True if the database is in connected state.
*/
{
    return (flags & FLAG_CONNECTED) ? true : false;
}

// -------------------------------------------------------------------------------------------------
inline int
Database::IsFeatureSupported(const int featureId)
/*!
  This function is not supported currently.
  \param featureId Feature constants from the directdb.h
  \retval bool True if the given feature is supported. Otherwice function returns false.
*/
{
    return (feat_support & featureId) > 0 ? true : false;
}

// -------------------------------------------------------------------------------------------------
inline bool
Database::SetFeature(const int feature_new)
/*!
  Some database features (e.g. transactions) are not on by default. They need to be
  explicitly turned on with this function. Use logical OR '|' to combine more than one
  feature.

  \param feature_new     Feature to set.
  \retval bool True if the database supports the given features and the features have been
  successfully set on.
*/
{
    if (IsFeatureSupported(feature_new)) {
        feat_on |= feature_new;
        return true;
    }
    return false;
}

}; // namespace ddb

#endif // if defined DDB_H_FILE

#ifdef __DDB_POSTGRE__
#include "postgre.hpp"
#endif
#ifdef __DDB_SQLITE3__
#include "sqlite.hpp"
#endif
//#include "ddbmysql.hpp"
//#include "odbc.hpp"
//#include "firebird.hpp"
//#if defined(__DDB_MICROSOFT__) && defined(_WIN32)
//#include "ddbmicrosoft.hpp"
//#endif
