/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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
#include <fstream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

Function_t* findFunction(FileIR_t* firp, string funcName)
{
	assert(firp);

	for(
		set<Function_t*>::iterator it=firp->GetFunctions().begin();
		it!=firp->GetFunctions().end();
		++it
	   )
	{
		Function_t* func=*it;
		if (!func) continue;
		if (func->GetName() == funcName)
		{
			return func;
		}
	}

	return NULL;
}

void mark_function_safe(FileIR_t* firp, const std::string& function_file)
{
	assert(firp);

	std::ifstream ffile(function_file);
	std::string fn;
	while (std::getline(ffile, fn))
	{
		cerr << "Want to mark: " << fn << " as safe" << endl;
		Function_t *f = findFunction(firp, fn);
		if (f)
		{
			cerr << "Found in IRDB: Marking " << f->GetName() << " as safe" << endl;
			f->SetSafe(true);
		}
	}

	firp->WriteToDB();
}

int main(int argc, char* argv[])
{
	if(argc!=3)
	{
		cerr<<"Usage: mark_function_safe <id> <file_with_functions_to_mark_safe>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t *firp=NULL;
	string function_files(argv[2]);

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{
		pidp=new VariantID_t(atoi(argv[1]));
		assert(pidp->IsRegistered()==true);

		for(set<File_t*>::iterator it=pidp->GetFiles().begin(); it!=pidp->GetFiles().end(); ++it)
		{
			File_t* this_file=*it;
			assert(this_file);

			// only do main file for now
			if(this_file!=pidp->GetMainFile())
				continue;

			// read the db  
			firp=new FileIR_t(*pidp,this_file);
			
			// mark as safe
			mark_function_safe(firp, function_files);

			delete firp;
		}
		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cout<<"Done!"<<endl;

	delete pidp;
	return 0;
}

