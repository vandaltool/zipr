#include "PNTransformDriver.hpp"
#include <cassert>
#include <algorithm>
#include <unistd.h>
#include <fstream>
#include "beaengine/BeaEngine.h"
#include <cmath>
#include <map>

using namespace std;
using namespace libIRDB;

bool PNTransformDriver::timeExpired = false;

void sigusr1Handler(int signum);

//TODO: use of pointers?
//TODO: time expired is handled in a hackish way

//TODO: Use CFG class for all instruction looping
//TODO: if stack access instruction are encountered before stack allocation, ignore them, after using CFG

//Used for sorting layouts by number of memory objects in descending order
//TODO: change name to reflect descending order
static bool CompareBoundaryNumbers(PNStackLayout *a, PNStackLayout *b)
{
    return (a->GetNumberOfMemoryObjects() > b->GetNumberOfMemoryObjects());
}

//TODO: redesign the way inferences are added
void PNTransformDriver::AddInference(PNStackLayoutInference *inference, int level)
{
//TODO: throw exceptions
    if(level < 1)
	assert(false);
    if(inference == NULL)
	assert(false);

    //if the level does not already exist, add empty vectors in transform_hierarchy
    //until it does
    while((int)transform_hierarchy.size() < level)
    {
	vector<PNStackLayoutInference*> tmp;
	transform_hierarchy.push_back(tmp);
    }

    transform_hierarchy[level-1].push_back(inference);
}

void PNTransformDriver::AddBlacklist(set<string> &blacklist)
{
    set<string>::iterator it;
    for(it = blacklist.begin();it != blacklist.end();it++)
    {
	this->blacklist.insert(*it);
    }
}

void PNTransformDriver::AddBlacklistFunction(string &func_name)
{
    blacklist.insert(func_name);
}

