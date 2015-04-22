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
#include <utils.hpp>

using namespace libIRDB;
using namespace std;

string ICFS_t::WriteToDB(File_t *fid)
{
	assert(fid);

	db_id_t icfs_id = GetBaseID();

	// one of the many postgres encoding of boolean values
	string complete = IsComplete() ? "t" : "f"; 

	string q=string("insert into ") + fid->icfs_table_name + 
		string(" (icfs_id, is_complete) VALUES (") + 
		string("'") + to_string(icfs_id) + string("', ") + 
		string("'") + complete + string("'); ") ;

	for (InstructionSet_t::const_iterator it = this->begin(); 
		it != this->end(); ++it)
	{
		Instruction_t *insn = *it;		
		assert(insn);

		db_id_t address_id = insn->GetAddress()->GetBaseID();

		q += string("insert into ") + fid->icfs_map_table_name +
			string(" (icfs_id, address_id) VALUES(") +
			string("'") + to_string(icfs_id) + string("', ") +
			string("'") + to_string(address_id) + string("'); ");
	}

	return q;
}

