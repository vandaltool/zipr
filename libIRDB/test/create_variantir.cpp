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



#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

int main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: create_variant <name>"<<endl;
		exit(-1);
	}


	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);


	VariantID_t *pidp=NULL;
	FileIR_t * virp=NULL;
	try 
	{
		pidp=new VariantID_t();

		assert(pidp->IsRegistered()==false);

		pidp->SetName(argv[1]);

		cout<<"New Variant, before registration, is: "<<*pidp << endl;

		pidp->Register();

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after registration, is: "<<*pidp << endl;

		pidp->WriteToDB();

		cout<<"New Variant, after writing to DB, is: "<<*pidp << endl;

		virp=new FileIR_t(*pidp);


		// commit the changes to the db if all went well 
		pqxx_interface.Commit();


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	delete virp;
	delete pidp;
	return 0;
}
