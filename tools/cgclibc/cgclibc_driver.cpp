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
#include "cgclibc.hpp"

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id> [--dominator] [--cluster] [--positive_inferences <filename> ]\n"; 
}

static bool g_dominator = false;
static bool g_cluster = false;
static std::string g_positiveFile = "";

void parseOptions(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "--dominator") == 0)
			g_dominator = true;
		if (strcmp(argv[i], "--cluster") == 0)
			g_cluster = true;
		else if (strcmp(argv[i], "--positive-inferences") == 0)
			g_positiveFile = argv[++i];
	}
}

int main(int argc, char **argv)
{
        if(argc < 2)
        {
                usage(argv[0]);
                exit(1);
        }

	parseOptions(argc, argv);

        string programName(argv[0]);
        int variantID = atoi(argv[1]);

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);

        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                try
                {
                	FileIR_t *firp = new FileIR_t(*pidp, this_file);
	
                	assert(firp && pidp);

			CGC_libc cgclibc(firp);

			if (g_dominator) cgclibc.enableDominanceHeuristic();
			if (g_cluster) cgclibc.enableClusteringHeuristic();

			if (g_positiveFile.size() > 0)
				cgclibc.setPositiveInferences(g_positiveFile);

			bool success=cgclibc.execute();

                        if (success)
                        {
                                one_success = true;
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

        return 0;
}

