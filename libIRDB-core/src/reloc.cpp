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
#include <irdb-util>

using namespace std;

vector<std::string> Relocation_t::WriteToDB(File_t* fid, BaseObj_t* myinsn)
{
        db_id_t wrt_id=wrt_obj ? wrt_obj->getBaseID() : BaseObj_t::NOT_IN_DATABASE;
/*
        string q;
        q ="insert into " + fid->relocs_table_name;
        q+="(reloc_id,reloc_offset,reloc_type,instruction_id,addend,wrt_id,doip_id) "+
                string(" VALUES (") +
*/
	return {
                to_string(getBaseID()),
                to_string(offset),
                (type),
                to_string(myinsn->getBaseID()),
                to_string(addend),
                to_string(wrt_id),
                to_string(getDoipID())
		};
}

