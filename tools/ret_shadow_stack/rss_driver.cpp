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
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "MEDS_AnnotationParser.hpp"
#include "rss_instrument.hpp"

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;


/* options */
bool do_zipr=false;
int varid=-1;


#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"


void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id>\n"; 
}

int parse_args(int p_argc, char* p_argv[])
{
        int option = 0;
        char options[] = "v:z";
        struct option long_options[] = {
                {"varid", required_argument, NULL, 'v'},
                {"zipr", no_argument, NULL, 'z'},
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
                        case 'z':
			{
				do_zipr=true;	
                                break;
			}
			default:
				return 1;

		}
	}

	// varid is required.
	if(varid==-1)
		return 1;
	return 0;
}


int main(int argc, char **argv)
{
        if(parse_args(argc,argv)!=0)
        {
                usage(argv[0]);
                exit(1);
        }

        string programName(argv[0]);
        int variantID =varid;

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);

	cout<<"ret_shadow_stack.exe started\n";

        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                FileIR_t *firp = new FileIR_t(*pidp, this_file);
                char *fileBasename = basename((char*)this_file->GetURL().c_str());

		cout<<"Transforming "<<this_file->GetURL()<<endl;

                assert(firp && pidp);

                try
                {
                        MEDS_AnnotationParser annotationParser;
                        string annotationFilename;
                        // need to map filename to integer annotation file produced by STARS
                        // this should be retrieved from the IRDB but for now, we use files to store annotations
                        // convention from within the peasoup subdirectory is:
                        //      a.ncexe.infoannot
                        //      shared_objects/<shared-lib-filename>.infoannot
                        if (strcmp(fileBasename, BINARY_NAME) == 0)
                                annotationFilename = string(BINARY_NAME);
                        else
                                annotationFilename = string(SHARED_OBJECTS_DIR) + "/" + fileBasename ;

                        cerr << "annotation file: " << annotationFilename << endl;
			annotationParser.parseFile(annotationFilename+".annot");
			annotationParser.parseFile(annotationFilename+".infoannot");
			annotationParser.parseFile(annotationFilename+".STARScallreturn");


			RSS_Instrument rssi(firp, &annotationParser, do_zipr);


			int exitcode=rssi.execute();

                        if (exitcode == 0)
                        {
				cout<<"Writing changes for "<<this_file->GetURL()<<endl;
                                one_success = true;
                                firp->WriteToDB();
                                delete firp;
                        }
			else
			{
				cout<<"Skipping (no changes) "<<this_file->GetURL()<<endl;
			}
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

        return 0;
}