/*
void PNTransformDriver::GenerateTransforms2(VariantIR_t *virp,vector<Function_t*> funcs,string BED_script, int progid)
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


//TODO: break into smaller functions
void PNTransformDriver::GenerateTransforms(VariantIR_t *virp, string BED_script, int progid,map<string,double>coverage_map, double p1threshold)
{
    int total_funcs = 0;
    int blacklist_funcs = 0;
    vector<string> not_transformable;
    vector<PNStackLayout*> failed;

/*
    vector<Function_t*> funcs;
    for(
	set<Function_t*>::const_iterator it=virp->GetFunctions().begin();
	it!=virp->GetFunctions().end();
	++it
	)

    {
	Function_t *func = *it;
	if(blacklist.find(func->GetName()) != blacklist.end())
	{
	    cerr<<"PNTransformDriver: Blacklisted Function "<<func->GetName()<<endl;
	    continue;
	}
	funcs.push_back(func);
    }

      GenerateTransforms2(virp,funcs,BED_script,progid);


*/

    signal(SIGUSR1, sigusr1Handler);
    //For each function
    //Loop through each level, find boundaries for each, sort based on
    //the number of boundaries, attempt transform in order until successful
    //or until all inferences have been exhausted
    for(
	set<Function_t*>::const_iterator it=virp->GetFunctions().begin();
	it!=virp->GetFunctions().end();
	++it
	)

    {

	if(PNTransformDriver::timeExpired)
	{
	    cerr<<"PNTransformDriver: Time Expired --commit transforms" <<endl;
	    break;
	}

	Function_t *func = *it;
	bool success = false;

	//TODO: remove this at some point when I understand if this can happen or not
	assert(func != NULL);

	cerr<<"PNTransformDriver: Function: "<<func->GetName()<<endl;

	//Check if in black list
	if(blacklist.find(func->GetName()) != blacklist.end())
	{
	    cerr<<"PNTransformDriver: Blacklisted Function "<<func->GetName()<<endl;
	    blacklist_funcs++;
	    continue;
	}

    // @todo: specify regex patterns in black list file instead
	//        of special-casing here

	// filter out _L_lock_*
    // filter out _L_unlock_*
	if (func->GetName().find("_L_lock_") == 0 ||
	    func->GetName().find("_L_unlock_") == 0)
	{
	    cerr << "P1: filtering out: " << func->GetName() << endl;
	    blacklist_funcs++;
	    continue;
	}

	// filter out C++ stuff
	if (func->GetName().find("__gnu_")  != string::npos ||
            func->GetName().find("cxx_") != string::npos||
            func->GetName().find("_cxx")  != string::npos ||
            func->GetName().find("_GLOBAL_")  != string::npos ||
            func->GetName().find("_Unwind")  != string::npos ||
            func->GetName().find("__timepunct")  != string::npos ||
            func->GetName().find("__timepunct")  != string::npos ||
            func->GetName().find("__numpunct") != string::npos||
            func->GetName().find("__moneypunct")  != string::npos ||
            func->GetName().find("__PRETTY_FUNCTION__")  != string::npos ||
            func->GetName().find("__cxa")  != string::npos
        )
	{
	    cerr << "P1: filtering out: " << func->GetName() << endl;
	    blacklist_funcs++;
	    continue;
	}


	total_funcs++;

	//Get a layout inference for each level of the hierarchy. Sort these layouts based on
	//on the number of memory objects detected (in descending order). Then try each layout
	//as a basis for transformation until one succeeds or all layouts in each level of the
	//hierarchy have been exhausted. 

	//TODO: the code is horribly hacked for T and E, so this hiearchy is not even used
	//and it is assumed it is not used, but if you were to use it, this
	//would cause strange behavior given that I am checking a threshold for
	//p1 below. I assume that the one hierarchy level contains all transfroms
	//And the layout with the fewest memory objects is p1.
	for(unsigned int level=0;level<transform_hierarchy.size() && !success;level++)
	{

	    if(PNTransformDriver::timeExpired)
	    {
		cerr<<"PNTransformDriver: Time Expired --commit transforms" <<endl;
		break;
	    }

	    vector<PNStackLayout*> layouts = GenerateInferences(func, level);

	    //TODO: this is a quick hack and will fail if p1 doesn't have a transform
	    //but everything else does
	    if(layouts.size() == 0)
	    {
		if(transform_hierarchy.size()-1 == level)
		    not_transformable.push_back(func->GetName());

		continue;
	    }

	    sort(layouts.begin(),layouts.end(),CompareBoundaryNumbers);

	    //Check if function has coverage, if not, p1,
	    //check if function is above coverage threshold, if not, p1
	    //force p1 by creating a new layouts vector consisting only of p1

	    //See if the function is in the coverage map
	    if(coverage_map.find(func->GetName()) != coverage_map.end())
	    {
		//if coverage exists, if it is above the p1threshold,
		//do nothing, otherwise, resize layouts to have just
		//the p1 layout in it (the layout with the fewest objects)
		double func_coverage = coverage_map[func->GetName()];

		if(func_coverage <= p1threshold)
		{
		    //resize layouts, layouts is sorted by memory objects
		    //infered, take the last element of the vector
		    //(fewest number of memory objects), and make this
		    //the only layout
		    PNStackLayout* tmp = layouts[layouts.size()-1];
		    layouts.clear();
		    layouts.push_back(tmp);

		    cout<<"PNTransformDriver: Function "<<func->GetName()<< 
			"has insufficient coverage, using p1"<<endl;
		}
	    }
	    //If the function is not in the map, assume no coverage, and 
	    //resize layouts to have just
	    //the p1 layout in it (the layout with the fewest objects)
	    else
	    {
		//resize layouts, layouts is sorted by memory objects
		//infered, take the last element of the vector
		//(fewest number of memory objects), and make this
		//the only layout
		PNStackLayout* tmp = layouts[layouts.size()-1];
		layouts.clear();
		layouts.push_back(tmp);

		cout<<"PNTransformDriver: Function "<<func->GetName()<<
		    "does not have a coverage entry, using p1"<<endl;
	    }

	    for(unsigned int i=0;i<layouts.size();i++)
	    {
		if(layouts[i]->CanShuffle())
		{
		    //TODO: pass in the rep count
		    //TODO: make member variables?
		    if(!ShuffleValidation(1,layouts[i],virp,func,BED_script,progid))
		    {
			cerr<<"PNTransformDriver: Shuffle Validation Failure"<<endl;
			continue;
		    }
		    layouts[i]->Shuffle();//one final shuffle
		}
		layouts[i]->AddPadding();


		if(PNTransformDriver::timeExpired)
		{
		    cerr<<"PNTransformDriver: Time Expired --commit transforms" <<endl;
		    break;
		}
			
		if(!Rewrite((layouts[i]), func))
		{
		    undo(undo_list,func);
		    cerr<<"PNTransformDriver: Rewrite Failure: "<<layouts[i]->GetLayoutName()<<" Failed to Rewrite "<<func->GetName()<<endl;
		    continue;
		}
		else if(!Validate(virp,func,BED_script,progid))
		{
		    if(transform_hierarchy.size()-1 == level && i == layouts.size()-1)
			failed.push_back(layouts[i]);

		    undo(undo_list,func);
		    cerr<<"PNTransformDriver: Validation Failure: "<<layouts[i]->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
		}
		else
		{
		    cerr<<"PNTransformDriver: Final Transformation Success: "<<layouts[i]->ToString()<<endl;
		    //virp->WriteToDB();
		    transformed_history[layouts[i]->GetLayoutName()].push_back(layouts[i]);
		    success = true;
		    undo_list.clear();
		    break;
		}
		
	    }
	}
	
    }

    virp->WriteToDB();

	    /*
	    for(unsigned int i=0;i<layouts.size();i++)
	    {
		cerr<<"PNTransformDriver: Attempting Transform With Layout "<<layouts[i]->GetLayoutName()<<endl;

		(layouts[i])->Shuffle();

		//(layouts[i])->AddPadding();

		if(!Rewrite((layouts[i]), func))
		{
		    undo(undo_list,func);
		    cerr<<"PNTransformDriver: Rewrite Failure: "<<layouts[i]->GetLayoutName()<<" Failed to Rewrite "<<func->GetName()<<endl;
		    continue;
		}
		else if(!Validate(virp,func,BED_script,progid))
		{
		    undo(undo_list,func);
		    cerr<<"PNTransformDriver: Validation Failure: "<<layouts[i]->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
		}
		else
		{
		    cerr<<"PNTransformDriver: Transformation Success: "<<layouts[i]->ToString()<<endl;
		    virp->WriteToDB();
		    transformed_history[layouts[i]->GetLayoutName()].push_back(layouts[i]);
		    success = true;
		    undo_list.clear();
		    break;
		}

	    }
	    */

    cerr<<endl;
    cerr<<"############################SUMMARY############################"<<endl;

    cerr<<"PNTransformDriver: Functions Transformed"<<endl;

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
    cerr<<"PNTransformDriver: Non-Transformable Functions"<<endl;

    for(int i=0;i<not_transformable.size();++i)
    {
	cerr<<"\t"<<not_transformable[i]<<endl;
    }

    cerr<<"-----------------------------------------------"<<endl;
    cerr<<"PNTransformDriver: Functions Failing All Validation"<<endl;

    for(int i=0;i<failed.size();++i)
    {
	cerr<<"\t"<<failed[i]->GetFunctionName()<<"\t\tOut Args Size: "<<failed[i]->GetOutArgsSize()<<endl;
    }

    cerr<<"----------------------------------------------"<<endl;
    cerr<<"PNTransformDriver: Statistics by Transform"<<endl;

    for(int i=0;i<history_keys.size();++i)
    {
	cerr<<"\tLayout: "<<history_keys[i]<<endl;
	vector<PNStackLayout*> layouts = transformed_history[history_keys[i]];

	map<int,int> obj_histogram;

	cerr<<"\t\tTotal Transformed: "<<layouts.size()<<endl;

	int p1reductions = 0;
	double mem_obj_avg = 0.0;
	double mem_obj_dev = 0.0;
	for(int laynum=0;laynum<layouts.size();++laynum)
	{
	    if(layouts[laynum]->P1Reduction())
		p1reductions++;

	    unsigned int num_objects = layouts[laynum]->GetNumberOfMemoryObjects();

	    if(obj_histogram.find(num_objects) == obj_histogram.end())
		obj_histogram[num_objects] = 1;
	    else
		obj_histogram[num_objects] = obj_histogram[num_objects]+1;

	    mem_obj_avg += num_objects;
	}
	mem_obj_avg = mem_obj_avg/(double)layouts.size();

	for(int laynum=0;laynum<layouts.size();++laynum)
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
    cerr<<"PNTransformDriver: Non-Blacklisted Functions \t"<<total_funcs<<endl;
    cerr<<"PNTransformDriver: Blacklisted Functions \t"<<blacklist_funcs<<endl;
    cerr<<"PNTransformDriver: Transformable Functions \t"<<(total_funcs-not_transformable.size())<<endl;
    cerr<<"PNTransformDriver: Transformed \t\t\t"<<total_transformed<<endl;
}


