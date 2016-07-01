/*
 * Copyright (c) 2014 - Zephyr Software
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

#include <stdlib.h>
#include <fstream>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <libIRDB-core.hpp>
#include "wsc_instrument.hpp"

using namespace std;
using namespace libIRDB;


#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"


void usage(char* name)
{
	cerr<<"Usage: "<<name<<" "<<endl
		<<"--varid <varid>	 		specify the variant ID in the IRDB"<<endl
		<<"--warning_file <cso file>		specify a list of warnings from codesonar (or other sources) -- csv format, .cso extension"<<endl
		<<"--do_sandboxing			do callback-based sandboxing of instructions listed in warning file (or no sandboxing for insns not listed in cso file)"<<endl
		<<"--do_reverse_sandboxing		do faster sandboxing that stops an insn from accessing the flags page.  "<<endl
		<<"--do_promiscuous_sandboxing 		sandbox (or reverse sandbox) instructions not listed in cso file"<<endl
		<<"--do_input_filtering			add a 64-byte limit to cgc recieve call  buffer sizes."<<endl
		<<"--do_smart_cgc_sandboxing		do bounds checking or sandboxing on .cso listed insns (or all insns with promiscuous mode)"<<endl;
}


int varid=0;
std::set<string> warning_filename;
bool do_sandboxing = false;
bool do_reverse_sandboxing = false;
bool do_input_filtering = false;
bool do_promiscuous_sandboxing = false;
bool do_smart = false;

int parse_args(int p_argc, char* p_argv[])
{
	int option = 0;
	char options[] = "v:w:s:p:i:r";
	struct option long_options[] = {
		{"varid", required_argument, NULL, 'v'},			// specify the variant ID in the IRDB
		{"warning_file", required_argument, NULL, 'w'},			// specify a list of warnings from codesonar (or other sources) -- csv format, .cso extension
		{"do_sandboxing", no_argument, NULL, 's'},			// do callback-based sandboxing of instructions listed in warning file (or no sandboxing for insns not listed in cso file)
		{"do_reverse_sandboxing", no_argument, NULL, 'r'},		// do faster sandboxing that stops an insn from accessing the flags page.  
		{"do_promiscuous_sandboxing", no_argument, NULL, 'p'},		// sandbox (or reverse sandbox) instructions not listed in cso file
		{"do_input_filtering", no_argument, NULL, 'i'},			// add a 64-byte limit to cgc recieve call  buffer sizes.
		{"do_smart_cgc_sandboxing", no_argument, NULL, 'c'},		// do bounds checking or sandboxing on .cso listed insns (or all insns with promiscuous mode)
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
			case 'w':
			{
				string f(::optarg);
				cout<<"Using warning_file =  "<<f<<endl;
				warning_filename.insert(f);
				break;
			}
			case 'r':
				do_reverse_sandboxing = true;
				break;
			case 's':
				do_sandboxing = true;
				break;
			case 'i':
				do_input_filtering = true;
				break;
			case 'p':
				do_promiscuous_sandboxing = true;
				break;
			case 'c':
				do_smart = true;
				break;
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
	bool cso_file_success = false;
	bool one_cso_file_fail = false;

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

			WSC_Instrument wsci(firp);

			wsci.SetSandboxing(do_sandboxing);
			wsci.SetReverseSandboxing(do_reverse_sandboxing);
			wsci.SetPromiscuousSandboxing(do_promiscuous_sandboxing);
			wsci.SetInputFiltering(do_input_filtering);
			wsci.SetSmartMode(do_smart);

			int numInstructions = 0;
			cso_file_success = wsci.FindInstructionsToProtect(warning_filename, numInstructions);
		
			if (!cso_file_success)
				one_cso_file_fail = true;

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

			wsci.displayStatistics(cout);

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

	return one_fail || one_cso_file_fail;
}

