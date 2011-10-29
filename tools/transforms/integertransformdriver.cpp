#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>

#include "MEDS_AnnotationParser.hpp"
#include "transformutils.h"
#include "integertransform.hpp"

using namespace std;

void usage()
{
	cerr << "Usage: integertransformdriver.exe <variant_id> <annotation_file> <filtered_functions>"<<endl;
}

main(int argc, char **argv)
{
	if(argc < 3)
	{
		usage();
		exit(1);
	}

	string programName(argv[0]);
	int variantID = atoi(argv[1]);
	char *annotationFilename = argv[2];
	set<string> filteredFunctions = getFunctionList(argv[3]);

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

		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> annotations = annotationParser.getAnnotations();
			cerr << "Got all annotations" << endl;

		// do the transformation
			cerr << "Do the integer transform" << endl;
		libTransform::IntegerTransform integerTransform(pidp, virp, &annotations, &filteredFunctions);
		int exitcode = integerTransform.execute();
		if (exitcode == 0)
		{
			cerr << "Do not commit to DB for now" << endl;
			string aspri_filename("test.int.aspri");
			ofstream aspriFile;
			aspriFile.open(aspri_filename.c_str());
			if(!aspriFile.is_open())
			{
				fprintf(stderr, "integertransformdriver: Could not open: %s\n", aspri_filename.c_str());
				throw;
			}

			fprintf(stderr, "integertransformdriver: generating aspri file: %s\n", aspri_filename.c_str());
			virp->GenerateSPRI(aspriFile); // p1.xform/<function_name>/a.irdb.aspri
			aspriFile.close();

//			pqxx_interface.Commit();
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
