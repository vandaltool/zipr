
#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h>
#include <fstream>
#include <iostream>

using namespace libIRDB;
using namespace std;



File_t::File_t(db_id_t myfile_id, db_id_t my_orig_fid, std::string myurl, std::string myhash, std::string myarch, int myoid, 
		std::string atn, std::string ftn, std::string itn, std::string rtn, db_id_t mydoipid) :
	BaseObj_t(NULL), url(myurl), hash(myhash), arch(myarch), elfoid(myoid),
	address_table_name(atn), function_table_name(ftn), instruction_table_name(itn), 
	relocs_table_name(rtn), orig_fid(my_orig_fid)
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
		relocs_table_name+" "+
		tmpfile;

	system(command.c_str());


	std::ifstream t(tmpfile.c_str());
	std::stringstream buffer;
	buffer << t.rdbuf();


	dbintr->IssueQuery(buffer.str().c_str());	

}
