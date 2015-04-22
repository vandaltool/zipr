/*
 * Copyright (c) 2013, 2014 - University of Virginia 
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
#include <getopt.h>
#include <libgen.h>

#include "MEDS_AnnotationParser.hpp"
#include "transformutils.h"
#include "integertransform.hpp"
#include "integertransform32.hpp"
#include "integertransform64.hpp"
#include "pointercheck64.hpp"

// current convention
#define BINARY_NAME "a.ncexe"
#define ANNOTATION_SUFFIX ".infoannot"
#define SHARED_OBJECTS_DIR "shared_objects"

using namespace std;
using namespace libTransform;

bool saturating_arithmetic = false;
bool path_manip_detected = false; // deprecated
bool instrument_idioms = false;
bool warning_only = false;
bool check_pointers = false;

void usage()
{
	cerr << "Usage: integertransformdriver.exe <variant_id> <filtered_functions> <integer.warning.addresses> [--saturate] [--instrument-idioms] [--check-pointers] [--warning]"<<endl;
}

int parse_args(int p_argc, char* p_argv[])
{
	int option = 0;
	char options[] = "v:s:p:i";
	struct option long_options[] = {
		{"saturate", no_argument, NULL, 's'},
		{"instrument-idioms", no_argument, NULL, 'i'},
		{"warning", no_argument, NULL, 'w'},
		{"check-pointers", no_argument, NULL, 'c'},
		{NULL, no_argument, NULL, '\0'},         // end-of-array marker
	};

	while ((option = getopt_long(
		p_argc,
		p_argv,
		options,
		long_options,
		NULL)) != -1)
	{
		switch (option)
		{
			case 's':
			{
				saturating_arithmetic = true;
				printf("saturating arithmetic enabled\n");
				break;
			}
			case 'i':
			{
				printf("instrument idioms enabled\n");
				instrument_idioms = true;
				break;
			}
			case 'w':
			{
				printf("warning only mode\n");
				warning_only = true;
				break;
			}
			case 'c':
			{
				printf("check pointers mode\n");
				check_pointers = true;
				break;
			}
			default:
				return 1;
		}
	}
	return 0;
}

std::set<VirtualOffset> getInstructionWarnings(char *warningFilePath)
{
	std::set<VirtualOffset> warnings;
	ifstream warningsFile;

	warningsFile.open(warningFilePath);

	if (warningsFile.is_open())
	{
		while (!warningsFile.eof())
		{
			string address;
			getline(warningsFile, address);

			if (!address.empty())
			{
				VirtualOffset vo(address);
				warnings.insert(vo);

				cerr << "Detected warning address at: 0x" << hex << vo.getOffset() << endl;
			}
		}
	}

	warningsFile.close();

	cerr << "Detected a total of " << warnings.size() << " benign addresses" << endl;
	return warnings;
}

int main(int argc, char **argv)
{
	if(argc < 4)
	{
		usage();
		exit(1);
	}

	string programName(argv[0]);
	int variantID = atoi(argv[1]);
	set<string> filteredFunctions = getFunctionList(argv[2]);
	char *integerWarnings = argv[3];

	parse_args(argc, argv);

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
		FileIR_t *firp = new FileIR_t(*pidp, this_file);
		char *fileBasename = basename((char*)this_file->GetURL().c_str());

		assert(firp && pidp);

		try 
		{
			string annotationFilename;
			// need to map filename to integer annotation file produced by STARS
			// this should be retrieved from the IRDB but for now, we use files to store annotations
			// convention from within the peasoup subdirectory is:
			//      a.ncexe.infoannot
			//      shared_objects/<shared-lib-filename>.infoannot
			if (strcmp(fileBasename, BINARY_NAME) == 0)
				annotationFilename = string(BINARY_NAME) + string(ANNOTATION_SUFFIX);
			else
				annotationFilename = string(SHARED_OBJECTS_DIR) + "/" + fileBasename + ANNOTATION_SUFFIX;

			cerr << "annotation file: " << annotationFilename << endl;

			// parse MEDS integer annotations
			ifstream annotationFile(annotationFilename.c_str(), ifstream::in);
			if (!annotationFile.is_open())
			{
				cerr << "annotation file not found: " << annotationFilename.c_str() << endl;
				continue;
			}

			MEDS_AnnotationParser annotationParser(annotationFile);

			// this is now wrong as we're instrumenting shared libraries
			// we need to display file IDs along with the PC to distinguish between various libs
			std::set<VirtualOffset> warnings = getInstructionWarnings(integerWarnings); // keep track of instructions that should be instrumented as warnings (upon detection, print diagnostic & continue)

			MEDS_Annotations_t annotations = annotationParser.getAnnotations();

			cout << "integer transform driver: found " << annotations.size() << " annotations" << endl;

			// do the transformation

			libTransform::IntegerTransform *intxform = NULL;
			if(firp->GetArchitectureBitWidth()==64)
			{
				if (check_pointers)
					intxform = new PointerCheck64(pidp, firp, &annotations, &filteredFunctions, &warnings);
				else
					intxform = new IntegerTransform64(pidp, firp, &annotations, &filteredFunctions, &warnings);
			}
			else
			{
				intxform = new IntegerTransform32(pidp, firp, &annotations, &filteredFunctions, &warnings);
			}

			intxform->setSaturatingArithmetic(saturating_arithmetic);
			intxform->setPathManipulationDetected(path_manip_detected);
			intxform->setInstrumentIdioms(instrument_idioms);
			intxform->setWarningsOnly(warning_only);

			int exitcode = intxform->execute();

			if (exitcode == 0)
			{
				one_success = true;
				firp->WriteToDB();
				intxform->logStats();
				delete firp;
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
		pqxx_interface.Commit();

	return 0;
}
