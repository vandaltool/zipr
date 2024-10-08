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



#include <irdb-core>
#include <iostream>
#include <fstream>

#include <stdlib.h>

using namespace IRDB_SDK;
using namespace std;

int main(int argc, char* argv[])
{

	if(argc!=3)
	{
		cerr<<"Usage: clone <vid> <output.id>"<<endl;
		exit(-1);
	}


	/* setup the interface to the sql server */
	auto pqxx_interface=pqxxDB_t::factory();
	BaseObj_t::setInterface(pqxx_interface.get());


	try 
	{
		auto pidp=VariantID_t::factory(atoi(argv[1]));
		assert(pidp->isRegistered()==true);

		auto newpidp=pidp->clone();
		assert(newpidp->isRegistered()==true);

		cout<<"Cloned Variant is: "<<*newpidp << endl;

		// commit the changes to the db if all went well 
		pqxx_interface->commit();

		auto  newpid_id=newpidp->getBaseID();
		ofstream f;
		f.open(argv[2]);
		if(!f.is_open())
		{
			cerr<<"Cannot open output file"<<endl;
			exit(-1);
		}
		f<<std::dec<<newpid_id<<endl;
		f.close();

	}
	catch (const DatabaseError_t &pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }



	return 0;
}
