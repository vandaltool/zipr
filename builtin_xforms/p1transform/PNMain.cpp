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
// #include "MEDS_AnnotationParser.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <functional>

#include "globals.h"

using namespace std;
using namespace IRDB_SDK;
// using namespace MEDS_Annotation;

#define ALLOF(a) begin(a),end(a)

bool verbose_log = false;

PNOptions *pn_options;

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
	SHARED_OBJECT_PROTECTION_OPTION,
	MIN_STACK_PAD_OPTION,
	MAX_STACK_PAD_OPTION,
	RECURSIVE_MIN_STACK_PAD_OPTION,
	RECURSIVE_MAX_STACK_PAD_OPTION,
	SHOULD_DOUBLE_FRAME_SIZE_OPTION,
	DOUBLE_THRESHOLD_OPTION,
	SELECTIVE_CANARIES_OPTION,
	SET_RANDOM_SEED,
	SET_CANARY_VALUE,
	SET_FLOATING_CANARY_OPTION,
	SET_DETECTION_POLICY_OPTION,
	SET_DETECTION_EXIT_CODE_OPTION
};



static struct option const long_options[] = 
{
	{"variant_id",required_argument, nullptr, VARIANT_ID_OPTION},
	{"bed_script",required_argument, nullptr, BED_SCRIPT_OPTION},
	{"blacklist",required_argument, nullptr, BLACKLIST_OPTION},
	{"coverage_file",required_argument, nullptr, COVERAGE_FILE_OPTION},
	{"pn_threshold",required_argument, nullptr, PN_THRESHOLD_OPTION},
	{"canaries", required_argument, nullptr, CANARIES_OPTION},
	{"only_validate",required_argument, nullptr, ONLY_VALIDATE_OPTION},
	{"no_p1_validate",no_argument,nullptr,NO_P1_VALIDATE_OPTION},
	{"apriori_layout_file",required_argument, nullptr, APRIORI_OPTION},
	{"align_stack",no_argument,nullptr,ALIGN_STACK_OPTION},
	{"ground_truth",no_argument,nullptr,GROUND_TRUTH_OPTION},
	{"shared_object_protection",no_argument,nullptr,SHARED_OBJECT_PROTECTION_OPTION},
	{"min_stack_padding",required_argument, nullptr, MIN_STACK_PAD_OPTION},
	{"max_stack_padding",required_argument, nullptr, MAX_STACK_PAD_OPTION},
	{"recursive_min_stack_padding",required_argument, nullptr, RECURSIVE_MIN_STACK_PAD_OPTION},
	{"recursive_max_stack_padding",required_argument, nullptr, RECURSIVE_MAX_STACK_PAD_OPTION},
	{"should_double_frame_size",required_argument, nullptr, SHOULD_DOUBLE_FRAME_SIZE_OPTION},
	{"double_threshold_size",required_argument, nullptr, DOUBLE_THRESHOLD_OPTION,},
	{"selective_canaries",required_argument, nullptr, SELECTIVE_CANARIES_OPTION},
	{"random_seed",required_argument, nullptr, SET_RANDOM_SEED},
	{"canary_value",required_argument, nullptr, SET_CANARY_VALUE},
	{"floating_canary",no_argument, nullptr, SET_FLOATING_CANARY_OPTION},
	{"detection_policy",required_argument, nullptr, SET_DETECTION_POLICY_OPTION},
	{"detection_exit_code",required_argument, nullptr, SET_DETECTION_EXIT_CODE_OPTION},
	{nullptr, 0, NULL, 0}
};


//TODO: PN will now p1 if no coverage is available,
//this is not desired for black box testing. 
//Find a solution. 


