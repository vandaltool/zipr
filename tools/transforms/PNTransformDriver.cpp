#include "PNTransformDriver.hpp"
#include <cassert>
#include <algorithm>
#include <unistd.h>
#include <fstream>
#include "beaengine/BeaEngine.h"
#include "General_Utility.hpp"
#include <cmath>
#include "globals.h"

using namespace std;
using namespace libIRDB;

//TODO: this var is a hack for TNE
extern map<string, set<Instruction_t*> > inserted_instr;
extern map<string, set<AddressID_t*> > inserted_addr;

void sigusr1Handler(int signum);
bool PNTransformDriver::timeExpired = false;

//TODO: Error message functions?

//TODO: use of pointers?

//TODO: Use CFG class for all instruction looping
//TODO: if stack access instruction are encountered before stack allocation, ignore them, after using CFG

//Used for sorting layotus by number of memory objects in descending order
//TODO: change name to reflect descending order
static bool CompareBoundaryNumbers(PNStackLayout *a, PNStackLayout *b)
{
	return (a->GetNumberOfMemoryObjects() > b->GetNumberOfMemoryObjects());
}

PNTransformDriver::PNTransformDriver(VariantID_t *pidp,string BED_script)
{
	//TODO: throw exception?
	assert(pidp != NULL);
	srand(time(NULL));

	//TODO: throw exception?
	this->pidp = pidp;
	orig_progid = pidp->GetOriginalVariantID();
	orig_virp = new FileIR_t(*pidp);
	this->BED_script = BED_script;
	do_canaries = true;
	do_align = false;
}

PNTransformDriver::~PNTransformDriver()
{
	delete orig_virp;
}

//TODO: redesign the way inferences are added
void PNTransformDriver::AddInference(PNStackLayoutInference *inference, int level)
{
//TODO: throw exceptions
	if(level < 0)
		assert(false);
	if(inference == NULL)
		assert(false);

	//if the level does not already exist, add empty vectors in transform_hierarchy
	//until it does
	while((int)transform_hierarchy.size()-1 < level)
	{
		vector<PNStackLayoutInference*> tmp;
		transform_hierarchy.push_back(tmp);
	}

	transform_hierarchy[level].push_back(inference);
}

void PNTransformDriver::SetDoCanaries(bool do_canaries)
{
	this->do_canaries = do_canaries;
}

void PNTransformDriver::SetDoAlignStack(bool align_stack)
{
	this->do_align = align_stack;
}

void PNTransformDriver::AddBlacklist(set<string> &blacklist)
{
	set<string>::iterator it;
	for(it = blacklist.begin();it != blacklist.end();it++)
	{
		this->blacklist.insert(*it);
	}
}

void PNTransformDriver::AddBlacklistFunction(string func_name)
{
	blacklist.insert(func_name);
}

void PNTransformDriver::AddOnlyValidateList(std::set<std::string> &only_validate_list)
{
	set<string>::iterator it;
	for(it = only_validate_list.begin();it != only_validate_list.end();it++)
	{
		this->only_validate_list.insert(*it);
	}
}


/*
  void PNTransformDriver::GenerateTransforms2(FileIR_t *virp,vector<Function_t*> funcs,string BED_script, int progid)
  {
  for(int i=0;i<funcs.size();++i)
  {
  //transform all with
	
  vector<PNStackLayout*> layouts = GenerateInferences(funcs[i], 0);
	
  if(layouts.size() == 0)
  continue;
	
  sort(layouts.begin(),layouts.end(),CompareBoundaryNumbers);
	
  if(layouts[0]->CanShuffle())
  {
  layouts[0]->Shuffle();
  }
	
  //just for now padd too
  layouts[0]->AddPadding();
	
  if(!Rewrite(layouts[0],funcs[i]))
  {
  //I need to undo instead but right now undo will undo everything
  assert(false);
  }  
  }
	
  //TODO: right now I am passing the first func just to test this out, change validate to accept a string
  if(!Validate(virp,funcs[0],BED_script,progid))
  {
  undo(undo_list,funcs[0]);
	
  if(funcs.size()>1)
  {
  vector<Function_t*> left;
  vector<Function_t*> right;
		
  left.insert(left.begin(),funcs.begin(),funcs.begin()+(funcs.size()/2));
  right.insert(right.begin(),funcs.begin()+(funcs.size()/2),funcs.end());
		
  GenerateTransforms2(virp,left,BED_script,progid);
  GenerateTransforms2(virp,right,BED_script,progid);
  }
  else
  {
  vector<PNStackLayout*> layouts = GenerateInferences(funcs[0],0);
		
  if(layouts.size() == 0)
  return;
		
  sort(layouts.begin(),layouts.end(),CompareBoundaryNumbers);
		
  //TODO: I should not have to use the first inference at this point
  for(int i=0;i<layouts.size();++i)
  {
  if(layouts[i]->CanShuffle())
  {
  layouts[i]->Shuffle();
  }
		
  //just for now padd too
  layouts[i]->AddPadding();
		
  if(!Rewrite(layouts[i],funcs[0]))
  {
  //I need to undo instead but right now undo will undo everything
  assert(false);
  }  
  else if(!Validate(virp,funcs[0],BED_script,progid))
  {
  undo(undo_list,funcs[0]);
  continue;
  }
  }
  }
  }
  else
  {
  cerr<<"Validated "<<funcs.size()<<" functions"<<endl;
  virp->WriteToDB();
  undo_list.clear();
  }

  //Call a recursive function that takes in the number of total funcs, and attempts a 
  //transform and validation
  //Divide into N regions, initial N is total_funcs
  //for each division, transform all functions, then validate.
  //if validation succeeds, commit
  //if validation fails, divide N by 2 (if N is odd add one first)
  //Attemp transform for those divisions, when completley validated, continue on

  //recursive_validate(virp,total_funcs,BED_script, progid)
  //The problem is undoing only part of the undo list
  //when all validate, attempt transform again, only shuffling
  //when that validates add padding, 
  //You must remember the most aggressive transform that did not fail in some previous
  //run for each function, never use a more aggressive transform.

  //use a map to hold a pnstack layout for every function
  //after validation, loop through all functions again, and use these pnstack layouts
  //if the layout fails, how do I know which transform to pick next?
  //Perhaps another map listing the perferences of transforms?
  }
*/

