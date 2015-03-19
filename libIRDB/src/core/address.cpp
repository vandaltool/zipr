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


string AddressID_t::WriteToDB(File_t *fid, db_id_t newid, bool p_withHeader)
{
        assert(fid);
	
	if(GetBaseID()==NOT_IN_DATABASE)
		SetBaseID(newid);

	string q;
	if (p_withHeader)
        q=string("insert into ")+fid->address_table_name + 
			string("(address_id , file_id , vaddress_offset , doip_id)") +
			string(" values ");
	else
		q = ",";
			
	q +=
		string("(") + 
		string("'") + to_string(GetBaseID()) + string("', ") + 
		string("'") + to_string(fileID) + string("', ") + 
		string("'") + to_string(virtual_offset) + string("', ") + 
		string("'") + to_string(GetDoipID()) + string("' ") + 
		string(")");

	return q;
}