set<string> getFunctionList(const char * const p_filename)
{
	set<string> functionList;

	if(p_filename == nullptr)
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
map<string, map<string,double> > getCoverageMap(const char * const filename,double cov_threshold)
{
	map<string, map<string,double> > coverage_map;
	
	int acceptable_cov = 0;
	int total_funcs=0;

	if(filename == nullptr)
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

			double coverage = strtod(scoverage.c_str(),nullptr);

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
	printf("Usage TBD, exiting transform...\n");
	return;
}

class P1Transform_t : public TransformStep_t
{

	DatabaseID_t progid=BaseObj_t::NOT_IN_DATABASE;
	string BED_script="";
	string blacklist_file="";
	string coverage_file="";
	string only_validate="";
	bool validate_p1=false;
	bool align_stack=true;
	bool floating_canary=false;
	bool shared_object_protection=true;
	double p1threshold=0.75;
	bool do_ground_truth=false;
	VariantID_t *pidp=nullptr;
public:

P1Transform_t()
{
	auto env=getenv("PEASOUP_HOME");
	if(env==nullptr)
	{
		cerr<<"Must set $PEASOUP_HOME"<<endl;
		assert(0);
		exit(-1);
	}
	BED_script=string()+env+"/tools/bed.sh";
	blacklist_file=string()+env+"/tools/libc_functions.txt";
}

std::string getStepName(void) const override
{
        return "p1transform";
}


int parseArgs(const vector<string> step_args)
{
	auto argv = vector<char*>();
	transform(ALLOF(step_args), back_inserter(argv), [](const string &s) -> char* { return const_cast<char*>(s.c_str()); } );
	const auto argc=step_args.size();
	

	//Set the verbose flag
	char *verbose = getenv("VERBOSE");
	if(verbose == nullptr)
		verbose = getenv("PN_VERBOSE");

	verbose_log = (verbose != nullptr);

	progid = atoi(argv[0]);
	char buf[]="libp1transform.so";
	argv[0]=buf;
  
	int c=0;

	// global class to store options to Pn
	pn_options = new PNOptions();
	
	while((c = getopt_long(argc, &argv[0], "", long_options, nullptr)) != -1)
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
			p1threshold = strtod(optarg,nullptr);
			// valid values are -1, and 0-1, inclusive.
			// -1 means disabled.
			if(p1threshold != -1 && (p1threshold <0 || p1threshold >1))
			{
				//TODO: print a message call usage
				usage();
				return 1;
			}
			break;
		}
		case CANARIES_OPTION:
		{
			if(strcasecmp("on",optarg)==0)
			{
				pn_options->setDoCanaries(true);
			}
			else if(strcasecmp("off",optarg)==0)
			{
				pn_options->setDoCanaries(false);
			}
			else
			{
				//TODO: print error message and usage
				usage();
				return 1;
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
		case MIN_STACK_PAD_OPTION:
		{
			int min_stack_padding = atoi(optarg);
			if (min_stack_padding >= 0)
				pn_options->setMinStackPadding(min_stack_padding);
			break;
		}
		case MAX_STACK_PAD_OPTION:
		{
			int max_stack_padding = atoi(optarg);
			if (max_stack_padding >= 0)
				pn_options->setMaxStackPadding(max_stack_padding);
			break;
		}
		case RECURSIVE_MIN_STACK_PAD_OPTION:
		{
			int recursive_min_stack_padding = atoi(optarg);
			if (recursive_min_stack_padding >= 0)
				pn_options->setRecursiveMinStackPadding(recursive_min_stack_padding);
			break;
		}
		case RECURSIVE_MAX_STACK_PAD_OPTION:
		{
			int recursive_max_stack_padding = atoi(optarg);
			if (recursive_max_stack_padding >= 0)
				pn_options->setRecursiveMaxStackPadding(recursive_max_stack_padding);
			break;
		}
		case DOUBLE_THRESHOLD_OPTION:
		{
			const auto double_threshold = atoi(optarg);
			pn_options->setDoubleThreshold(double_threshold);
			break;	
		}
	
		case SHOULD_DOUBLE_FRAME_SIZE_OPTION:
		{
			if(strcasecmp("true",optarg)==0)
				pn_options->setShouldDoubleFrameSize(true);
			else if(strcasecmp("false",optarg)==0)
				pn_options->setShouldDoubleFrameSize(false);
			else
			{
				cout<<"Error:  should_double_frame_size option needs to be 'true' or 'false':  found "<<optarg<<endl;
				usage();
				return 1;
			}
			break;	
		}
		case SELECTIVE_CANARIES_OPTION:
		{
			string file=optarg;
  			ifstream in(file.c_str());
			string word;

			if(!in) 
			{
				cout << "Cannot open input file: "<<file<<endl;;
				usage();
				return 1;
			}

			while(in>>word)
				pn_options->addSelectiveCanaryFunction(word);

			break;
		}
		case SET_RANDOM_SEED:
		{
			int the_seed=atoi(optarg);
			cout<<"Setting random seed to: "<<dec<<the_seed<<endl;
			pn_options->setRandomSeed(the_seed);
			break;
		}
		case SET_CANARY_VALUE:
		{
			int the_val=strtoul(optarg, nullptr, 0);
			cout<<"Setting canary value to: 0x"<<hex<<the_val<<endl;
			pn_options->setCanaryValue(the_val);
			break;
		}
		case SET_FLOATING_CANARY_OPTION:
		{
			floating_canary = true;
			break;
		}
		case SET_DETECTION_POLICY_OPTION:
		{
			if(strcasecmp("exit",optarg)==0)
				pn_options->setDetectionPolicy(P_CONTROLLED_EXIT);
			else if(strcasecmp("halt",optarg)==0)
				pn_options->setDetectionPolicy(P_HARD_EXIT);
			else
				pn_options->setDetectionPolicy(P_CONTROLLED_EXIT);
			break;
		}
		case SET_DETECTION_EXIT_CODE_OPTION:
		{
			auto exit_code=(unsigned)atoi(optarg);
			assert(exit_code >= 0 && exit_code <= 255);
			pn_options->setDetectionExitCode(exit_code);
			break;
		}

		case '?':
		{
			//error message already printed by getopt_long
			//TODO: exit?
			usage();
			return 1;
		}
		default:
		{
			//TODO: invalid argument, and print usage
			usage();
			return 1;
		}
		}
	}

	

	// sanity check padding
	assert(pn_options->getMaxStackPadding() >= pn_options->getMinStackPadding());
	assert(pn_options->getRecursiveMaxStackPadding() >= pn_options->getRecursiveMinStackPadding());

	cout << "min_stack_padding: " << pn_options->getMinStackPadding() << endl;
	cout << "max_stack_padding: " << pn_options->getMaxStackPadding() << endl;
	cout << "recursive_min_stack_padding: " << pn_options->getRecursiveMinStackPadding() << endl;
	cout << "recursive_max_stack_padding: " << pn_options->getRecursiveMaxStackPadding() << endl;
	cout << "canaries: " << pn_options->getDoCanaries() << endl;

	return 0;
}


int executeStep(IRDBObjects_t *const irdb_objects)
{
	//setup the interface to the sql server 

	const auto pqxx_interface=irdb_objects->getDBInterface();
	BaseObj_t::setInterface(pqxx_interface);


	try
	{
		// read the variant ID using variant id number = atoi(argv[1])
                pidp = irdb_objects->addVariant(progid);

	  
		// verify that we read it correctly.
		assert(pidp->isRegistered()==true);
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);	
	}
	
	set<std::string> blackListOfFunctions;
	blackListOfFunctions = getFunctionList(blacklist_file.c_str());
	set<std::string> onlyValidateFunctions;
	onlyValidateFunctions = getFunctionList(only_validate.c_str());
	map<string, map<string,double> > coverage_map = getCoverageMap(coverage_file.c_str(),p1threshold);

	cout<<"P1threshold parsed = "<<p1threshold<<endl;

	try 
	{
		PNTransformDriver transform_driver(pidp,BED_script, pqxx_interface);

		cout << "   detection_policy: " << pn_options->getDetectionPolicy() << endl;
		cout << "detection_exit_code: " << pn_options->getDetectionExitCode() << " only active if controlled exit specified" << endl;

		transform_driver.SetMitigationPolicy(pn_options->getDetectionPolicy());
		transform_driver.SetDetectionExitCode(pn_options->getDetectionExitCode());

		OffsetInference *offset_inference = new OffsetInference();

		//TODO: hard coding the file in for now. 
		ifstream annotationFile("a.ncexe.infoannot", ifstream::in);
		assert(annotationFile.is_open());

// 		AnnotationBoundaryGenerator *abgen = new AnnotationBoundaryGenerator(new MEDS_AnnotationParser(annotationFile));

// 		PrecedenceBoundaryInference *aggressive_memset_inference = new PrecedenceBoundaryInference(offset_inference,abgen);

		DirectOffsetInference *direct_offset_inference = new DirectOffsetInference(offset_inference);
		ScaledOffsetInference *scaled_offset_inference = new ScaledOffsetInference(offset_inference);
		P1Inference *p1 = new P1Inference(offset_inference);
		// PrecedenceBoundaryInference *conservative_memset_inference = new PrecedenceBoundaryInference(p1, abgen);

		//Add new boundary inferences here

		//TODO: in addition to a hierarchy there should be equivalence classes, a failure in one member, is a failure for all. 

// 		transform_driver.AddInference(aggressive_memset_inference);
		transform_driver.AddInference(offset_inference,1);
		transform_driver.AddInference(direct_offset_inference,1);
		transform_driver.AddInference(scaled_offset_inference,1);
//		transform_driver.AddInference(conservative_memset_inference,1);
		transform_driver.AddInference(p1,2);

		transform_driver.AddBlacklist(blackListOfFunctions);
		transform_driver.AddOnlyValidateList(onlyValidateFunctions);
		transform_driver.SetDoCanaries(pn_options->getDoCanaries());
		transform_driver.SetDoFloatingCanary(floating_canary);
		transform_driver.SetDoAlignStack(align_stack);
		transform_driver.SetCoverageMap(coverage_map);
		transform_driver.SetCoverageThreshold(p1threshold);
		transform_driver.SetProtectSharedObjects(shared_object_protection);
		transform_driver.SetWriteStackIrToDb(do_ground_truth);

		//The passed in level must match a level that exists
		if(! validate_p1)
			transform_driver.SetNoValidationLevel(2);

		//Produce SLX transformation
		transform_driver.GenerateTransforms(irdb_objects); 

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		return -1;
	}

//TODO: Catch all other exceptions?
	
	return 0;
}

};


static shared_ptr<TransformStep_t> curInvocation;

extern "C"
shared_ptr<TransformStep_t> getTransformStep(void)
{
        curInvocation.reset(new P1Transform_t());
        return curInvocation;
}


