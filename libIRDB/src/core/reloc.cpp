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

using namespace std;

std::string Relocation_t::WriteToDB(File_t* fid, BaseObj_t* myinsn)
{
        string q;
        db_id_t wrt_id=wrt_obj ? wrt_obj->GetBaseID() : BaseObj_t::NOT_IN_DATABASE;
        q ="insert into " + fid->relocs_table_name;
        q+="(reloc_id,reloc_offset,reloc_type,instruction_id,wrt_id,addend,doip_id) "+
                string(" VALUES (") +
                string("'") + to_string(GetBaseID())          + string("', ") +
                string("'") + to_string(offset)               + string("', ") +
                string("'") + (type)                          + string("', ") +
                string("'") + to_string(myinsn->GetBaseID())  + string("', ") +
                string("'") + to_string(wrt_id)  + string("', ") +
                string("'") + to_string(addend)  + string("', ") +
                string("'") + to_string(GetDoipID())          + string("') ; ") ;
        return q;
}