//TODO: rewrite, this was a hackish solution to clean up the code for the hierarchy, which isn't even used for T&E
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

bool PNTransformDriver::ShuffleValidation(int reps, PNStackLayout *layout,VariantIR_t *virp,Function_t *func,string BED_script,int progid)
{
    cerr<<"PNTransformDriver: ShuffleValidation(): "<<layout->GetLayoutName()<<endl;

    for(int i=0;i<reps;i++)
    {
	if(PNTransformDriver::timeExpired)
	{
	    cerr<<"PNTransformDriver: Time Expired -- aborting shuffle validation" <<endl;
	    undo(undo_list,func);
	    return false;
	}
		
	cerr<<"PNTransformDriver: ShuffleValidation(): Shuffle attempt "<<i+1<<endl;

	layout->Shuffle();

	if(!Rewrite(layout, func))
	{
	    undo(undo_list,func);
	    cerr<<"PNTransformDriver: ShuffleValidation(): Rewrite Failure: attempt: "<<i+1<<" "<<
		layout->GetLayoutName()<<" Failed to Rewrite "<<func->GetName()<<endl;
	    return false;
	}
	else if(!Validate(virp,func,BED_script,progid))
	{
	    undo(undo_list,func);
	    cerr<<"PNTransformDriver: ShuffleValidation(): Validation Failure: attempt: "<<i+1<<" "<<
		layout->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
	    return false;
	}
	else
	{
	    undo(undo_list,func);
	}
    }

    return true;
}

