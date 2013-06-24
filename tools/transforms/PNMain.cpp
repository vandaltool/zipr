

#include <iostream>
#include <limits.h>
//#include <unistd.h>
#include <getopt.h>
#include "PNStackLayoutInference.hpp"
#include "P1Inference.hpp"
#include "OffsetInference.hpp"
#include "ScaledOffsetInference.hpp"
#include "DirectOffsetInference.hpp"
#include "PNTransformDriver.hpp"

#include "PrecedenceBoundaryInference.hpp"
#include "AnnotationBoundaryGenerator.hpp"
#include "MEDS_AnnotationParser.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <cstdlib>
#include <sstream>

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;

bool verbose_log = false;

enum
{
	VARIANT_ID_OPTION = CHAR_MAX+1,
	BED_SCRIPT_OPTION,
	BLACKLIST_OPTION,
	COVERAGE_FILE_OPTION,
	PN_THRESHOLD_OPTION,
	CANARIES_OPTION,
	ONLY_VALIDATE_OPTION,
	NO_P1_VALIDATE_OPTION,
	ALIGN_STACK_OPTION,
	APRIORI_OPTION,
	GROUND_TRUTH_OPTION,
	SHARED_OBJECT_PROTECTION_OPTION
};



static struct option const long_options[] = 
{
	{"variant_id",required_argument, NULL, VARIANT_ID_OPTION},
	{"bed_script",required_argument, NULL, BED_SCRIPT_OPTION},
	{"blacklist",required_argument, NULL, BLACKLIST_OPTION},
	{"coverage_file",required_argument, NULL, COVERAGE_FILE_OPTION},
	{"pn_threshold",required_argument, NULL, PN_THRESHOLD_OPTION},
	{"canaries", required_argument, NULL, CANARIES_OPTION},
	{"only_validate",required_argument, NULL, ONLY_VALIDATE_OPTION},
	{"no_p1_validate",no_argument,NULL,NO_P1_VALIDATE_OPTION},
	{"apriori_layout_file",required_argument, NULL, APRIORI_OPTION},
	{"align_stack",no_argument,NULL,ALIGN_STACK_OPTION},
	{"ground_truth",no_argument,NULL,GROUND_TRUTH_OPTION},
	{"shared_object_protection",no_argument,NULL,SHARED_OBJECT_PROTECTION_OPTION},
	{NULL, 0, NULL, 0}
};


//TODO: PN will now p1 if no coverage is available,
//this is not desired for black box testing. 
//Find a solution. 