//Assumed no coverage is used, and the entire hierarchy of transforms should be attempted on all functions
void PNTransformDriver::GenerateTransforms()
{
	map<string,double> empty_map;
	GenerateTransforms(empty_map,0,0,-1);
}

void PNTransformDriver::GenerateTransformsInit()
{
	//TODO: have a thread safe timeExpired
	timeExpired = false;
	signal(SIGUSR1, sigusr1Handler);
	total_funcs = 0;
	blacklist_funcs = 0;
	not_transformable.clear();
	failed.clear();	   
}

bool PNTransformDriver::CanaryTransformHandler(PNStackLayout *layout, Function_t *func, bool validate)
{
	//TODO: hack for TNE: if not doing canaries, use padding transform handler instead
/*
  if(!do_canaries)
  {
  cerr<<"PNTransformDriver: canary transformations turned off, attempting padding transformation."<<endl;
  return PaddingTransformHandler(layout, func, validate);	
  }
*/

	bool success = false;

	if(!validate) 
		cerr<<"PNTransformDriver: Function "<<func->GetName()<<" is flagged to be transformed without validation"<<endl;

	cerr<<"PNTransformDriver: Function "<<func->GetName()<<" is canary safe, attempting canary rewrite"<<endl;

	layout->Shuffle();
	layout->AddRandomPadding(do_align);

	if(!Canary_Rewrite(layout,func))
	{
		//Experimental code
		undo(func);

		//TODO: error message
		cerr<<"PNTransformDriver: canary_rewrite failure"<<endl;
	}
	else
	{
		//if(!Validate(new_virp,targ_func))
		if(validate && !Validate(orig_virp,func))
		{
			//Experimental code
			undo(func);

			//TODO: error message
			cerr<<"PNTransformDriver: canary validation failure, rolling back"<<endl;
		}
		else
		{
			cerr<<"PNTransformDriver: Final Transformation Success: "<<layout->ToString()<<endl;
			cerr<<"PNTransformDriver: Canary rewrite and validation successful."<<endl;

			//TODO: I would like to set something in the data structures to indicate
			//the canary is possible, but turned off. 
			if(!do_canaries)
			{
				cerr<<"PNTransformDriver: canary transformations turned off, removing canary from transformation."<<endl;
				undo(func);
				Sans_Canary_Rewrite(layout,func);
			}

			transformed_history[layout->GetLayoutName()].push_back(layout);
			success = true;
			//TODO: message

		}
	}

	//reset_undo(func->GetName());

	return success;
}

bool PNTransformDriver::PaddingTransformHandler(PNStackLayout *layout, Function_t *func, bool validate)
{
	bool success = false;

	if(!validate)
		cerr<<"PNTransformDriver: Function "<<func->GetName()<<" is flagged to be transformed without validation"<<endl;

	cerr<<"PNTransformDriver: Function "<<func->GetName()<<" is not canary safe, attempting shuffle validation"<<endl;

	if(validate && !ShuffleValidation(2,layout,func))
	{
		cerr<<"PNTransformDriver: Shuffle Validation Failure"<<endl;
		return success;
	}

	layout->Shuffle();//one final shuffle
	layout->AddRandomPadding(do_align);
			
	if(!Sans_Canary_Rewrite(layout,func))
	{
		undo(func);
		cerr<<"PNTransformDriver: Rewrite Failure: "<<layout->GetLayoutName()<<" Failed to Rewrite "<<func->GetName()<<endl;
	}
	else if(validate && !Validate(orig_virp, func))
	{
		undo(func);
		cerr<<"PNTransformDriver: Validation Failure: "<<layout->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
	}
	else
	{
		cerr<<"PNTransformDriver: Final Transformation Success: "<<layout->ToString()<<endl;
		transformed_history[layout->GetLayoutName()].push_back(layout);
		success = true;
		//undo_list.clear();
		//reset_undo(func->GetName());
	}

	//orig_virp->WriteToDB();

	return success;
}

bool PNTransformDriver::LayoutRandTransformHandler(PNStackLayout *layout, Function_t *func,bool validate)
{
	if(!validate)
		cerr<<"PNTransformDriver: Function "<<func->GetName()<<" is flagged to be transformed without validation"<<endl;

	bool success = false;
	cerr<<"PNTransformDriver: Function "<<func->GetName()<<" is not padding safe, attempting layout randomization only"<<endl;

	if(validate && !ShuffleValidation(2,layout,func))
	{
		cerr<<"PNTransformDriver: Validation Failure: "<<layout->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
	}
	else
	{
		layout->Shuffle();
		//TODO: do I need to check for success at this point?
		Sans_Canary_Rewrite(layout,func);
		cerr<<"PNTransformDriver: Final Transformation Success: "<<layout->ToString()<<endl;
		transformed_history[layout->GetLayoutName()].push_back(layout);
		success = true;
		//undo_list.clear();
		//reset_undo(func->GetName());
	}

	//orig_virp->WriteToDB();
	return success;
}

