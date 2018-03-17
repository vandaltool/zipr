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

static void ignore_result(int /* res */) { }



File_t::File_t(const db_id_t &myfile_id, const db_id_t &my_orig_fid, const std::string &myurl, 
	       const std::string &myhash, const std::string &myarch, const int &myoid, 
	       const std::string &atn, const std::string &ftn, const std::string &itn, const std::string &icfs, 
               const std::string &icfs_map, const std::string &rtn, const std::string &typ, const std::string &scoop, 
               const std::string &ehpgms, const std::string &ehcss, 
	       const db_id_t &mydoipid) 
	:
	BaseObj_t(NULL), 
	orig_fid(my_orig_fid),
	url(myurl), 
	hash(myhash), 
	arch(myarch), 
	address_table_name(atn), 
	function_table_name(ftn), 
	instruction_table_name(itn), 
  	icfs_table_name(icfs), 
	icfs_map_table_name(icfs_map), 
	relocs_table_name(rtn), 
	types_table_name(typ), 
	scoop_table_name(scoop), 
	ehpgm_table_name(ehpgms), 
	ehcss_table_name(ehcss), 
	elfoid(myoid)
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
		scoop_table_name+" "+
		ehpgm_table_name+" "+
		ehcss_table_name+" "+
		tmpfile;

	ignore_result(system(command.c_str()));


	std::ifstream t(tmpfile.c_str());
	std::stringstream buffer;
	buffer << t.rdbuf();


	dbintr->IssueQuery(buffer.str().c_str());	

}
