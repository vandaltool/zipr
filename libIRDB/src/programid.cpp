

#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;


/*
 * Create a new program ID that is not yet in the database
 */
ProgramID_t::ProgramID_t() :
	BaseObj_t(NULL)
{
        schema_ver=-1;
        orig_pid=-1;       
        name="";
        address_table_name="";
        function_table_name="";
        instruction_table_name="";
}

ProgramID_t::ProgramID_t(db_id_t pid) : BaseObj_t(NULL)
{
	std::string q="select * from program_info where variant_id = " ;
	q+=to_string(pid);
	q+=";";


	try 
	{
		BaseObj_t::dbintr->IssueQuery(q);
	}
	catch (const std::exception &e)
	{
        	schema_ver=-1;
        	orig_pid=-1;       
        	name="";
        	address_table_name="";
        	function_table_name="";
        	instruction_table_name="";

		throw DatabaseError_t(DatabaseError_t::ProgramNotInDatabase); 
	};
        SetBaseID(atoi(BaseObj_t::dbintr->GetResultColumn("variant_id").c_str()));
        schema_ver=atoi(BaseObj_t::dbintr->GetResultColumn("schema_version_id").c_str());
        orig_pid=atoi(BaseObj_t::dbintr->GetResultColumn("orig_variant_id").c_str());
        name=(BaseObj_t::dbintr->GetResultColumn("name"));
        address_table_name=(BaseObj_t::dbintr->GetResultColumn("address_table_name"));
        function_table_name=(BaseObj_t::dbintr->GetResultColumn("function_table_name"));
        instruction_table_name=(BaseObj_t::dbintr->GetResultColumn("instruction_table_name"));

	BaseObj_t::dbintr->MoveToNextRow();
	assert(BaseObj_t::dbintr->IsDone());
}

bool ProgramID_t::IsRegistered()
{
	return GetBaseID()!=BaseObj_t::NOT_IN_DATABASE;
}

bool ProgramID_t::Register()
{
	assert(0);
}    

ProgramID_t ProgramID_t::Clone()
{
	assert(0);
}       

void ProgramID_t::WriteToDB()
{
	assert(0);
}

std::ostream& libIRDB::operator<<(std::ostream& out, const ProgramID_t& pid)
{

	out << "("
		"schema="<<pid.schema_ver<<":"
		"orig_pid="<<pid.orig_pid<<":"
		"name="<<pid.name<<":"
		"ATN="<<pid.address_table_name<<":"
		"FTN="<<pid.function_table_name<<":"
		"ITN="<<pid.instruction_table_name<<")";
	return out;
}

