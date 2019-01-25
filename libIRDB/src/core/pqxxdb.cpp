/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */


#include <all.hpp>
#include <pqxx/pqxx>
#include <string>

using namespace libIRDB;
using namespace std;

pqxxDB_t::pqxxDB_t() : DBinterface_t(), txn(conn)
{
	/* no other init needed */
}

void pqxxDB_t::issueQuery(std::string query)
{
	results=txn.exec(query);
	results_iter=results.begin();
}

void pqxxDB_t::issueQuery(std::stringstream & query)
{
	results=txn.exec(query);
	results_iter=results.begin();
}

void pqxxDB_t::moveToNextRow()
{
	assert(!isDone());
	++results_iter;
}

std::string pqxxDB_t::getResultColumn(std::string colname)
{
	if(results_iter[colname].is_null())
		return std::string("");

	pqxx::binarystring bin_str(results_iter.at(colname));

	return bin_str.str();

//	return results_iter[colname].as<std::string>();
}

bool pqxxDB_t::isDone()
{
	return results_iter==results.end();
}

void pqxxDB_t::commit()
{
	txn.commit();
}

unique_ptr<IRDB_SDK::pqxxDB_t> IRDB_SDK::pqxxDB_t::factory()
{
	return unique_ptr<IRDB_SDK::pqxxDB_t>(new libIRDB::pqxxDB_t);
}
