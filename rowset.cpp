/* This file is part of 'Direct Database' C++ library (directdb)
 * https://github.com/jaaskelainen-aj/directdb
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>

#include "directdb.hpp"

namespace ddb {

// -------------------------------------------------------------------------------------------------
BoundField::BoundField(DT type_in, void* data_in)
  : type(type_in)
  , data(data_in)
/*!
  Simplified constructor for the situations when the field name is not needed
  and it is known that the bound values are to be used only in queries.

  \param type_in Field type. One of DDB_TYPE...
  \param data_in Pointer to client data.
*/
{
    next = 0;
}

// -------------------------------------------------------------------------------------------------
BoundField::~BoundField()
/*!
  Empty destructor
*/
{}

// -------------------------------------------------------------------------------------------------
RowSet::RowSet()
/*!
  Since this is protected member function only Database-class
  as a friend to this class can construct these (i.e. internal use only).
*/
{
    fieldRoot = 0;
    field_count = 0;
    row_count = 0;
}

// -------------------------------------------------------------------------------------------------
RowSet::~RowSet()
/*!
    Rowset destructor releases all the bound variables.
*/
{
    BoundField *field, *nextf;

    field = fieldRoot;
    while (field) {
        nextf = field->next;
        delete field;
        field = nextf;
    }
    fieldRoot = 0;
}

// -------------------------------------------------------------------------------------------------
bool
RowSet::ValidateBind(DT type, void* data)
/*!
  Function quicly validates that the type is valid and data is specified.
  \param type Data type. One of DDB_TYPE...
  \param data Pointer to actual data on the client.
*/
{
    if (!data)
        return false;
    int tval = (int)type;
    if (tval < 0 || tval > (int)DT::CHR)
        return false;
    return true;
}

// -------------------------------------------------------------------------------------------------
bool
RowSet::Bind(DT type, void* data)
/*!
  Function binds given field variable into this rowset. It is important to call this
  function for each variable one wants to retreive or save from/into the rowset. In
  addition the order of calls is important: first field appearing in the query result
  should be bound first and so on.

  Each time client calls the GetNext the rowset copies the values from the next row into
  the bound variables.

  \param type DDBT... for the variable. This is used to convert the database variable
   into program compatible value.
  \param data Pointer to the client side data buffer.
  \retval bool if the Bind is successfull. false if not.
*/
{
    if (!ValidateBind(type, data))
        return false;
    return InsertField(new BoundField(type, data));
}

// -------------------------------------------------------------------------------------------------
bool
RowSet::InsertField(BoundField* newField)
/*!
  Inserts a new field into the bound list. Function is used by Bind functions.

   \param newField Field to insert at the end of the list.
   \retval True if the insert is succesful, false if not.
*/
{
    // If this is first field:
    if (!fieldRoot) {
        // Make this the root and return success.
        fieldRoot = newField;
        return true;
    }

    // While existing fields:
    BoundField* field = fieldRoot;
    while (field) {
        // If this is the last field:
        if (!field->next) {
            // Add new to the end and return success.
            field->next = newField;
            field_count++;
            return true;
        }
        // Get next field.
        field = field->next;
    }

    // Return error.
    delete newField;
    return false;
}

}; // namespace ddb