bool PNTransformDriver::IsBlacklisted(Function_t *func)
{
	// @todo: specify regex patterns in black list file instead
	//		  of special-casing here

	// filter out _L_lock_*
	// filter out _L_unlock_*

  if (func->GetName().find("_L_lock_") == 0 ||
  func->GetName().find("_L_unlock_") == 0 ||
  func->GetName().find("__gnu_")	!= string::npos ||
  func->GetName().find("cxx_") != string::npos||
  func->GetName().find("_cxx")  != string::npos ||
  func->GetName().find("_GLOBAL_")  != string::npos ||
  func->GetName().find("_Unwind")	 != string::npos ||
  func->GetName().find("__timepunct")	 != string::npos ||
  func->GetName().find("__timepunct")	 != string::npos ||
  func->GetName().find("__numpunct") != string::npos||
  func->GetName().find("__moneypunct")  != string::npos ||
  func->GetName().find("__PRETTY_FUNCTION__")	 != string::npos ||
  func->GetName().find("__cxa")  != string::npos ||
  blacklist.find(func->GetName()) != blacklist.end())
//	if(blacklist.find(func->GetName()) != blacklist.end())
	{
		cerr<<"PNTransformDriver: Blacklisted Function "<<func->GetName()<<endl;
		blacklist_funcs++;
		return true;
	}
	return false;
}


//TODO: break into smaller functions
//threshold_level is the hierarchy level to start at if the coverage for a function is less than the threshold.
//Set this value to a negative integer to abort transformation of the function if the threshold is not met. 
void PNTransformDriver::GenerateTransforms(map<string,double> coverage_map, double threshold, int threshold_level, int never_validate_level)
{
	if(transform_hierarchy.size() == 0)
	{
		cerr<<"PNTransformDriver: No Transforms have been registered, aborting GenerateTransforms"<<endl;
		return;
	}

	this->never_validate_level = never_validate_level;

	GenerateTransformsInit();

	if(threshold < 0)
		threshold = 0;
	else if(threshold > 1)
		threshold = 1;

	//TODO: threshold_level should be unsigned but I am not sure
	//if I allow the threshold to be negative. For now
	//covert the tramsform_hierarchy size to an int. 
	//Check for max int boundary condition in the future. 
	if(threshold_level >= (int)transform_hierarchy.size())
		threshold_level = transform_hierarchy.size()-1;

	//For each function
	//Loop through each level, find boundaries for each, sort based on
	//the number of boundaries, attempt transform in order until successful
	//or until all inferences have been exhausted

	unsigned int report_count = 0;
	for(
		set<Function_t*>::const_iterator it=orig_virp->GetFunctions().begin();
		it!=orig_virp->GetFunctions().end()&&!timeExpired;
		++it
		)

	{
		report_count++;

		Function_t *func = *it;
		bool success = false;

		//TODO: remove this at some point when I understand if this can happen or not
		assert(func != NULL);

		cerr<<"PNTransformDriver: Function: "<<func->GetName()<<endl;

		//Check if in blacklist
		if(IsBlacklisted(func))
			continue;

		if(report_count %50 == 0)
		{
			cerr<<"#########################Intermediate Report#########################"<<endl;
			Print_Report();
		}

		total_funcs++;

		unsigned int level=0;
		double func_coverage = 0;

		//TODO: check a priori map, if in the map, ignore coverage

		//see if the function is in the coverage map
		if(coverage_map.find(func->GetName()) != coverage_map.end())
		{
			//if coverage exists, if it is above or equal to the threshold
			//do nothing, otherwise set hierachy to start at the level
			//passed. 
			func_coverage = coverage_map[func->GetName()];
		}

		if(func_coverage < threshold)
		{
			level = threshold_level;

			if(threshold_level < 0)
			{
				cout<<"PNTransformDriver: Function "<<func->GetName()<<
					" has insufficient coverage, aborting transformation"<<endl;
				continue;
			}
			else
			{
				cout<<"PNTransformDriver: Function "<<func->GetName()<<
					" has insufficient coverage, starting at hierarchy at "
					" level "<<threshold_level<<endl;
			}

		}

		//Get a layout inference for each level of the hierarchy. Sort these layouts based on
		//on the number of memory objects detected (in descending order). Then try each layout
		//as a basis for transformation until one succeeds or all layouts in each level of the
		//hierarchy have been exhausted. 

		//TODO: need to properly handle not_transformable and functions failing all transforms. 
		unsigned int null_inf_count = 0;
		unsigned int starting_level = level;
		for(;level<transform_hierarchy.size()&&!success&&!timeExpired;level++)
		{
			vector<PNStackLayout*> layouts = GenerateInferences(func, level);
 
			if(layouts.size() == 0)
			{
				null_inf_count++;

				//If the number of null inferences encountered equals the number of inferences
				//that are possible (based on the starting level which may not be 0), then
				//the function is not transformable. 
				if(transform_hierarchy.size()-starting_level == null_inf_count)
					not_transformable.push_back(func->GetName());
				//TODO: is there a better way of checking this?
				else if(transform_hierarchy.size()-1 == level)
					failed.push_back(func);

				continue;
			}

			sort(layouts.begin(),layouts.end(),CompareBoundaryNumbers);

			//TODO: for now, only validate if the only_validate_list doesn't have any contents and
			//the level does not equal the never_validate level. I may want to allow the only validate
			//list to be empty, in which case a pointer is better, to check for NULL.
			bool do_validate = true;
			if(only_validate_list.size() != 0)
			{
				if(only_validate_list.find(func->GetName()) == only_validate_list.end())
				{
					do_validate = false;
				}
		
			}

			//TODO: type casting never_validate_level to unsigned.
			//I need to make uses of unsigned and signed consistent.
			//The rule of thumb should be if the value is absolutely
			//not allowed to go 0, then it is unsigned. For now I am
			//type casting to remove the warning. 
			do_validate = do_validate && (level != (unsigned int)never_validate_level);
		

			//Go through each layout in the level of the hierarchy in order. 
			for(unsigned int i=0;i<layouts.size()&&!timeExpired;i++)
			{

				//TODO: I need to have the ability to make transformations optional
				//the approach taken now is a last minute hack for TNE

				if(layouts[i]->IsCanarySafe())
				{
	
					success = CanaryTransformHandler(layouts[i],func,do_validate);
					if(!success && transform_hierarchy.size()-1 == level && i == layouts.size()-1)
						failed.push_back(func);
					else if(success)
						break;

				}

				else if(layouts[i]->IsPaddingSafe())
				{
					success = PaddingTransformHandler(layouts[i],func,do_validate);
					if(!success && transform_hierarchy.size()-1 == level && i == layouts.size()-1)
						failed.push_back(func);
					else if(success)
						break;

				}
				//if not canary or padding safe, the layout can only be randomized
				else
				{
					success = LayoutRandTransformHandler(layouts[i],func,do_validate);
					if(!success && transform_hierarchy.size()-1 == level && i == layouts.size()-1)
						failed.push_back(func);
					else if(success)
						break;
				}
			}
		}	
	}

	if(timeExpired)
		cerr<<"Time Expired: Commit Changes"<<endl;

	//finalize transformation, commit to database
	orig_virp->WriteToDB();

	cerr<<"############################Final Report############################"<<endl;
	Print_Report();
}

