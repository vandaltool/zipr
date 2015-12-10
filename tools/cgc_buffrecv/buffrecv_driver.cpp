/*
 * Copyright (c) 2015 - University of Virginia
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
 */

#include <stdlib.h>
#include <fstream>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <libIRDB-core.hpp>
#include "buffrecv_instrument.hpp"

using namespace std;
using namespace libIRDB;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" --varid=<variant_id>\n"; 
}


int varid=0;

int parse_args(int p_argc, char* p_argv[])
{
	int option = 0;
	char options[] = "v:";
	struct option long_options[] = {
		{"varid", required_argument, NULL, 'v'},
		{NULL, no_argument, NULL, '\0'},         // end-of-array marker
	};

	while ((option = getopt_long(
		p_argc,
		p_argv,
		options,
		long_options,
		NULL)) != -1)
	{
		printf("Found option %c\n", option);
		switch (option)
		{
			case 'v':
			{
				varid=atoi(::optarg);	
				cout<<"Transforming variant "<<dec<<varid<<endl;
				break;
			}
			default:
				return 1;
		}
	}
	return 0;
}


int main(int argc, char **argv)
{
	if(0 != parse_args(argc,argv))
	{
		usage(argv[0]);
		exit(1);
	}

	string programName(argv[0]);
	int variantID = varid;

	VariantID_t *pidp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	cout << argv[0] << " started\n";

	bool success = false;
	bool one_success = false;
	bool one_fail=false;

	for(set<File_t*>::iterator it=pidp->GetFiles().begin();
		it!=pidp->GetFiles().end();
		++it)
	{
		File_t* this_file = *it;
		try
		{
			FileIR_t *firp = new FileIR_t(*pidp, this_file);
	
			cout<<"Transforming "<<this_file->GetURL()<<endl;
	
			assert(firp && pidp);

			BuffRecv_Instrument wsci(firp);

			success = wsci.execute();

			if (success)
			{	
				cout<<"Writing changes for "<<this_file->GetURL()<<endl;
				one_success = true;
	
				firp->WriteToDB();
			}
			else
			{
				one_fail=true;
				cout<<"Skipping (no changes) "<<this_file->GetURL()<<endl;
			}

			delete firp;
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
		}
	} // end file iterator

	// if any transforms for any files succeeded, we commit
	if (one_success)
	{
		cout<<"Commiting changes...\n";
		pqxx_interface.Commit();
	}

	return one_fail;
}

