
#include <all.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;


File_t::File_t(db_id_t myfile_id, db_id_t my_orig_fid, std::string myurl, std::string myhash, std::string myarch, int myoid, 
		std::string atn, std::string ftn, std::string itn, db_id_t mydoipid) :
	BaseObj_t(NULL), url(myurl), hash(myhash), arch(myarch), elfoid(myoid),
	address_table_name(atn), function_table_name(ftn), instruction_table_name(itn), orig_fid(my_orig_fid)
{
	SetBaseID(myfile_id);

}


void File_t::CreateTables()
{
/*
 * WARNING!  If you edit these tables, you must also edit $PEASOUP_HOME/tools/db/*.tbl
 */

	dbintr->IssueQuery(
		"CREATE TABLE " + address_table_name + 
		" ( "
		"  address_id         	SERIAL  PRIMARY KEY, "
		"  file_id            	integer REFERENCES file_info, "
		"  vaddress_offset    	integer, "
		"  doip_id		integer DEFAULT -1 "
		");"
	);

	dbintr->IssueQuery(
		"CREATE TABLE " + function_table_name + 
		" ( "
  		"	function_id        	SERIAL  PRIMARY KEY, "
  		"	file_id            	integer REFERENCES file_info, "
  		"	name               	text, "
  		"	stack_frame_size   	integer, "
  		"	doip_id	   	integer DEFAULT -1, "
		"       out_args_region_size    integer, "
		"       use_frame_pointer    integer "
		"); "
	);

	dbintr->IssueQuery(
		"CREATE TABLE " + instruction_table_name + 
		" ( "
		"instruction_id		   SERIAL PRIMARY KEY, "
  		"address_id                integer REFERENCES " + address_table_name + ", " +
  		"parent_function_id        integer, "
  		"orig_address_id           integer, "
  		"fallthrough_address_id    integer DEFAULT -1, "
  		"target_address_id         integer DEFAULT -1, "
  		"data                      bytea, "
  		"callback                  text, "
  		"comment                   text, "
		"ind_target_address_id 	   integer DEFAULT -1, "
  		"doip_id		   integer DEFAULT -1 "
		");"
	);
}
