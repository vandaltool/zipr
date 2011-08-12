#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;
using namespace std;



Function_t::Function_t(db_id_t id, std::string myname, int size, int oa_size, bool useFP, File_t* file)
	: BaseObj_t(NULL), my_file(file)
{
	SetBaseID(id);
	name=myname;
	stack_frame_size=size;	
	out_args_region_size=oa_size;
        use_fp = useFP;
}


void Function_t::WriteToDB()
{
	assert(0);
}

string Function_t::WriteToDB(VariantID_t *vid, db_id_t newid)
{
	assert(vid);
	assert(my_file);

	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	string q=string("insert into ")+vid->function_table_name + 
		string(" (function_id, file_id, name, stack_frame_size, out_args_region_size, use_frame_pointer, doip_id) ")+
		string(" VALUES (") + 
		string("'") + to_string(GetBaseID()) 		+ string("', ") + 
		string("'") + to_string(my_file->GetBaseID()) 	+ string("', ") + 
		string("'") + name 				+ string("', ") + 
		string("'") + to_string(stack_frame_size) 	+ string("', ") + 
	        string("'") + to_string(out_args_region_size) 	+ string("', ") + 
	        string("'") + to_string(use_fp) 	+ string("', ") + 
		string("'") + to_string(GetDoipID()) 		+ string("') ; ") ;

	return q;
}
