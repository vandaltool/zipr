
#include <iostream>
#include "PNStackLayoutInference.hpp"
#include "P1Inference.hpp"
#include "OffsetInference.hpp"
#include "ScaledOffsetInference.hpp"
#include "DirectOffsetInference.hpp"
#include "PNTransformDriver.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <cstdlib>

using namespace std;
using namespace libIRDB;

set<string> getFunctionList(char *p_filename)
{
	set<string> functionList;

	ifstream candidateFile;
	candidateFile.open(p_filename);

	if(candidateFile.is_open())
	{
		while(!candidateFile.eof())
		{
			string functionName;
			getline(candidateFile, functionName);

			functionList.insert(functionName);
		}

		candidateFile.close();
	}

	return functionList;
}


int main(int argc, char **argv)
{
    if(argc!=4)
    {
	cerr<<"Usage: p1transform.exe <variantid> <bed_script> <file containing name of blacklisted functions>"<<endl;
	exit(-1);
    }
    else
    {
	cout << "bed_script: " << argv[2] << " blacklist: " << argv[3] << endl;
    }

    VariantID_t *pidp=NULL;
    VariantIR_t *virp=NULL;
  
    int progid = atoi(argv[1]);
    char *BED_script = argv[2];

    //setup the interface to the sql server 
    pqxxDB_t pqxx_interface;
    BaseObj_t::SetInterface(&pqxx_interface);

    try
    {
	// read the variant ID using variant id number = atoi(argv[1])
	pidp=new VariantID_t(progid);
      
	// verify that we read it correctly.
	assert(pidp->IsRegistered()==true);
      
	// read the IR from the db
	virp=new VariantIR_t(*pidp);
    }
    catch (DatabaseError_t pnide)
    {
	cout<<"Unexpected database error: "<<pnide<<endl;
	exit(-1);
    }
    
    set<std::string> blackListOfFunctions;

    blackListOfFunctions = getFunctionList(argv[3]);
/*
    vector<std::string> functionsTransformed;
    int numFuncProcessed = 0;
    int numFuncFiltered = 0;
    int numFuncBEDfailed = 0;
    int numFuncBEDpassed = 0;
    int numFunP1skipped = 0;
*/

    string report;
    try 
    {
	PNTransformDriver transform_driver;

	transform_driver.AddBlacklist(blackListOfFunctions);
	OffsetInference *offset_inference = new OffsetInference();
	DirectOffsetInference *direct_offset_inference = new DirectOffsetInference(offset_inference);
	ScaledOffsetInference *scaled_offset_inference = new ScaledOffsetInference(offset_inference);
	P1Inference *p1 = new P1Inference(offset_inference);

	//Add new boundary inferences here


	transform_driver.AddInference(offset_inference);
	transform_driver.AddInference(direct_offset_inference);
	transform_driver.AddInference(scaled_offset_inference);
	transform_driver.AddInference(p1);

	transform_driver.GenerateTransforms(virp,BED_script,progid);

/*
        cerr << "P1: " << func->GetName() << " processed: " << numFuncProcessed << "/" << virp->GetFunctions().size() << " filtered: " << numFuncFiltered << " BED-passed: " << numFuncBEDpassed << " BED-failed: " << numFuncBEDfailed << " P1-skipped: " << numFunP1skipped << endl;
*/
	pqxx_interface.Commit();      
    }
    catch (DatabaseError_t pnide)
    {
	cout<<"Unexpected database error: "<<pnide<<endl;
	exit(-1);
    }
//TODO: Catch all other exceptions?
    
    return 0;
}
