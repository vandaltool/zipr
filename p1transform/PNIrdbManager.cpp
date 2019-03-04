/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include "PNIrdbManager.hpp"

#include <stdlib.h>
#include <set>
#include <algorithm>

PNIrdbManager::PNIrdbManager(IRDB_SDK::DatabaseID_t variant_db_id)
{
    // look up this variant ID
    m_variant_id = IRDB_SDK::VariantID_t::factory(variant_db_id);
    assert(m_variant_id != NULL);

    // maintain a mapping of function name to Function_t object
    m_file_ir = IRDB_SDK::FileIR_t::factory(m_variant_id.get());
    std::set<IRDB_SDK::Function_t*> function_set = m_file_ir->getFunctions();
    for (std::set<IRDB_SDK::Function_t*>::const_iterator it = function_set.begin();
         it != function_set.end(); it++)
    {
        m_function_map[(*it)->getName()] = *it;
    }

    // create new table name (lowercase)
    assert(!m_variant_id->getName().empty());
    m_table_name = m_variant_id->getName() + "_stack";
    std::transform(m_table_name.begin(),
                   m_table_name.end(),
                   m_table_name.begin(),
                   ::tolower);
}

PNIrdbManager::~PNIrdbManager()
{
	// handled by unique_ptr
	//delete m_file_ir;
	//delete m_variant_id;
}

void
PNIrdbManager::CreateTable()
{
#if 1
	assert(0); // depreciated
#else
    std::stringstream query;
    query << "CREATE TABLE " << m_table_name << " ("
          << "stack_id               SERIAL PRIMARY KEY, "
          << "source_id              INTEGER, "
          << "esp_offset             INTEGER, "
          << "size                   INTEGER, "
          << "function_id            INTEGER, "
          << "instruction_id         INTEGER DEFAULT (-1), "
          << "range_start_address_id INTEGER DEFAULT (-1),"
          << "range_end_address_id   INTEGER DEFAULT (-1),"
          << "doip                   INTEGER DEFAULT (-1)"
          << ");";

    IRDB_SDK::pqxxDB_t transaction;
    transaction.IssueQuery(query);
    transaction.Commit();
#endif

}

bool
PNIrdbManager::TableExists()
{
#if 1
	assert(0); // depreciated
#else
    // using pg_tables is Postgresql-specific
    std::stringstream query;
    query << "SELECT COUNT(tablename) FROM pg_tables WHERE "
          << "tablename='" << m_table_name << "';";

    IRDB_SDK::pqxxDB_t transaction;
    transaction.IssueQuery(query);
    assert(!transaction.IsDone());
    int table_count = atoi(transaction.GetResultColumn("count").c_str());
    return table_count > 0;
#endif
}


void
PNIrdbManager::ClearTable()
{
#if 1
	assert(0); // depreciated
#else
    std::stringstream query;
    query << "TRUNCATE TABLE " << m_table_name << " CASCADE;";

    IRDB_SDK::pqxxDB_t transaction;
    transaction.IssueQuery(query);
    transaction.Commit();
#endif
}

void PNIrdbManager::DeleteSource(IRSource source)
{
#if 1
	assert(0); // depreciated
#else
    std::stringstream query;
    query << "DELETE FROM " << m_table_name << " WHERE source_id=" << source
          << ";";

    IRDB_SDK::pqxxDB_t transaction;
    transaction.IssueQuery(query);
    transaction.Commit();
#endif
}

IRDB_SDK::DatabaseID_t PNIrdbManager::InsertStackObject(
    std::string function_name,
    int offset,
    unsigned int size,
    IRSource source)
{
#if 1
	assert(0); // depreciated
#else
    assert(m_function_map.find(function_name) != m_function_map.end());
    IRDB_SDK::Function_t * func = m_function_map[function_name];
std::cout << "IRDB: function name: " << function_name << ", ID: " << func->getBaseID();
    IRDB_SDK::pqxxDB_t transaction;
    std::stringstream query;
    query << "INSERT INTO " << m_table_name
          << " (esp_offset, size, function_id, source_id) VALUES ("
          << offset << "," << size << "," << func->getBaseID() << "," << source
          << ") RETURNING stack_id; ";

    transaction.IssueQuery(query);
    transaction.Commit();

    return atoi(transaction.GetResultColumn("stack_id").c_str());
#endif
}