void PNTransformDriver::Print_Report()
{
	cerr<<endl;
	cerr<<"############################SUMMARY############################"<<endl;

	cerr<<"Functions Transformed"<<endl;

	map<string,vector<PNStackLayout*> >::const_iterator it;
	int total_transformed = 0;

	vector<string> history_keys;
	for(it = transformed_history.begin(); it != transformed_history.end(); it++)
	{
		total_transformed += it->second.size();
	
		//TODO: sort by layout type then print
		for(unsigned int i=0;i<it->second.size();i++)
		{
			cerr<<"\t"<<it->second[i]->ToString()<<endl;
		}

		history_keys.push_back(it->first);
	}

	cerr<<"-----------------------------------------------"<<endl;
	cerr<<"Non-Transformable Functions"<<endl;

	for(unsigned int i=0;i<not_transformable.size();++i)
	{
		cerr<<"\t"<<not_transformable[i]<<endl;
	}

	cerr<<"-----------------------------------------------"<<endl;
	cerr<<"Functions Failing All Validation"<<endl;

	for(unsigned int i=0;i<failed.size();++i)
	{
		cerr<<"\t"<<failed[i]->GetName()<<endl;
	}

	cerr<<"----------------------------------------------"<<endl;
	cerr<<"Statistics by Transform"<<endl;

	for(unsigned int i=0;i<history_keys.size();++i)
	{
		cerr<<"\tLayout: "<<history_keys[i]<<endl;
		vector<PNStackLayout*> layouts = transformed_history[history_keys[i]];

		map<int,int> obj_histogram;

		cerr<<"\t\tTotal Transformed: "<<layouts.size()<<endl;

		int p1reductions = 0;
		double mem_obj_avg = 0.0;
		double mem_obj_dev = 0.0;
		for(unsigned int laynum=0;laynum<layouts.size();++laynum)
		{
			if(layouts[laynum]->GetNumberOfMemoryObjects()==1)
				p1reductions++;

			unsigned int num_objects = layouts[laynum]->GetNumberOfMemoryObjects();

			if(obj_histogram.find(num_objects) == obj_histogram.end())
				obj_histogram[num_objects] = 1;
			else
				obj_histogram[num_objects] = obj_histogram[num_objects]+1;

			mem_obj_avg += num_objects;
		}
		mem_obj_avg = mem_obj_avg/(double)layouts.size();

		for(unsigned int laynum=0;laynum<layouts.size();++laynum)
		{
			mem_obj_dev += pow((((double)layouts[laynum]->GetNumberOfMemoryObjects())-mem_obj_avg),2);
		}

		mem_obj_dev = sqrt(mem_obj_dev/(double)layouts.size());

		cerr<<"\t\tMemory Object Average: "<<mem_obj_avg<<endl;
		cerr<<"\t\tMemory Object Deviation: "<<mem_obj_dev<<endl;
		cerr<<"\t\tObject Histogram:"<<endl;

		map<int,int>::const_iterator hist_it;
		for(hist_it = obj_histogram.begin();hist_it != obj_histogram.end();hist_it++)
		{
			cerr<<"\t\t\tMemory Objects: "<<hist_it->first<<" Instances: "<<hist_it->second<<endl;
		}

		cerr<<"\t\tP1 Reductions: "<<p1reductions<<endl;
	}

	cerr<<"----------------------------------------------"<<endl;
	cerr<<"Non-Blacklisted Functions \t"<<total_funcs<<endl;
	cerr<<"Blacklisted Functions \t\t"<<blacklist_funcs<<endl;
	cerr<<"Transformable Functions \t"<<(total_funcs-not_transformable.size())<<endl;
	cerr<<"Transformed \t\t\t"<<total_transformed<<endl;
}


