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
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

using namespace libIRDB;
using namespace std;



File_t::File_t(db_id_t myfile_id, db_id_t my_orig_fid, std::string myurl, std::string myhash, std::string myarch, int myoid, 
		std::string atn, std::string ftn, std::string itn, std::string icfs, std::string icfs_map, std::string rtn, std::string typ, db_id_t mydoipid) :
	BaseObj_t(NULL), url(myurl), hash(myhash), arch(myarch), elfoid(myoid),
	address_table_name(atn), function_table_name(ftn), instruction_table_name(itn), icfs_table_name(icfs), icfs_map_table_name(icfs_map),
	relocs_table_name(rtn), types_table_name(typ), orig_fid(my_orig_fid)
{
	SetBaseID(myfile_id);
}


void File_t::CreateTables()
{

	// to avoid duplicate schemas for the DB, we're calling 
	// the script that has the table creation schema in it  
	string home(getenv("PEASOUP_HOME"));
	string tmpfile= "db_script."+to_string(getpid());

	string command=home+"/tools/db/pdb_create_program_tables.sh "+
		address_table_name+" "+
		function_table_name+" "+
		instruction_table_name+" "+
		icfs_table_name+" "+
		icfs_map_table_name+" "+
		relocs_table_name+" "+
		types_table_name+" "+
		tmpfile;

	system(command.c_str());


	std::ifstream t(tmpfile.c_str());
	std::stringstream buffer;
	buffer << t.rdbuf();


	dbintr->IssueQuery(buffer.str().c_str());	

}
