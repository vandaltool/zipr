
#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;
using namespace std;

Instruction_t::Instruction_t() :
	BaseObj_t(NULL), 
	data(""),
	comment(""),
	file_id(NOT_IN_DATABASE)
{
	SetBaseID(NOT_IN_DATABASE);
	my_address=NULL;
	my_function=NULL;
	orig_address_id=NOT_IN_DATABASE;
	fallthrough=NULL;
	target=NULL;
}

Instruction_t::Instruction_t(db_id_t id, 
		AddressID_t *addr, 
		Function_t *func, 
		db_id_t my_file_id, 
		db_id_t orig_id, 
                std::string thedata, 
		std::string my_comment, 
		db_id_t doip_id) :

	BaseObj_t(NULL), 
	data(thedata),
	comment(my_comment),
	file_id(my_file_id)
{
	SetBaseID(id);
	my_address=addr;
	my_function=func;
	orig_address_id=orig_id;
	fallthrough=NULL;
	target=NULL;
}


string Instruction_t::WriteToDB(VariantID_t *vid, db_id_t newid)
{
	assert(vid);
	assert(my_address);

	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	db_id_t func_id=NOT_IN_DATABASE;
	if(my_function)
		func_id=my_function->GetBaseID();

	db_id_t ft_id=NOT_IN_DATABASE;
	if(fallthrough)
		ft_id=fallthrough->GetBaseID();

	db_id_t targ_id=NOT_IN_DATABASE;
	if(target)
		targ_id=target->GetBaseID();

        string q=
		string("insert into ")+vid->instruction_table_name +
                string(" (instruction_id, address_id, parent_function_id, file_id, orig_address_id, fallthrough_address_id, target_address_id, data, comment, doip_id) ")+
                string(" VALUES (") +
                string("'") + to_string(GetBaseID())            	+ string("', ") +
                string("'") + to_string(my_address->GetBaseID())   	+ string("', ") +
                string("'") + to_string(func_id)            		+ string("', ") +
                string("'") + to_string(file_id)            		+ string("', ") +
                string("'") + to_string(orig_address_id)         	+ string("', ") +
                string("'") + to_string(ft_id)         			+ string("', ") +
                string("'") + to_string(targ_id)         		+ string("', ") +
                string("E'") + pqxx::escape_binary(data) + "'::bytea"   + string(" , ") + // no ticks for this field
											  // also need to append ::bytea
                string("'") + comment                              	+ string("', ") +
                string("'") + to_string(GetDoipID())            	+ string("') ; ") ;

	return q;
}