vector<PNStackLayout*> PNTransformDriver::GenerateInferences(Function_t *func,int level)
{
	vector<PNStackLayout*> layouts;

	for(unsigned int inf=0;inf<transform_hierarchy[level].size();inf++)
	{
		cerr<<"PNTransformDriver: Generating Layout Inference for "<<transform_hierarchy[level][inf]->GetInferenceName()<<endl;
		PNStackLayout *tmp = (transform_hierarchy[level][inf])->GetPNStackLayout(func);
		
		if(tmp == NULL)
		{  
			cerr<<"PNTransformDriver: NULL Inference Generated"<<endl;
		}
		else
		{
			cerr<<"PNTransformDriver: Inference Successfully Generated"<<endl;
			layouts.push_back(tmp);
		}
	}

	return layouts;
}

bool PNTransformDriver::ShuffleValidation(int reps, PNStackLayout *layout,Function_t *func)
{
	if(!layout->CanShuffle())
		return true;

	cerr<<"PNTransformDriver: ShuffleValidation(): "<<layout->GetLayoutName()<<endl;

	for(int i=0;i<reps;i++)
	{
		if(timeExpired)
			return false;

		cerr<<"PNTransformDriver: ShuffleValidation(): Shuffle attempt "<<i+1<<endl;

		layout->Shuffle();

		if(!Sans_Canary_Rewrite(layout, func))
		{
			undo(func);
			cerr<<"PNTransformDriver: ShuffleValidation(): Rewrite Failure: attempt: "<<i+1<<" "<<
				layout->GetLayoutName()<<" Failed to Rewrite "<<func->GetName()<<endl;
			return false;
		}
		else if(!Validate(orig_virp,func))
		{
			undo(func);
			cerr<<"PNTransformDriver: ShuffleValidation(): Validation Failure: attempt: "<<i+1<<" "<<
				layout->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
			return false;
		}
		else
		{
			undo(func);
		}
	}

	return true;
}

bool PNTransformDriver::Validate(FileIR_t *virp, Function_t *func)
{
	cerr<<"PNTransformDriver: Validate(): Validating function "<<func->GetName()<<endl;

	string dirname = "p1.xform/" + func->GetName();
	string cmd = "mkdir -p " + dirname;
	system(cmd.c_str());
	
	string aspri_filename = string(get_current_dir_name()) + "/" + dirname + "/a.irdb.aspri";
	string bspri_filename = string(get_current_dir_name()) + "/" + dirname + "/a.irdb.bspri";
	ofstream aspriFile;
	aspriFile.open(aspri_filename.c_str());
	
	if(!aspriFile.is_open())
	{
		assert(false);
	}
	
	cerr<<"Pre genreate SPRI"<<endl;
	virp->GenerateSPRI(aspriFile,false); // p1.xform/<function_name>/a.irdb.aspri
	cerr<<"Post genreate SPRI"<<endl;
	aspriFile.close();

	char new_instr[1024];
	//This script generates the aspri and bspri files; it also runs BED
	sprintf(new_instr, "%s %d %s %s", BED_script.c_str(), orig_progid, aspri_filename.c_str(), bspri_filename.c_str());
	
	//If OK=BED(func), then commit	
	int rt=system(new_instr);
	int actual_exit = -1, actual_signal = -1;
	if (WIFEXITED(rt)) actual_exit = WEXITSTATUS(rt);
	else actual_signal = WTERMSIG(rt);
	int retval = actual_exit;
	
	return (retval == 0);
}

unsigned int PNTransformDriver::GetRandomCanary()
{

	//TODO: check for bias.
	stringstream canary;
	canary.str("");
	for(int i=0;i<8;i++)
	{
		canary<<hex<<(rand()%16);
	}
	unsigned int ret_val;
	sscanf(canary.str().c_str(),"%x",&ret_val);

	return ret_val;
}

