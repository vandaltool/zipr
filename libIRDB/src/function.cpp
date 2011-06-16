#include <libIRDB.hpp>
#include <utils.hpp>
#include <stdlib.h>
using namespace libIRDB;



Function_t::Function_t(db_id_t id, std::string myname, int size)
	: BaseObj_t(NULL)
{
	SetBaseID(id);
	name=myname;
	stack_frame_size=size;	
}


void Function_t::WriteToDB()
{
assert(0);
}
