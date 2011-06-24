

#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;
using namespace std;


/*
 * Create a new variant ID that is not yet in the database
 */
VariantID_t::VariantID_t() :
	BaseObj_t(NULL)
{
        schema_ver=CURRENT_SCHEMA;
        orig_pid=-1;       
        name="";
        address_table_name="";
        function_table_name="";
        instruction_table_name="";
}


void VariantID_t::CreateTables()
{

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
  		"	doip_id	   	integer DEFAULT -1 "
		"); "
	);

	dbintr->IssueQuery(
		"CREATE TABLE " + instruction_table_name + 
		" ( "
		"instruction_id		   SERIAL PRIMARY KEY, "
  		"address_id                integer REFERENCES " + address_table_name + ", " +
  		"parent_function_id        integer, "
  		"file_id                   integer REFERENCES file_info, "
  		"orig_address_id           integer, "
  		"fallthrough_address_id    integer, "
  		"target_address_id         integer, "
  		"data                      bytea, "
  		"comment                   text, "
  		"doip_id		    integer DEFAULT -1 "
		");"
	);
}

VariantID_t::VariantID_t(db_id_t pid) : BaseObj_t(NULL)
{
	std::string q="select * from Variant_info where variant_id = " ;
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

		throw DatabaseError_t(DatabaseError_t::VariantTableNotRegistered); 
	};

	if(BaseObj_t::dbintr->IsDone())
		throw DatabaseError_t(DatabaseError_t::VariantNotInDatabase); 

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

bool VariantID_t::IsRegistered()
{
	return GetBaseID()!=BaseObj_t::NOT_IN_DATABASE;
}

bool VariantID_t::Register()
{
	assert(!IsRegistered());

	std::string q;
	q="insert into variant_info (schema_version_id,name) "
		"values('";
	q+=to_string(schema_ver);
	q+="','";
	q+=to_string(name);
	q+="')";
	q+="returning variant_id;";

	dbintr->IssueQuery(q);
	assert(!BaseObj_t::dbintr->IsDone());

	db_id_t newid=atoi(dbintr->GetResultColumn("variant_id").c_str());

	/* set IDs */
	SetBaseID(newid);
	if(NOT_IN_DATABASE==orig_pid)
		orig_pid=newid;

	address_table_name="AddressTable_Variant"+to_string(GetBaseID());
	function_table_name="FunctionTable_Variant"+to_string(GetBaseID());
	instruction_table_name="InstructionTable_Variant"+to_string(GetBaseID());

	BaseObj_t::dbintr->MoveToNextRow();
	assert(BaseObj_t::dbintr->IsDone());

	CreateTables();
}    

VariantID_t* VariantID_t::Clone()
{
	assert(IsRegistered());	// cannot clone something that's not registered 

	// create the new program id 
	VariantID_t *ret=new VariantID_t;
	
	// set the inhereted fields 
	ret->SetName(name+"_cloneof"+to_string(GetBaseID()));
	ret->orig_pid=orig_pid;

	// register the new VID to the database. 
	ret->Register();
	// and write it to the database
	ret->WriteToDB();

	// clone the tables 	
	std::string q;

	// first drop the old values 
	q="drop table ";
	q+=ret->instruction_table_name;
	q+=" ; ";
	dbintr->IssueQuery(q);

	q="drop table ";
	q+=ret->address_table_name;
	q+=" ; ";
	dbintr->IssueQuery(q);

	q="drop table ";
	q+=ret->function_table_name;
	q+=" ; ";
	dbintr->IssueQuery(q);


	// next issue SQL to clone each table
	q="select * into ";
	q+=ret->address_table_name;
	q+=" from ";
	q+=address_table_name;
	q+=" ;";
	dbintr->IssueQuery(q);

	q="select * into ";
	q+=ret->instruction_table_name;
	q+=" from ";
	q+=instruction_table_name;
	q+=" ;";
	dbintr->IssueQuery(q);
	
	q="select * into ";
	q+=ret->function_table_name;
	q+=" from ";
	q+=function_table_name;
	q+=" ;";
	dbintr->IssueQuery(q);


	// lastly update the variant_dependency table to make a copy of the rows in which 
	// the old variant depended upon.  The new rows will indicate that the 
	// new variant also depends on those files 
	q="insert into variant_dependency (variant_id, file_id, doip_id) select '";
	q+=to_string(ret->GetBaseID());
	q+="', file_id, doip_id from variant_dependency where variant_id='";
	q+=to_string(GetBaseID());
	q+="';";
	dbintr->IssueQuery(q);

	return ret;
}       

void VariantID_t::WriteToDB()
{
	assert(IsRegistered());

	std::string q="update variant_info SET ";
	q+=" schema_version_id = '" + to_string(schema_ver) + "', ";
	q+=" name = '"  + name  + "', ";
	q+=" orig_variant_id = '" + to_string(orig_pid) + "', ";
	q+=" address_table_name = '"  + address_table_name + "', ";
	q+=" function_table_name = '"  + function_table_name + "', ";
	q+=" instruction_table_name = '" + instruction_table_name + "', ";
	q+=" doip_id = '" + to_string(GetDoipID()) + "' ";
	q+=" where variant_id = '" + to_string(GetBaseID()) + "';";

	dbintr->IssueQuery(q);
}

std::ostream& libIRDB::operator<<(std::ostream& out, const VariantID_t& pid)
{

	out << "("
		"variant_id="<<pid.GetBaseID()<<":"
		"schema="<<pid.schema_ver<<":"
		"orig_pid="<<pid.orig_pid<<":"
		"name="<<pid.name<<":"
		"ATN="<<pid.address_table_name<<":"
		"FTN="<<pid.function_table_name<<":"
		"ITN="<<pid.instruction_table_name<<")";
	return out;
}


void VariantID_t::DropFromDB()
{
	assert(IsRegistered());

	string q;

	q =string("drop table ")+instruction_table_name + string(" cascade;");
	q+=string("drop table ")+address_table_name     + string(" cascade;");
	q+=string("drop table ")+function_table_name    + string(" cascade;");
	q+=string("delete from variant_dependency where variant_id = '") + to_string(GetBaseID()) + string("';");
	q+=string("delete from variant_info where variant_id = '") + to_string(GetBaseID()) + string("';");

	dbintr->IssueQuery(q);

	SetBaseID(NOT_IN_DATABASE);
	orig_pid=NOT_IN_DATABASE;
	name=instruction_table_name=address_table_name=function_table_name=string("");
        schema_ver=CURRENT_SCHEMA;
}