bool PNTransformDriver::Canary_Rewrite(PNStackLayout *orig_layout, Function_t *func)
{
	//TODO: hack for TNE, assuming all virp is orig_virp now. 
	FileIR_t *virp = orig_virp;

	if(!orig_layout->IsCanarySafe())
		return Sans_Canary_Rewrite(orig_layout,func);

	PNStackLayout tmp = orig_layout->GetCanaryLayout();
	PNStackLayout *layout = &tmp;
	vector<canary> canaries;

	vector<PNRange*> mem_objects = layout->GetRanges();

	for(unsigned int i=0;i<mem_objects.size();i++)
	{

		canary c;
		c.esp_offset = mem_objects[i]->GetOffset() + mem_objects[i]->GetDisplacement() + mem_objects[i]->GetSize();

		//TODO: make this random
		c.canary_val = GetRandomCanary();

		//bytes to frame pointer or return address (depending on if a frame
		//pointer is used) from the stack pointer
		c.ret_offset = (layout->GetAlteredAllocSize()+layout->GetSavedRegsSize());
		//if frame pointer is used, add 4 bytes to get to the return address
		if(layout->HasFramePointer())
			c.ret_offset += 4;

		//Now with the total size, subtract off the esp offset to the canary
		c.ret_offset = c.ret_offset - c.esp_offset;
		//The number should be positive, but we want negative so 
		//convert to negative
		c.ret_offset = c.ret_offset*-1;
	
		if(verbose_log)
			cerr << "c.canary_val = " << c.canary_val << "	c.ret_offset = " <<	 c.ret_offset << endl;

		canaries.push_back(c);
	}
	
	bool stack_alloc = false;
	int max = PNRegularExpressions::MAX_MATCHES;
	regmatch_t pmatch[max];
	memset(pmatch, 0,sizeof(regmatch_t) * max);

	for(
		set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
		)
	{
		Instruction_t* instr=*it;

		string matched="";
		string disasm_str = "";
		DISASM disasm;

		instr->Disassemble(disasm);
		disasm_str = disasm.CompleteInstr;

		if(verbose_log)
			cerr<<"PNTransformDriver: Canary_Rewrite: Looking at instruction "<<disasm_str<<endl;

		//TODO: is the stack_alloc flag necessary anymore? 
		//if(!stack_alloc && regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
		if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
		{
			if(verbose_log)
				cerr << "PNTransformDriver: Canary Rewrite: Transforming Stack Alloc"<<endl;

			//TODO: determine size of alloc, and check if consistent with alloc size?

			//extract K from: sub esp, K 
			if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
			{
				int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
				matched = disasm_str.substr(pmatch[1].rm_so,mlen);
				//extract K 
				unsigned int ssize;
				if(str2uint(ssize, matched.c_str()) != SUCCESS)
				{
					//If this occurs, then the found stack size is not a 
					//constant integer, so it must be a register. 

					//cerr<<"PNTransformDriver: Canary Rewrite: Stack alloc of non-integral type ("<<matched<<"), ignoring instruction "<<endl;

					//TODO: hack for TNE, assuming that isntruction_rewrite
					//will add padding to dynamic arrays. 
					Instruction_Rewrite(layout,instr);
					continue;			
				}
			}

			//TODO: I need a check to see if the previous amount is equal to
			//the expect stack frame, a check is done is the inference
			//generation now, so it should be okay without it,

			stringstream ss;
			ss << hex << layout->GetAlteredAllocSize();
		
			disasm_str = "sub esp, 0x"+ss.str();

			if(verbose_log)
				cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;		
			//undo_list[instr] = instr->GetDataBits();
			//undo_list[instr] = copyInstruction(instr);
			undo_list[func->GetName()][instr] = copyInstruction(instr);

			virp->RegisterAssembly(instr,disasm_str);
/*
  if(!instr->Assemble(disasm_str))
  return false;
*/
			stack_alloc = true;

			for(unsigned int i=0;i<canaries.size();i++)
			{
				ss.str("");
				ss<<"mov dword [esp+0x"<<hex<<canaries[i].esp_offset<<"], 0x"<<hex<<canaries[i].canary_val;
				instr = insertAssemblyAfter(virp,instr,ss.str());
				if(i==0)
					instr->SetComment("Canary Setup Entry: "+ss.str());
				else
					instr->SetComment("Canary Setup: "+ss.str());
			}
		}
		else if(regexec(&(pn_regex.regex_ret), disasm_str.c_str(),5,pmatch,0)==0)
		{
			if(verbose_log)
				cerr<<"PNTransformDriver: Canary Rewrite: inserting ret canary check"<<endl;

			//undo_list[instr] = instr->GetDataBits();
			//undo_list[instr] = copyInstruction(instr);
			undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);



			//This could probably be done once, but having the original instruction
			//allows me to produce messages that indicate more precisely where
			//the overflow occurred. 
			Instruction_t *handler_code = getHandlerCode(virp,instr,P_CONTROLLED_EXIT);

			//insert canary checks
			//
			//TODO: may need to save flags register

			for(unsigned int i=0;i<canaries.size();i++)
			{
				instr = insertCanaryCheckBefore(virp,instr,canaries[i].canary_val,canaries[i].ret_offset, handler_code);	
			}
		}
		//if the stack is not believed to be static, I can't check the canary using esp.
		//for now don't do a canary check on calls in these situations. 
		else if(layout->IsStaticStack() && regexec(&(pn_regex.regex_call), disasm_str.c_str(),5,pmatch,0)==0)
		{
		
			if(verbose_log)
				cerr<<"PNTransformDriver: Canary Rewrite: inserting call canary check"<<endl;
			//undo_list[instr] = copyInstruction(instr);
			undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

			//This could probably be done once, but having the original instruction
			//allows me to produce messages that indicate more precisely where
			//the overflow occurred. 
			Instruction_t *handler_code = getHandlerCode(virp,instr,P_CONTROLLED_EXIT);

			//insert canary checks
			//
			//TODO: may need to save flags register
			for(unsigned int i=0;i<canaries.size();i++)
			{
				instr = insertCanaryCheckBefore(virp,instr,canaries[i].canary_val,canaries[i].esp_offset, handler_code);	
			}
		}
		//TODO: message if not static stack?
		else
		{
			if(!Instruction_Rewrite(layout,instr))
				return false;
		}
	}

	return true;
}

bool PNTransformDriver::Sans_Canary_Rewrite(PNStackLayout *layout, Function_t *func)
{
	//TODO: add return value
	if(verbose_log)
		cerr<<"PNTransformDriver: Sans Canary Rewrite for Function = "<<func->GetName()<<endl;

	for(
		set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
		)
	{
		Instruction_t* instr=*it;
		string disasm_str = "";
		DISASM disasm;
		instr->Disassemble(disasm);
		disasm_str = disasm.CompleteInstr;

		if(verbose_log)
			cerr<<"PNTransformDriver: Sans_Canary_Rewrite: Looking at instruction "<<disasm_str<<endl;

		if(!Instruction_Rewrite(layout,instr))
			return false;
	}

	return true;
}

