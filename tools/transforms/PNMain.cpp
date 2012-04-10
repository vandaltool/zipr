
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

//TODO: PN will now p1 if no coverage is available,
//this is not desired for black box testing. 
//Find a solution. 


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

map<string,double> getCoverageMap(char *filename)
{
    map<string,double> coverage_map;

    ifstream coverage_file;
    coverage_file.open(filename);
    
    if(coverage_file.is_open())
    {
	while(!coverage_file.eof())
	{
	    //TODO: there is no sanity checking of this file

	    string line;
	    getline(coverage_file, line);
	    stringstream ss;
	    ss.str(line);
	    string func_name,tmp;
	    double coverage;

	    ss >>func_name;

	    if(func_name.compare("") == 0)
		continue;

	    ss >>tmp;

	    coverage = strtod(tmp.c_str(),NULL);

	    cout<<"func: "<<func_name<<" coverage: "<<coverage<<endl;

	    coverage_map[func_name] = coverage;
	}
	
	coverage_file.close();
    }    
    return coverage_map;
}

int main(int argc, char **argv)
{
    if(argc!=6)
    {
	cerr<<"Usage: p1transform.exe <variantid> <bed_script> <file containing name of blacklisted functions> <coverage file> <p1 threshold>"<<endl;
	exit(-1);
    }
    else
    {
	cout << "bed_script: " << argv[2] << " blacklist: " << argv[3] << 
	    " coverage: "<<argv[4]<<" p1 threshold: "<<argv[5]<<endl;
    }

    VariantID_t *pidp=NULL;
  
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
    }
    catch (DatabaseError_t pnide)
    {
	cout<<"Unexpected database error: "<<pnide<<endl;
	exit(-1);
    }
    
    set<std::string> blackListOfFunctions;
    blackListOfFunctions = getFunctionList(argv[3]);
    map<string,double> coverage_map = getCoverageMap(argv[4]);
    double p1threshold = strtod(argv[5],NULL);

   if(p1threshold > 1 || p1threshold <0)
    {
	cerr<<"usage: p1 threshold must be a value greater than or equal to 0 or less than or equal to 1"<<endl;
	exit(-1);
    }

   cout<<"P1threshold parsed = "<<p1threshold<<endl;

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
	PNTransformDriver transform_driver(pidp,BED_script);

	transform_driver.AddBlacklist(blackListOfFunctions);
	OffsetInference *offset_inference = new OffsetInference();
	DirectOffsetInference *direct_offset_inference = new DirectOffsetInference(offset_inference);
	ScaledOffsetInference *scaled_offset_inference = new ScaledOffsetInference(offset_inference);
	P1Inference *p1 = new P1Inference(offset_inference);

	//Add new boundary inferences here

	transform_driver.AddInference(offset_inference);
	transform_driver.AddInference(direct_offset_inference);
	transform_driver.AddInference(scaled_offset_inference);
	transform_driver.AddInference(p1,1);

	transform_driver.GenerateTransforms(coverage_map,p1threshold,1);
	//transform_driver.GenerateTransforms();

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
