#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "MEDS_AnnotationParser.hpp"
#include "transformutils.h"
#include "integertransform.hpp"

// current convention
#define BINARY_NAME "a.ncexe"
#define ANNOTATION_SUFFIX ".infoannot"
#define SHARED_OBJECTS_DIR "shared_objects"

using namespace std;

void usage()
{
	cerr << "Usage: integertransformdriver.exe <variant_id> <filtered_functions> <integer.warning.addresses> [--saturating-arithmetic] [--path-manip-detected]"<<endl;
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

bool isSaturatingArithmeticOn(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i)
	{
		if (strncasecmp(argv[i], "--saturat", strlen("--saturat")) == 0)
			return true;
	}

	return false;
}

bool isWarningsOnly(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i)
	{
		if (strncasecmp(argv[i], "--warning", strlen("--warning")) == 0)
			return true;
	}

	return false;
}

bool isPathManipDetected(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i)
	{
		if (strncasecmp(argv[i], "--path-manip", strlen("--path-manip")) == 0)
			return true;
	}

	return false;
}

main(int argc, char **argv)
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

	VariantID_t *pidp=NULL;
	FileIR_t *virp=NULL;

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

			std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> annotations = annotationParser.getAnnotations();

			// do the transformation
			libTransform::IntegerTransform integerTransform(pidp, firp, &annotations, &filteredFunctions, &warnings);
			integerTransform.setSaturatingArithmetic(isSaturatingArithmeticOn(argc, argv));
			integerTransform.setPathManipulationDetected(isPathManipDetected(argc, argv));
			integerTransform.setWarningsOnly(isWarningsOnly(argc, argv));

			int exitcode = integerTransform.execute();
			if (exitcode == 0)
			{
				one_success = true;
				firp->WriteToDB();
				integerTransform.logStats();
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