inline bool PNTransformDriver::Instruction_Rewrite(PNStackLayout *layout, Instruction_t *instr)
{
	FileIR_t* virp = orig_virp;

	int max = PNRegularExpressions::MAX_MATCHES;
	regmatch_t pmatch[max];
	memset(pmatch, 0,sizeof(regmatch_t) * max);

	string matched="";
	string disasm_str = "";
	DISASM disasm;

	instr->Disassemble(disasm);
	disasm_str = disasm.CompleteInstr;
	
	//the disassmebly of lea has extra tokens not accepted by nasm, remove those tokens
	if(regexec(&(pn_regex.regex_lea_hack), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming LEA Instruction"<<endl;

		matched = "";
		for (int k = 1; k < 5; ++k)
		{
			if (pmatch[k].rm_so >= 0 && pmatch[k].rm_eo >= 0) 
			{
				int mlen = pmatch[k].rm_eo - pmatch[k].rm_so;
				matched.append(disasm_str.substr(pmatch[k].rm_so,mlen));
			}
		}
		disasm_str = matched;
		if(verbose_log)
			cerr<<"PNTransformDriver: New LEA Instruction = "<<disasm_str<<endl;

		matched = "";
	}
		

	if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		cerr << "PNTransformDriver: Transforming Stack Alloc"<<endl;

		//TODO: determine size of alloc, and check if consistent with alloc size?


		//extract K from: sub esp, K 
		if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
		{
			int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
			matched = disasm_str.substr(pmatch[1].rm_so,mlen);
			//extract K 
			unsigned int ssize;
			if(str2uint(ssize, matched.c_str()) != SUCCESS)
			{
				//If this occurs, then the found stack size is not a 
				//constant integer, so it must be a register. 

				//cerr<<"PNTransformDriver: Stack alloc of non-integral type ("<<matched<<"), ignoring instruction"<<endl;

				undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);
				//TODO: hack for TNE, padd the allocation by adding a random
				//amount to the register used to subtract from esp. 

				stringstream ss;
				//TODO: I am uncertain how alignment will work in this situation
				//if the layout is aligned, this will return a padding amount
				//divisible by the alignment stride, however, without knowing
				//the size of the object, this may not ensure alignment, it is
				//up to the compiler to handle that else where. 
				ss<<"add "<<matched<<" , 0x"<<hex<<layout->GetRandomPadding();//"0x500";

				if(verbose_log)
				{
					cerr<<"PNTransformDriver: adding padding to dynamic stack allocation"<<endl;
					cerr<<"PNTransformDriver: inserted instruction = "<<ss.str()<<endl;
				}

				Instruction_t *new_instr = insertAssemblyBefore(virp,instr,ss.str(),NULL);
				new_instr->SetComment("Dynamic array padding:" +ss.str());
				return true;			
			}
		}

		stringstream ss;
		ss << hex << layout->GetAlteredAllocSize();
		
		disasm_str = "sub esp, 0x"+ss.str();

		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;		
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str))
  return false;
*/

		//stack_alloc = true;
	} 
	else if(regexec(&(pn_regex.regex_and_esp), disasm_str.c_str(), max, pmatch, 0)==0)
	{
/*
  cerr<<"PNTransformDriver: Transforming AND ESP instruction"<<endl;

  disasm_str = "nop";

  cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;

  undo_list[instr] = copyInstruction(instr);
  if(!instr->Assemble(disasm_str))
  return false;
*/
	}
	else if(regexec(&(pn_regex.regex_esp_only), disasm_str.c_str(), max, pmatch, 0)==0) 
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming ESP Only Instruction ([esp])"<<endl;

		PNRange *closest = layout->GetClosestRangeESP(0);

		if(closest == NULL)
		{
			//There should always be a closet range to esp+0
			assert(false);
		}

		int new_offset = closest->GetDisplacement();

		assert(new_offset >= 0);

		if(new_offset == 0)
		{
			if(verbose_log)
				cerr<<"PNTransformDriver: Displacement of [esp] is Zero, Ignoring Transformation"<<endl;
			
			return true;
		}

		stringstream ss;
		ss<<hex<<new_offset;

		matched = "esp+0x"+ss.str();
		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;	
		disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;		
*/
		
	}
