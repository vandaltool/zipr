/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> <target_cinderella_executable> <libc_function> <function_to_test>\n";
}

Function_t* findFunction(FileIR_t* firp, string functionName)
{
	FunctionSet_t functions = firp->GetFunctions();
	for (FunctionSet_t::iterator it = functions.begin(); it != functions.end(); ++it)
	{
		Function_t *fn = *it;	
		if (fn && fn->GetName() == functionName)
			return fn;
	}
	return NULL;
}

extern int test_prince(string targetName, string functionName, Function_t* fn);

int main(int argc, char **argv)
{
	if(argc != 5)
  	{
		usage(argv[0]);
		exit(1);
	}

	string programName(argv[0]);
	int variantID = atoi(argv[1]);
	string cinderellaExecutable(argv[2]);
	string libcFunction(argv[3]);
	string functionName(argv[4]);

	VariantID_t *pidp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	for(set<File_t*>::iterator it=pidp->GetFiles().begin(); 
		it!=pidp->GetFiles().end(); 
		++it)
	{
		File_t* this_file = *it;
		try
		{
			FileIR_t *firp = new FileIR_t(*pidp, this_file);
			assert(firp && pidp);

			Function_t *fn = findFunction(firp, functionName);
			if (fn) {
				cout << "prince_driver: cinderella_exec: " << cinderellaExecutable << "  libc_function: " << functionName << "  candidate_fn: " << fn->GetName() << endl;
				return test_prince(cinderellaExecutable, libcFunction, fn);
			}
			else
				return 1;
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
		}
	} 

	return 1; // error
}

