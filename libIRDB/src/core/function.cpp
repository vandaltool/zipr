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

Function_t::Function_t(db_id_t id, std::string myname, int size, int oa_size, bool useFP, FuncType_t *fn_type, Instruction_t* entry)
	: BaseObj_t(NULL), entry_point(entry)
{
	SetBaseID(id);
	name=myname;
	stack_frame_size=size;	
	out_args_region_size=oa_size;
    use_fp = useFP;
	function_type = fn_type;
}

string Function_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);

	int entryid=NOT_IN_DATABASE;
	if(entry_point)
		entryid=entry_point->GetBaseID();

	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

    int function_type_id = NOT_IN_DATABASE;
	if (GetType())
		function_type_id = GetType()->GetBaseID();	 

	string q=string("insert into ")+fid->function_table_name + 
		string(" (function_id, entry_point_id, name, stack_frame_size, out_args_region_size, use_frame_pointer, type_id, doip_id) ")+
		string(" VALUES (") + 
		string("'") + to_string(GetBaseID()) 		  + string("', ") + 
		string("'") + to_string(entryid) 		  + string("', ") + 
		string("'") + name 				  + string("', ") + 
		string("'") + to_string(stack_frame_size) 	  + string("', ") + 
	        string("'") + to_string(out_args_region_size) 	  + string("', ") + 
	        string("'") + to_string(use_fp) 		  + string("', ") + 
	        string("'") + to_string(function_type_id) 	  + string("', ") + 
		string("'") + to_string(GetDoipID()) 		  + string("') ; ") ;

	return q;
}

int Function_t::GetNumArguments()
{
	if (!function_type) return -1;
	AggregateType_t *argtype = function_type->GetArgumentsType();
	if (argtype)
		return argtype->GetNumAggregatedTypes();
	else
		return -1;
}