//TODO: the regular expression order does matter, scaled must come first, change the regex so this doesn't matter  
	else if(regexec(&(pn_regex.regex_esp_scaled), disasm_str.c_str(), 5, pmatch, 0)==0 ||
			regexec(&(pn_regex.regex_esp_direct), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming ESP Relative Instruction"<<endl;

		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
		matched = disasm_str.substr(pmatch[1].rm_so,mlen);

		// extract displacement 
		int offset = strtol(matched.c_str(),NULL,0);

		//TODO: I don't think this can happen but just in case
		assert(offset >= 0);

		int new_offset = layout->GetNewOffsetESP(offset);
		
		stringstream ss;
		ss<<hex<<new_offset;

		matched = "0x"+ss.str();
		
		disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;
*/
	}
	//TODO: the regular expression order does matter, scaled must come first, change the regex so this doesn't matter
	//for lea esp, [ebp-<const>] it is assumed the <const> will not be in the stack frame, so it should be ignored.
	//this should be validated prior to rewrite (i.e., this is a TODO, it hasn't been done yet).
	else if(regexec(&(pn_regex.regex_ebp_scaled), disasm_str.c_str(), 5, pmatch, 0)==0 ||
			regexec(&(pn_regex.regex_ebp_direct), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming EBP Relative Instruction"<<endl;

		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
		matched = disasm_str.substr(pmatch[1].rm_so,mlen);

		// extract displacement 
		int offset = strtol(matched.c_str(),NULL,0);

		if(verbose_log)
			cerr<<"PNTransformDriver: Offset = "<<offset<<endl;

		int new_offset = layout->GetNewOffsetEBP(offset);

		if(new_offset == offset)
		{
			if(verbose_log)
				cerr<<"PNTransformDriver: No offset transformation necessary, skipping instruction"<<endl;

			return true;
		}

		stringstream ss;
		ss<<hex<<new_offset;

		matched = "0x"+ss.str();
		
		disasm_str.replace(pmatch[1].rm_so,mlen,matched);

		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;

		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;
*/
		
	}
	//if we get an instruction where ebp is the index, transform it using the
	//offset as the lookup (the second pattern matched), however, it is assumed
	//that only p1 is performed in these cases. 
	//TODO: if this is encountered, insert a switch statement to evaluate at runtime
	//which offset to use. e.g., [ecx+ebp*1-0x21], at run time evaluate ecx+0x21,
	//and determine which reange it falls into, and jump to an instruction which
	//uses the correct offset. 
	else if(regexec(&(pn_regex.regex_scaled_ebp_index), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming Scaled EBP Indexed Instruction"<<endl;

		int mlen = pmatch[2].rm_eo - pmatch[2].rm_so;
		matched = disasm_str.substr(pmatch[2].rm_so,mlen);

		// extract displacement 
		int offset = strtol(matched.c_str(),NULL,0);

		if(verbose_log)
			cerr<<"PNTransformDriver: Offset = "<<offset<<endl;

		int new_offset = layout->GetNewOffsetEBP(offset);

		if(new_offset == offset)
		{
			if(verbose_log)
				cerr<<"PNTransformDriver: No offset transformation necessary, skipping instruction"<<endl;

			return true;
		}

		stringstream ss;
		ss<<hex<<new_offset;

		matched = "0x"+ss.str();
		
		disasm_str.replace(pmatch[2].rm_so,mlen,matched);

		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;
*/
	}
	else if(regexec(&(pn_regex.regex_stack_dealloc), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming Stack Dealloc Instruction"<<endl;

		//Check if the dealloc amount is 0. In unoptimized code, sometimes the
		//compiler will reset esp, and then add 0 to esp
		//In this case, do not deallocate the stack

		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
		matched = disasm_str.substr(pmatch[1].rm_so,mlen);

		// extract displacement 
		int offset = strtol(matched.c_str(),NULL,0);

		if(verbose_log)
			cerr<<"PNTransformDriver: Dealloc Amount = "<<offset<<endl;

		if(offset == 0)
		{
			if(verbose_log)
				cerr<<"PNTransformDriver: Dealloc of 0 detected, ignoring instruction"<<endl;

			return true;
		}

		stringstream ss;
		ss << hex <<layout->GetAlteredAllocSize();

		disasm_str = "add esp, 0x"+ss.str();
		
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()->GetName()][instr] = copyInstruction(instr);

		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;

		virp->RegisterAssembly(instr,disasm_str);
/*
  if (!instr->Assemble(disasm_str)) 
  return false;
*/
	}
	else
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: No Pattern Match"<<endl;
	}
	return true;
}



//TODO: there is a memory leak, I need to write a undo_list clear to properly cleanup
//void PNTransformDriver::undo(map<Instruction_t*, Instruction_t*> undo_list, Function_t *func)
void PNTransformDriver::undo( Function_t *func)
{
	string func_name = func->GetName();

	//rollback any changes
	cerr<<"PNTransformDriver: Undo Transform: "<<undo_list[func_name].size()<<" instructions to rollback for function "<<func_name<<endl;
	for(
		map<Instruction_t*, Instruction_t*>::const_iterator mit=undo_list[func_name].begin();
		mit != undo_list[func_name].end();
		++mit)
	{
		Instruction_t* alt = mit->first;
		Instruction_t* orig = mit->second;
  
		copyInstruction(orig,alt);
		
		orig_virp->UnregisterAssembly(alt);
	
		//TODO: apparently there is a issue with this delete.
		//When using the padding/shuffle transformation PN terminates
		//for some reason with no segfault. Removing this delete
		//solves the issue. Using the canary transformation, I haven't
		//observed the same issue however there are fewer undos when
		//using the canary transform. 
//	delete orig;
	}

	for(set<Instruction_t*>::const_iterator it=inserted_instr[func_name].begin();
		it != inserted_instr[func_name].end();
		++it
		)
	{
		orig_virp->UnregisterAssembly(*it);
		orig_virp->GetInstructions().erase(*it);
		delete *it;
	}

	for(set<AddressID_t*>::const_iterator it=inserted_addr[func_name].begin();
		it != inserted_addr[func_name].end();
		++it
		)
	{
		orig_virp->GetAddresses().erase(*it);
		delete *it;
	}
	//reset_undo(func->GetName());

	undo_list.erase(func_name);
	inserted_instr.erase(func_name);
	inserted_addr.erase(func_name);
	//undo_list.clear();
}

/*
  void PNTransformDriver::reset_undo(string func)
  {
  undo_list.erase(func);
  inserted_instr.erase(func);
  inserted_addr.erase(func);
  }
*/



void sigusr1Handler(int signum)
{
	PNTransformDriver::timeExpired = true;
}
