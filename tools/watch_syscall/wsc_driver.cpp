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
	cerr<<"Usage: "<<name<<" --varid=<variant_id> --warn --warning_file=<file>\n"; 
}


int varid=0;
char* warning_filename=NULL;

int parse_args(int p_argc, char* p_argv[])
{
        int option = 0;
        char options[] = "v:w:";
        struct option long_options[] = {
                {"varid",     		required_argument,       NULL, 'v'},
                {"warning_file",       	required_argument,       NULL, 'w'},
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
				warning_filename=strdup(::optarg);
				cout<<"Using warning_file =  "<<warning_filename<<endl;
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

	cout<<"ret_shadow_stack.exe started\n";

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

			WSC_Instrument wsci(firp);

			bool success=wsci.FindInstructionsToProtect(warning_filename);
			success= success && wsci.execute();

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

        // if any integer transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface.Commit();
	}

        return one_fail;
}

