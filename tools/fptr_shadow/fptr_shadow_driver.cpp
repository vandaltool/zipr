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
// #include <libgen.h>

#include "MEDS_AnnotationParser.hpp"
#include "fptr_shadow_instrument64.hpp"

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;

#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"

void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id>\n"; 
}

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		usage(argv[0]);
		exit(1);
	}

	string programName(argv[0]);
	int variantID = atoi(argv[1]);

	VariantID_t *pidp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	pidp=new VariantID_t(variantID);
	assert(pidp->IsRegistered()==true);

	cout<<"shadow_fptr.exe started\n";

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

			annotationFilename += ".fptrannot";
			cerr << "annotation file: " << annotationFilename << endl;
			annotationParser.parseFile(annotationFilename);
			cerr << "done parsing file" << endl;

			FPTRShadow_Instrument64 fptrShadow64(firp, &annotationParser);

			bool success = fptrShadow64.execute();

			if (success)
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

	if (one_success)
	{
		cout<<"Commiting changes...\n";
		pqxx_interface.Commit();
	}

	return 0;
}