bool PNTransformDriver::Validate(VariantIR_t *virp, Function_t *func, string BED_script, int progid)
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
	//TODO: throw exception
	//return false;
    }
    
    virp->GenerateSPRI(aspriFile); // p1.xform/<function_name>/a.irdb.aspri
    aspriFile.close();

    char new_instr[1024];
    //This script generates the aspri and bspri files; it also runs BED
//	  sprintf(new_instr, "$PEASOUP_HOME/tools/p1xform_v2.sh %d %s", progid, func->GetName().c_str());
    
    sprintf(new_instr, "%s %d %s %s", BED_script.c_str(), progid, aspri_filename.c_str(), bspri_filename.c_str());
    
    //If OK=BED(func), then commit 
    
    int rt=system(new_instr);
    int actual_exit = -1, actual_signal = -1;
    if (WIFEXITED(rt)) actual_exit = WEXITSTATUS(rt);
    else actual_signal = WTERMSIG(rt);
    int retval = actual_exit;
    
    return (retval == 0);
}

//TODO: this is a naive rewrite, more analysis is needed
//TODO: check if pmatch actually has a match
bool PNTransformDriver::Rewrite(PNStackLayout *layout, Function_t *func)
{
    //TODO: handle this better
    assert(layout != NULL);
    assert(func != NULL);

    cerr<<"PNTransformDriver: Rewriting Function = "<<func->GetName()<<endl;

    int max = PNRegularExpressions::MAX_MATCHES;
    regmatch_t pmatch[max];
    memset(pmatch, 0,sizeof(regmatch_t) * max);

    //TODO: if no stack allocation is seen before a stack access, ignore, don't abort?

    //rewrite instructions
    bool stack_alloc = false;
    for(
	set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
	it!=func->GetInstructions().end();
	++it
	)
    {
	Instruction_t* instr=*it;
	string matched ="";
	string disasm_str = "";
	DISASM disasm;

	instr->Disassemble(disasm);
	disasm_str = disasm.CompleteInstr;

	cerr << "PNTransformDriver:  Looking at Instruction = " << disasm_str << endl;
	
	//the disassmebly of lea has extra tokens not accepted by nasm, remove those tokens
	if(regexec(&(pn_regex.regex_lea_hack), disasm_str.c_str(), max, pmatch, 0)==0)
	{
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
	    cerr<<"PNTransformDriver: New LEA Instruction = "<<disasm_str<<endl;
	    matched = "";
	}
	    

	if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
	    cerr << "PNTransformDriver: Transforming Stack Alloc"<<endl;

	    //TODO: what should I do in this case?
	    if(stack_alloc)
	    {
		cerr <<"PNTransformDriver: Stack Alloc Previously Found, Ignoring Instruction"<<endl;
		continue;
	    }

	    stringstream ss;
	    ss << hex << layout->GetAlteredAllocSize();
	    
	    disasm_str = "sub esp, 0x"+ss.str();

	    cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;	    
	    undo_list[instr] = instr->GetDataBits();
	    if(!instr->Assemble(disasm_str))
		return false;

	    stack_alloc = true;
	} 
	else if(regexec(&(pn_regex.regex_esp_only), disasm_str.c_str(), max, pmatch, 0)==0) 
	{
	    cerr<<"PNTransformDriver: Transforming ESP Only Instruction ([esp])"<<endl;

/*
	    if(!stack_alloc)
	    {
		cerr<<"PNTransformDriver: No Stack Alloc, Aborting Transformation"<<endl;
		return false;
	    }
*/
	    PNRange *closest = layout->GetClosestRangeESP(0);

	    if(closest == NULL)
	    {
		/*
		cerr<<"PNTransformDriver: No Displacement Found"<<endl;
		return false;
		*/
		//There should always be a closet range to esp+0
		assert(false);
	    }

	    int new_offset = closest->GetDisplacement();

	    assert(new_offset >= 0);

	    if(new_offset == 0)
	    {
		cerr<<"PNTransformDriver: Displacement of [esp] is Zero, Ignoring Transformation"<<endl;
		continue;
	    }

	    stringstream ss;
	    ss<<hex<<new_offset;

	    matched = "esp+0x"+ss.str();
	    int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;	
	    disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
	    cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
	    undo_list[instr] = instr->GetDataBits();
	    if(!instr->Assemble(disasm_str.c_str()))
		return false;	    
	    
	}
//TODO: the regular expression order does matter, scaled must come first, change the regex so this doesn't matter  
	else if(regexec(&(pn_regex.regex_esp_scaled), disasm_str.c_str(), 5, pmatch, 0)==0 ||
		regexec(&(pn_regex.regex_esp_direct), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
	    cerr<<"PNTransformDriver: Transforming ESP Relative Instruction"<<endl;

/*
	    if(!stack_alloc)
	    {
		cerr<<"PNTransformDriver: No Stack Alloc, Aborting Transformation"<<endl;
		return false;
	    }
*/

	    int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
	    matched = disasm_str.substr(pmatch[1].rm_so,mlen);

	    // extract displacement 
	    int offset = strtol(matched.c_str(),NULL,0);

	    //TODO: I don't think this can happen but just in case
	    assert(offset >= 0);

	    //If the esp relative address goes outside of the frame, then
	    //we have a situation where esp is accessing incoming args
	    //(such as fomit-frame-pointer). In this case we simply add
	    //to the offset the size of the altered frame.
	    if(offset >= layout->GetOriginalAllocSize())
	    {
		//Get the number of bytes beyond the stack frame
		offset = offset-layout->GetOriginalAllocSize();
		//add those bytes to the altered stack size
		offset += layout->GetAlteredAllocSize();
		stringstream ss;
		ss<<hex<<offset;

		matched = "0x"+ss.str();
	    }
	    else
	    {
		PNRange *closest = layout->GetClosestRangeESP(offset);

		if(closest == NULL)
		{
		    cerr<<"PNTransformDriver: No Displacement Found"<<endl;
		    return false;
		}

		//TODO: put all functionality for getting new offset in PNStackLayout?
		int new_offset = closest->GetDisplacement() + offset;
		stringstream ss;
		ss<<hex<<new_offset;

		matched = "0x"+ss.str();
	    }
		
	    disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
	    cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
	    undo_list[instr] = instr->GetDataBits();
	    if(!instr->Assemble(disasm_str.c_str()))
		return false;
	}
	//TODO: the regular expression order does matter, scaled must come first, change the regex so this doesn't matter
	else if(regexec(&(pn_regex.regex_ebp_scaled), disasm_str.c_str(), 5, pmatch, 0)==0 ||
		regexec(&(pn_regex.regex_ebp_direct), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
	    cerr<<"PNTransformDriver: Transforming EBP Relative Instruction"<<endl;

/*
	    if(!stack_alloc)
	    {
		cerr<<"PNTransformDriver: No Stack Alloc, Aborting Transformation"<<endl;
		return false;
	    }
*/

	    int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
	    matched = disasm_str.substr(pmatch[1].rm_so,mlen);

	    // extract displacement 
	    int offset = strtol(matched.c_str(),NULL,0);

	    cerr<<"PNTransformDriver: Offset = "<<offset<<endl;

/*
	    //TODO: I think get closest ebp should throw and exception in this situation but for now
	    //I want to see if this happens at all.
	    if(((int)layout->GetOriginalAllocSize()+(int)layout->GetSavedRegsSize())-offset < 0)
		assert(false);
*/

	    PNRange *closest = layout->GetClosestRangeEBP(offset);

/*
	    if(((int)layout->GetOriginalAllocSize())-offset < 0 && closest == NULL)
	    {
		cerr<<"PNTransformDiver: Detected a Negative ESP Relative Offset and No Corresponding Range, Ignoring Instruction"<<endl;
		//	return false;	
		
		//In this situation the inference must not handle negative offsets or didn't detect it, we will
		//not transform it, and continue transforming the rest of the function.
		continue;
	    }
*/

	    if(closest == NULL)
	    {
		cerr<<"PNTransformDriver: No Displacement Found"<<endl;
		//Continue to transform for now.
		continue;
	    }

	    //TODO: put all functionality for getting new offset in PNStackLayout?
	    cerr<<"PNTransformDriver: closest displacement = "<<closest->GetDisplacement()<<endl;

	    int new_offset = ((int)layout->GetOriginalAllocSize() + (int)layout->GetSavedRegsSize() - offset);
	    new_offset += closest->GetDisplacement();
	    new_offset = ((int)layout->GetAlteredAllocSize() + (int)layout->GetSavedRegsSize()) - new_offset;

	    assert(new_offset >= 0);

	    stringstream ss;
	    ss<<hex<<new_offset;

	    matched = "0x"+ss.str();
		
	    disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
	    cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
	    undo_list[instr] = instr->GetDataBits();
	    if(!instr->Assemble(disasm_str.c_str()))
		return false;
		
	}

	else if(regexec(&(pn_regex.regex_stack_dealloc), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
	    cerr<<"PNTransformDriver: Transforming Stack Dealloc Instruction"<<endl;
/*
	    if(!stack_alloc)
	    {
		cerr<<"PNTransformDriver: No Stack Alloc, Aborting Transformation"<<endl;
		return false;
	    }
*/

	    //Check if the dealloc amount is 0. In unoptimized code, sometimes the
	    //compiler will reset esp, and then add 0 to esp
	    //In this case, do not deallocate the stack

	    int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
	    matched = disasm_str.substr(pmatch[1].rm_so,mlen);

	    // extract displacement 
	    int offset = strtol(matched.c_str(),NULL,0);

	    cerr<<"PNTransformDriver: Dealloc Amount = "<<offset<<endl;

	    if(offset == 0)
	    {
		cerr<<"PNTransformDriver: Dealloc of 0 detected, ignoring instruction"<<endl;
		continue;
	    }

	    stringstream ss;
	    ss << hex <<layout->GetAlteredAllocSize();

	    disasm_str = "add esp, 0x"+ss.str();
		
	    undo_list[instr] = instr->GetDataBits();
	    cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
	    if (!instr->Assemble(disasm_str)) 
		return false;
	}
	else
	    cerr<<"PNTransformDriver: No Pattern Match"<<endl;
    }
 
    //If you get here assume the transform was successfully made
    return true;
}

void PNTransformDriver::undo(map<Instruction_t*, string> undo_list, Function_t *func)
{
    //rollback any changes
    cerr<<"PNTransformDriver: Undo Transform: "<<undo_list.size()<<" instructions to rollback for function "<<func->GetName()<<endl;
    for(
	map<Instruction_t*, std::string>::const_iterator mit=undo_list.begin();
	mit != undo_list.end();
	++mit)
    {
	Instruction_t* insn = mit->first;
	std::string dataBits = mit->second;
  
	DISASM disasm;
	insn->Disassemble(disasm);
	insn->SetDataBits(dataBits);
    }

    undo_list.clear();
}

void sigusr1Handler(int signum)
{
    PNTransformDriver::timeExpired = true;
}
