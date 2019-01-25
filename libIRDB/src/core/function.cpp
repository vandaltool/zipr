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

using namespace libIRDB;
using namespace std;

Function_t::Function_t(db_id_t id, std::string myname, int size, int oa_size, bool useFP, bool isSafe, IRDB_SDK::FuncType_t *fn_type, IRDB_SDK::Instruction_t* entry)
	: BaseObj_t(NULL), entry_point(dynamic_cast<Instruction_t*>(entry))
{
	setBaseID(id);
	name=myname;
	stack_frame_size=size;	
	out_args_region_size=oa_size;
	use_fp = useFP;
	setSafe(isSafe);
	function_type = dynamic_cast<FuncType_t*>(fn_type);
	if(entry) assert(entry_point);
	if(fn_type) assert(function_type);
}

string Function_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);

	int entryid=NOT_IN_DATABASE;
	if(entry_point)
		entryid=entry_point->getBaseID();

	if(getBaseID()==NOT_IN_DATABASE)
		setBaseID(newid);

    int function_type_id = NOT_IN_DATABASE;
	if (getType())
		function_type_id = getType()->getBaseID();	 

	string q=string("insert into ")+fid->function_table_name + 
		string(" (function_id, entry_point_id, name, stack_frame_size, out_args_region_size, use_frame_pointer, is_safe, type_id, doip_id) ")+
		string(" VALUES (") + 
		string("'") + to_string(getBaseID()) 		  + string("', ") + 
		string("'") + to_string(entryid) 		  + string("', ") + 
		string("'") + name 				  + string("', ") + 
		string("'") + to_string(stack_frame_size) 	  + string("', ") + 
	        string("'") + to_string(out_args_region_size) 	  + string("', ") + 
	        string("'") + to_string(use_fp) 		  + string("', ") + 
	        string("'") + to_string(is_safe) 		  + string("', ") + 
	        string("'") + to_string(function_type_id) 	  + string("', ") + 
		string("'") + to_string(getDoipID()) 		  + string("') ; ") ;

	return q;
}

int Function_t::getNumArguments() const
{
	if (!function_type) return -1;
	auto argtype = function_type->getArgumentsType();
	if (argtype)
		return argtype->getNumAggregatedTypes();
	else
		return -1;
}

void Function_t::setType(IRDB_SDK::FuncType_t *t)  
{ 
	function_type = dynamic_cast<FuncType_t*>(t); 
	if(t) 
		assert(function_type); 
}

IRDB_SDK::FuncType_t* Function_t::getType() const  
{ 
	return function_type; 
}

