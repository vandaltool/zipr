#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>

#include "MEDS_AnnotationParser.hpp"
#include "transformutils.h"
#include "integertransform.hpp"

using namespace std;

void usage()
{
	cerr << "Usage: integertransformdriver.exe <variant_id> <annotation_file> <filtered_functions> <integer.warning.addresses> [--saturate]"<<endl;
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

	cerr << "Detected a total of " << warnings.size() << " addresses" << endl;
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

main(int argc, char **argv)
{
	if(argc < 4)
	{
		usage();
		exit(1);
	}

	string programName(argv[0]);
	int variantID = atoi(argv[1]);
	char *annotationFilename = argv[2];
	set<string> filteredFunctions = getFunctionList(argv[3]);
	char *integerWarnings = argv[4];

	VariantID_t *pidp=NULL;
	VariantIR_t *virp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	try 
	{
			cerr << "Getting variand id" << endl;
		pidp=new VariantID_t(variantID);
		assert(pidp->IsRegistered()==true);

		// read the db  
			cerr << "Reading IR DB" << endl;
		virp=new VariantIR_t(*pidp);

		assert(virp && pidp);

		cerr << "Parse annotation file" << endl;
		// parse MEDS integer annotations
		ifstream annotationFile(annotationFilename, ifstream::in);
		MEDS_AnnotationParser annotationParser(annotationFile);
		cerr << "Done parsing annotation file" << endl;

		std::set<VirtualOffset> warnings = getInstructionWarnings(integerWarnings); // keep track of instructions that should be instrumented as warnings (upon detection, print diagnostic & continue)

		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> annotations = annotationParser.getAnnotations();
			cerr << "Got all annotations" << endl;

		// do the transformation
			cerr << "Do the integer transform" << endl;
		libTransform::IntegerTransform integerTransform(pidp, virp, &annotations, &filteredFunctions, &warnings);
		integerTransform.setSaturatingArithmetic(isSaturatingArithmeticOn(argc, argv));
		int exitcode = integerTransform.execute();
		if (exitcode == 0)
		{
			pqxx_interface.Commit();
			delete virp;
			delete pidp;
		}
		return exitcode;
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
}