set<string> getFunctionList(char *p_filename)
{
	set<string> functionList;

	if(p_filename == NULL)
		return functionList;

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


//TODO: the coverage map should not use the function name since
//it is possible this will repeat when analyzing shared objects. 
map<string, map<string,double> > getCoverageMap(char *filename,double cov_threshold)
{
	map<string, map<string,double> > coverage_map;
	
	int acceptable_cov = 0;
	int total_funcs=0;

	if(filename == NULL)
		return coverage_map;

	ifstream coverage_file;
	coverage_file.open(filename);
	
	if(coverage_file.is_open())
	{
		while(!coverage_file.eof())
		{
			//TODO: there is no sanity checking of this file

			string line;
			getline(coverage_file, line);

			stringstream ss_line;
			ss_line.str(line);

			string func_id,file,func_name;
			ss_line>>func_id;
			istringstream iss_fid(func_id);
			getline(iss_fid,file,'+');
			getline(iss_fid,func_name,'+');

			string scoverage;
			ss_line>>scoverage;

			double coverage = strtod(scoverage.c_str(),NULL);

			if(func_name.length() > 0 && func_name[0] != '.')
			{
				if(coverage > cov_threshold)
				{
					if(func_name.length() > 0 && func_name[0] != '.')
						acceptable_cov++;
				}
				total_funcs++;
			}
			coverage_map[file][func_name]=coverage;

			cout<<"file: "<<file<<" func: "<<func_name<<" coverage: "<<coverage<<endl;
		}
		cout<<"Summary:"<<endl;
		cout<<"\tTotal non-plt functions = "<<total_funcs<<endl;
		cout<<"\tTotal non-plt functions exceeding "<<cov_threshold<<" threshold = "<<acceptable_cov<<" ("<<(double)acceptable_cov/total_funcs<<")"<<endl;
		coverage_file.close();
	}	 
	return coverage_map;
}

void usage()
{
	printf("Usage TBD, exiting\n");
	exit(-1);
}

int main(int argc, char **argv)
{

	//Set the verbose flag
	char *verbose = getenv("VERBOSE");
	if(verbose == NULL)
		verbose = getenv("PN_VERBOSE");

	verbose_log = (verbose != NULL);

	VariantID_t *pidp=NULL;
  
	int c;
	int progid=0;
	char *BED_script=NULL;
	char *blacklist_file=NULL;
	char *coverage_file=NULL;
	char *only_validate=NULL;
	bool validate_p1=true;
	bool do_canaries=true;
	bool align_stack=false;
	bool shared_object_protection=false;
	double p1threshold=0.0;
	bool do_ground_truth=false;

	while((c = getopt_long(argc, argv, "", long_options, NULL)) != -1)
	{
		switch(c)
		{
		case VARIANT_ID_OPTION:
		{
			progid = atoi(optarg);
			break;
		}
		case BED_SCRIPT_OPTION:
		{
			BED_script = optarg;
			break;
		}
		case BLACKLIST_OPTION:
		{
			blacklist_file = optarg;
			break;
		}
		case COVERAGE_FILE_OPTION:
		{
			coverage_file = optarg;
			break;
		}
		case PN_THRESHOLD_OPTION:
		{
			p1threshold = strtod(optarg,NULL);
			if(p1threshold <0 || p1threshold >1)
			{
				//TODO: print a message call usage
				usage();
			}
			break;
		}
		case CANARIES_OPTION:
		{
			if(strcasecmp("on",optarg)==0)
			{
				do_canaries=true;
			}
			else if(strcasecmp("off",optarg)==0)
			{
				do_canaries=false;
			}
			else
			{
				//TODO: print error message and usage
				usage();
			}
			break;
		}
		case ONLY_VALIDATE_OPTION:
		{
			only_validate=optarg;
			break;
		}
		case NO_P1_VALIDATE_OPTION:
		{
			validate_p1 = false;
			break;
		} 
		case ALIGN_STACK_OPTION:
		{
			align_stack = true;
			break;
		}
		case SHARED_OBJECT_PROTECTION_OPTION:
		{
			shared_object_protection=true;
			break;
		}
		case GROUND_TRUTH_OPTION:
		{
			do_ground_truth=true;
			break;
		}
		case '?':
		{
			//error message already printed by getopt_long
			//TODO: exit?
			usage();
			break;
		}
		default:
		{
			//TODO: invalid argument, and print usage
			usage();
		}
		}
	}

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
	blackListOfFunctions = getFunctionList(blacklist_file);
	set<std::string> onlyValidateFunctions;
	onlyValidateFunctions = getFunctionList(only_validate);
	map<string, map<string,double> > coverage_map = getCoverageMap(coverage_file,p1threshold);

	cout<<"P1threshold parsed = "<<p1threshold<<endl;

	try 
	{
		PNTransformDriver transform_driver(pidp,BED_script);

		OffsetInference *offset_inference = new OffsetInference();

		//TODO: hard coding the file in for now. 
		ifstream annotationFile("a.ncexe.infoannot", ifstream::in);
		assert(annotationFile.is_open());

		AnnotationBoundaryGenerator *abgen = new AnnotationBoundaryGenerator(new MEDS_AnnotationParser(annotationFile));

		PrecedenceBoundaryInference *aggressive_memset_inference = new PrecedenceBoundaryInference(offset_inference,abgen);

		DirectOffsetInference *direct_offset_inference = new DirectOffsetInference(offset_inference);
		ScaledOffsetInference *scaled_offset_inference = new ScaledOffsetInference(offset_inference);
		P1Inference *p1 = new P1Inference(offset_inference);
		PrecedenceBoundaryInference *conservative_memset_inference = new PrecedenceBoundaryInference(p1, abgen);

		//Add new boundary inferences here

		//TODO: in addition to a hierarchy there should be equivalence classes, a failure in one member, is a failure for all. 

		transform_driver.AddInference(aggressive_memset_inference);
		transform_driver.AddInference(offset_inference,1);
		transform_driver.AddInference(direct_offset_inference,1);
		transform_driver.AddInference(scaled_offset_inference,1);
		transform_driver.AddInference(conservative_memset_inference,1);
		transform_driver.AddInference(p1,2);

		transform_driver.AddBlacklist(blackListOfFunctions);
		transform_driver.AddOnlyValidateList(onlyValidateFunctions);
		transform_driver.SetDoCanaries(do_canaries);
		transform_driver.SetDoAlignStack(align_stack);
		transform_driver.SetCoverageMap(coverage_map);
		transform_driver.SetCoverageThreshold(p1threshold);
		transform_driver.SetProtectSharedObjects(shared_object_protection);
		transform_driver.SetWriteStackIrToDb(do_ground_truth);

		//The passed in level must match a level that exists
		if(! validate_p1)
			transform_driver.SetNoValidationLevel(2);

		//Produce SLX transformation
		transform_driver.GenerateTransforms(); 

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
