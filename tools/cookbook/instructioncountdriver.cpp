#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>

#include "MEDS_AnnotationParser.hpp"
#include "transformutils.h"
#include "instructioncount.hpp"

using namespace std;

void usage(string programName)
{
	cerr << "Usage: " << programName << " <variant id> <annotation file>" <<endl;
}

int main(int argc, char **argv)
{	
	string programName(argv[0]);
	char *strtolError = NULL;
	int transformExitCode = 0;
	int variantID = -1; 
	set<string> filteredFunctions;
	VariantID_t *pidp = NULL;
	FileIR_t *virp = NULL;
	pqxxDB_t pqxx_interface;
	File_t *fileId;
	std::set<File_t*> files;

	/*
	 * Check that we've been called correctly:
	 * <program> <variant id> <annotation file>
	 */
	if(argc < 3)
	{
		usage(programName);
		exit(1);
	}
	variantID = strtol(argv[1], &strtolError, 10);
	if (*strtolError != '\0')
	{
		cerr << "Invalid variantID: " << argv[1] << endl;
		exit(1);
	}

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	try 
	{
		/*
		 * Read information about the program
		 * variant from the database and check
		 * some important assumptions.
		 */
		pidp=new VariantID_t(variantID);
		assert(pidp && pidp->IsRegistered()==true);

		/*
		 * Loop through the variant's files and match
		 * it to the one given as a parameter.
		 */
		files = pidp->GetFiles();
		for (set<File_t*>::iterator it=files.begin();
			it!=files.end();
			++it
			) 
		{
			const char *name = (*it)->GetURL().c_str();
			if (strstr(name, argv[2]) != NULL) 
			{
				fileId = *it;
				break;
			}
		}
		assert(fileId);
		virp=new FileIR_t(*pidp, fileId);
		assert(virp);

		/*
		 * Create a transformation and then
		 * invoke its execution.
		 */
		libTransform::InstructionCount instructionCount(pidp, virp, &filteredFunctions);
		transformExitCode = instructionCount.execute();
		/*
		 * If everything about the transformation
		 * went okay, then we will write the updated
		 * set of instructions to the database.
		 */
		if (transformExitCode == 0)
		{
			virp->WriteToDB();
			pqxx_interface.Commit();
			delete virp;
			delete pidp;
		}
		else
		{
			cerr << programName << ": transform failed. Check logs." << endl;
		}
		return transformExitCode;
	}
	catch (DatabaseError_t pnide)
	{
		cerr << programName << ": Unexpected database error: " << pnide << endl;
		exit(1);
	}
	catch (...)
	{
		cerr << programName << ": Unexpected error" << endl;
		exit(1);
	}
	return 1;
}
