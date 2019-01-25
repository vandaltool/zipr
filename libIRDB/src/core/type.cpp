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

#include "all.hpp" 
#include <utils.hpp>

using namespace std;
using namespace libIRDB;

bool BasicType_t::isNumericType() const
{
		auto type = getTypeID();
		return type == IRDB_SDK::itNumeric || 
		       type == IRDB_SDK::itInt     || 
		       type == IRDB_SDK::itFloat   || 
		       type == IRDB_SDK::itDouble  || 
		       type == IRDB_SDK::itChar;
}

/*
CREATE TABLE #TYP#
(
  type_id            integer,     
  type               integer DEFAULT 0,    -- possible types (0: UNKNOWN)
  name               text DEFAULT '',      -- string representation of the type
  ref_type_id        integer DEFAULT -1,   -- for aggregate types
  pos                integer DEFAULT -1,   -- for aggregate types, position in aggregate
  ref_type_id2       integer DEFAULT -1,   -- for func types
  doip_id            integer DEFAULT -1    -- the DOIP
);
*/

string BasicType_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);

	string q=string("insert into ")+fid->types_table_name + 
		string(" (type_id, type, name, ref_type_id, pos, ref_type_id2) ")+
		string(" VALUES (") + 
		string("'") + to_string(getBaseID()) + string("', ") + 
		string("'") + to_string(getTypeID()) + string("', ") + 
		string("'") + getName()              + string("','-1','-1','-1') ; ") ;

//    cout << "BasicType_t::WriteToDB(): " << q << endl;
	return q;
}

string PointerType_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid && getReferentType());
	string q=string("insert into ")+fid->types_table_name + 
		string(" (type_id, type, name, ref_type_id,pos,ref_type_id2) ")+
		string(" VALUES (") + 
		string("'") + to_string(getBaseID())                    + string("', ") + 
		string("'") + to_string(getTypeID())                    + string("', ") + 
		string("'") + getName()                                 + string("', ") +
		string("'") + to_string(getReferentType()->getBaseID()) + 
		string("','-1','-1') ; ") ;

//    cout << "PointerType_t::WriteToDB(): " << q << endl;
	return q;
}

string AggregateType_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);
	assert(getNumAggregatedTypes() > 0);

	string q;

	for (auto i = 0U; i < getNumAggregatedTypes(); ++i)
	{
		auto t = getAggregatedType(i);
		q+=string("insert into ")+fid->types_table_name + 
			string(" (type_id, type, name, ref_type_id, pos,ref_type_id2) ")+
			string(" VALUES (") + 
			string("'") + to_string(getBaseID())                    + string("', ") + 
			string("'") + to_string(getTypeID())                    + string("', ") + 
			string("'") + getName()                                 + string("', ") +
			string("'") + to_string(t->getBaseID()) + string("', ") +
			string("'") + to_string(i) +
			string("','-1') ; ") ;
	}

 //   cout << "AggregatedType_t::WriteToDB(): " << q << endl;
	return q;
}

void AggregateType_t::addAggregatedType(IRDB_SDK::Type_t *t, int pos)
{
	refTypes.push_back(t);

//	cout << "AggregatedType_t::AddAggregatedType(): new size: " << refTypes.size() << endl;
}

/*
CREATE TABLE #TYP#
(
  type_id            integer,     
  type               integer DEFAULT 0,    -- possible types (0: UNKNOWN)
  name               text DEFAULT '',      -- string representation of the type
  ref_type_id        integer DEFAULT -1,   -- for aggregate types
  pos                integer DEFAULT -1,   -- for aggregate types, position in aggregate
  ref_type_id2       integer DEFAULT -1,   -- for func types
  doip_id            integer DEFAULT -1    -- the DOIP
);
*/
string FuncType_t::WriteToDB(File_t *fid, db_id_t newid)
{
	assert(fid);
	assert(getReturnType());
	assert(getArgumentsType());
	string q=string("insert into ")+fid->types_table_name + 
		string(" (type_id, type, name, ref_type_id, ref_type_id2,pos) ")+
		string(" VALUES (") + 
		string("'") + to_string(getBaseID())                    + string("', ") + 
		string("'") + to_string(getTypeID())                    + string("', ") + 
		string("'") + getName()                                 + string("', ") +
		string("'") + to_string(getReturnType()->getBaseID()) + string("', ") +
		string("'") + to_string(getArgumentsType()->getBaseID()) + 
		string("','-1') ; ") ;

//  cout << "FuncType_t::WriteToDB(): " << q << endl;
	return q;
}

