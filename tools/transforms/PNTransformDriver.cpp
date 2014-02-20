#include "PNTransformDriver.hpp"
#include <cassert>
#include <algorithm>
#include <unistd.h>
#include <fstream>
#include "beaengine/BeaEngine.h"
#include "General_Utility.hpp"
#include "PNIrdbManager.hpp"
#include <cmath>
#include "globals.h"
#include <libIRDB-cfg.hpp>

using namespace std;
using namespace libIRDB;

//TODO: this var is a hack for TNE
extern map<Function_t*, set<Instruction_t*> > inserted_instr;
extern map<Function_t*, set<AddressID_t*> > inserted_addr;

void sigusr1Handler(int signum);
bool PNTransformDriver::timeExpired = false;

static Instruction_t* GetNextInstruction(Instruction_t *prev, Instruction_t* insn, Function_t* func);


int get_saved_reg_size()
{
	return FileIR_t::GetArchitectureBitWidth()/(sizeof(char)*8);
}

//TODO: Error message functions?

//TODO: use of pointers?

//TODO: Use CFG class for all instruction looping
//TODO: if stack access instruction are encountered before stack allocation, ignore them, after using CFG

//Used for sorting layotus by number of memory objects in descending order
//TODO: change name to reflect descending order
static bool CompareBoundaryNumbersDescending(PNStackLayout *a, PNStackLayout *b)
{
	return (a->GetNumberOfMemoryObjects() > b->GetNumberOfMemoryObjects());
}

static bool CompareValidationRecordAscending(validation_record a, validation_record b)
{
	return (a.layouts[a.layout_index]->GetNumberOfMemoryObjects() < b.layouts[b.layout_index]->GetNumberOfMemoryObjects());
}


PNTransformDriver::PNTransformDriver(VariantID_t *pidp,string BED_script, pqxxDB_t *pqxx_if) : 
	pn_regex(NULL), pqxx_interface(pqxx_if)
{
	//TODO: throw exception?
	assert(pidp != NULL);
	srand(time(NULL));

	//TODO: throw exception?
	this->pidp = pidp;
	orig_progid = pidp->GetOriginalVariantID();
	orig_virp = NULL;
	this->BED_script = BED_script;
	do_canaries = true;
	do_align = false;
	no_validation_level = -1;
	coverage_threshold = -1;
	do_shared_object_protection = false;

}

PNTransformDriver::~PNTransformDriver()
{
	//NOTE: because I now handle shared objects, I clean up orig_virp
	//as I create use it, deleting here caused errors, I had to 
	//ensure orig_virp is NULL, which I could do, but it is just as
	//easy to destroy it myself when I am finished with it. 
//	if(orig_virp != NULL)
//		delete orig_virp;
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

	//TODO: For the optimized TNE version I had to remove
	//optional canaries. This would've been done by
	//the finalize_transformation step, but for some reason
	//I can't get the registration functionality to work
	//assert false for now if the canaries are turned off
	//to remind me to fix this. 
	assert(do_canaries);
}

void PNTransformDriver::SetDoAlignStack(bool align_stack)
{
	this->do_align = align_stack;
}

void PNTransformDriver::AddBlacklist(set<string> &blacklist)
{
	this->blacklist.insert(blacklist.begin(),blacklist.end());

/*
	set<string>::iterator it;
	for(it = blacklist.begin();it != blacklist.end();it++)
	{
		this->blacklist.insert(*it);
	}
*/
}

void PNTransformDriver::AddBlacklistFunction(string func_name)
{
	blacklist.insert(func_name);
}


//This function was experimental, and I never did anything with it
void PNTransformDriver::AddOnlyValidateList(std::set<std::string> &only_validate_list)
{
	set<string>::iterator it;
	for(it = only_validate_list.begin();it != only_validate_list.end();it++)
	{
		this->only_validate_list.insert(*it);
	}
}

void PNTransformDriver::SetCoverageMap(std::map<std::string,map<string,double> > coverage_map)
{
	this->coverage_map = coverage_map;
}

void PNTransformDriver::SetNoValidationLevel(unsigned int no_validation_level)
{
	this->no_validation_level = no_validation_level;
}

void PNTransformDriver::SetCoverageThreshold(double threshold)
{
	coverage_threshold = threshold;
}

void PNTransformDriver::SetProtectSharedObjects(bool do_protection)
{
	do_shared_object_protection = do_protection;
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
		if(validate && !Validate(orig_virp,func->GetName()))
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

//			transformed_history[layout->GetLayoutName()].push_back(layout);

			// finalize_record fr;
			// fr.layout = layout;
			// fr.func = func;
			// fr.firp = orig_virp;
			// finalization_registry.push_back(fr);
			// undo(func);

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
	else if(validate && !Validate(orig_virp, func->GetName()))
	{
		undo(func);
		cerr<<"PNTransformDriver: Validation Failure: "<<layout->GetLayoutName()<<" Failed to Validate "<<func->GetName()<<endl;
	}
	else
	{
		cerr<<"PNTransformDriver: Final Transformation Success: "<<layout->ToString()<<endl;
//		transformed_history[layout->GetLayoutName()].push_back(layout);
		// finalize_record fr;
		// fr.layout = layout;
		// fr.func = func;
		// fr.firp = orig_virp;
		// finalization_registry.push_back(fr);
		// undo(func);
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
//		transformed_history[layout->GetLayoutName()].push_back(layout);
		// finalize_record fr;
		// fr.layout = layout;
		// fr.func = func;
		// fr.firp = orig_virp;
		// finalization_registry.push_back(fr);
		// undo(func);
		success = true;
		//undo_list.clear();
		//reset_undo(func->GetName());
	}

	//orig_virp->WriteToDB();
	return success;
}

bool PNTransformDriver::IsBlacklisted(Function_t *func)
{
  if(sanitized.find(func) != sanitized.end())
  {
	  cerr<<"PNTransformDriver:: Sanitized Function "<<func->GetName()<<endl;
	  sanitized_funcs++;
	  return true;
  }

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


void PNTransformDriver::GenerateTransformsInit()
{
	//TODO: have a thread safe timeExpired
	timeExpired = false;
	signal(SIGUSR1, sigusr1Handler);
	total_funcs = 0;
	blacklist_funcs = 0;
	sanitized_funcs = 0;
	push_pop_sanitized_funcs = 0;
	dynamic_frames = 0;
	high_coverage_count = low_coverage_count = no_coverage_count = validation_count = 0;
	not_transformable.clear();
	failed.clear();	   
}


void PNTransformDriver::InitNewFileIR(File_t* this_file)
{
	//Always modify a.ncexe first. This is assumed to be what the constructor of FileIR_t will return if
	//the variant ID is used alone as the parameter to the constructor. 
	orig_virp = new FileIR_t(*pidp, this_file);
	assert(orig_virp && pidp);

	int elfoid=orig_virp->GetFile()->GetELFOID();
	pqxx::largeobject lo(elfoid);
	lo.to_file(pqxx_interface->GetTransaction(),"readeh_tmp_file.exe");

	elfiop=new ELFIO::elfio;
	elfiop->load("readeh_tmp_file.exe");
	
	ELFIO::dump::header(cout,*elfiop);
	ELFIO::dump::section_headers(cout,*elfiop);
}



void PNTransformDriver::GenerateTransforms()
{
	if(transform_hierarchy.size() == 0)
	{
		cerr<<"PNTransformDriver: No Transforms have been registered, aborting GenerateTransforms"<<endl;
		return;
	}
	GenerateTransformsInit();

	//sanity check of no_validation_level
	if(no_validation_level >= (int)transform_hierarchy.size())
	{
		cerr<<"PNTransformDriver ERROR: no_validation_level greater than highest level in the hierarchy, exiting."<<endl;
		exit(1);
	}
	

	//TODO: I refactored PN, but to keep the refactoring simple, I did not change the use of the class field "orig_virp"
	//now that I am protecting shared objects, it might be better to pass this variable around, or at least change the
	//name, something to consider

	//--------------------a.ncexe protection-----------------------
	InitNewFileIR(NULL);


	// now that we've loaded the FileIR, we can init the reg expressions needed for this object.
	pn_regex=new PNRegularExpressions;


	//Sanity check: make sure that this file is actually a.ncexe, if not assert false for now
	string url = orig_virp->GetFile()->GetURL();
	//TODO: basename is only used as a hack
	//because of the way the url is stored in the db.
	//The url should be fixed to be the absolute path. 
	if(url.find("a.ncexe")==string::npos)
	{
		assert(false);
	}

	cout<<"PNTransformDriver: Protecting File: "<<url<<endl;
	GenerateTransformsHidden(coverage_map["a.ncexe"]);


	//-----------------------shared object protection----------------------------

	//Transform any shared objects if shared object protection is on
	//This will loop through all files stored in pidp and skip a.ncexe
	if(do_shared_object_protection)
	{
		cout<<"PNTransformDriver: Shared Object Protection ON"<<endl;
		for(set<File_t*>::iterator it=pidp->GetFiles().begin();
			it!=pidp->GetFiles().end()&&!timeExpired;
			++it
			)
		{
			File_t* this_file=*it;
			assert(this_file);

			string url = this_file->GetURL();
			//if the file is a.ncexe skip it (it has been transformed already)
			if(url.find("a.ncexe")!=string::npos)
				continue;

			// read the db  
			InitNewFileIR(this_file);
#if 0
orig_virp=new FileIR_t(*pidp,this_file);
assert(orig_virp && pidp);
int elfoid=firp->GetFile()->GetELFOID();
pqxx::largeobject lo(elfoid);
lo.to_file(pqxx_interface.GetTransaction(),"readeh_tmp_file.exe");

ELFIO::elfio*    elfiop=new ELFIO::elfio;
elfiop->load("readeh_tmp_file.exe");

ELFIO::dump::header(cout,*elfiop);
ELFIO::dump::section_headers(cout,*elfiop);
#endif


			map<string,double> file_coverage_map;

			string key;
			//find the appropriate coverage map for the given file
			//TODO: in theory this should be a simple map look up, but because
			//the complete path is not currently in the DB for a shared object
			//I have to loop through the map for now. 
			for(map<string, map<string, double> >::iterator it=coverage_map.begin();
				it!=coverage_map.end()&&!timeExpired; ++it)
			{
				key = it->first;

				//TODO: basename is only used as a hack
				//because of the way the url is stored in the db.
				//The url should be fixed to be the absolute path. 
				key=string(basename(key.c_str()));

				if(key.empty())
					continue;

				if(url.find(key)!=string::npos)
				{
					file_coverage_map=it->second;
					break;
				}
			}


			cerr<<"######PreFile Report: Accumulated results prior to processing file: "<<url<<"######"<<endl;
			Print_Report();

			cout<<"PNTransformDriver: Protecting File: "<<url<<endl;
			GenerateTransformsHidden(file_coverage_map);
		}
	}

	if (write_stack_ir_to_db)
	{
		WriteStackIRToDB();
	}

	if(timeExpired)
		cerr<<"Time Expired: Commit Changes So Far"<<endl;

	//finalize transformation, commit to database 
	Finalize_Transformation();

	//TODO: clean up malloced (new'ed) FileIR_t

	cerr<<"############################Final Report############################"<<endl;
	Print_Report();
}

// count_prologue_pushes -
// start at the entry point of a function, and count all push instructions
static int count_prologue_pushes(Function_t *func)
{
	int count=0;
	Instruction_t* insn=NULL, *prev=NULL;

	// note: GetNextInstruction will return NULL if it tries to leave the function
	// or encounters conditional control flow (it will follow past unconditional control flow
	// it also stops at indirect branches (which may leave the function, or may generating 
	// multiple successors)
	for(insn=func->GetEntryPoint(); insn!=NULL; insn=GetNextInstruction(prev,insn, func))
	{
		DISASM d;
		insn->Disassemble(d);
		if(strstr(d.CompleteInstr,"push"))
		{
			if( 	(d.Argument2.ArgType&CONSTANT_TYPE)==CONSTANT_TYPE   //&& 
//				insn->GetFallthrough()!=NULL &&
//				insn->GetFallthrough()->GetTarget()!=NULL && 
//				func->GetInstructions().find(insn->GetFallthrough()->GetTarget())!=func->GetInstructions().end()
			  )
			{
				// push imm followed by a jump outside the function. 
				// this is a call, don't count it as a push.
			}
			else
				count++;
		}

		prev=insn;
	}
	return count;
}
Instruction_t* find_exit_insn(Instruction_t *insn, Function_t *func)
{
	Instruction_t *prev=NULL;
	for(; insn!=NULL; insn=GetNextInstruction(prev,insn, func))
	{
		prev=insn;
	}

	if(prev->GetFallthrough()!=NULL && prev->GetTarget()!=NULL)
		return NULL; // cond branch 
	if(prev->GetFallthrough()==NULL && prev->GetTarget()==NULL)
	{
		DISASM d;
		prev->Disassemble(d);
		if(strstr(d.CompleteInstr,"ret")!=NULL) // return ret.
			return prev;
		return NULL; // indirect branch 
	}

	// it must have either a fallthrough or target, but not both.
	// so it must leave the function.

	return prev;

}

// check_for_push_pop_coherence -
// we are trying to check whether the function's prologue uses 
// the same number of push as each exit to the function does.
// odd things happen if we can't match pushes and pops.
// but, we are actively trying to ignore pushes/pops related to calling another function.
// return true if push/pop coherence is OK.
// false otherwise.
bool	check_for_push_pop_coherence(Function_t *func)
{

	// count pushes in the prologue
	int prologue_pushes=count_prologue_pushes(func);

//cerr<<"Found "<<prologue_pushes<<" pushes in "<<func->GetName()<<endl;

	// keep a map that keeps the count of pops for each function exit.
	map<Instruction_t*, int> pop_count_per_exit;

	// now, look for pops, and fill in that map.
	for(
		set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		DISASM d;
		insn->Disassemble(d);
		if(strstr(d.CompleteInstr,"pop")!=NULL || strstr(d.CompleteInstr,"leave")!=NULL)
		{
			Instruction_t *exit_insn=find_exit_insn(insn,func);

			if(exit_insn)
			{
				DISASM d2;
				exit_insn->Disassemble(d2);
//cerr<<"Found exit insn ("<< d2.CompleteInstr << ") for pop ("<< d.CompleteInstr << ")"<<endl;
				map<Instruction_t*, int>::iterator mit;
				mit=pop_count_per_exit.find(exit_insn);
				if(mit == pop_count_per_exit.end())		// not found
					pop_count_per_exit[exit_insn]=0;	// init

				pop_count_per_exit[exit_insn]++;
			}
			else
			{
//cerr<<"Could not find exit insn for pop ("<< d.CompleteInstr << ")"<<endl;
			}
		}
	}

	// now, look at each exit with pops, and verify the count matches the push count.
	// we always allow an exit point from the function to have 0 pops, as things like
	// calls to non-return functions (e.g., exit, abort, assert_fail) don't clean up the
	// stack first.  Also handy as this allows "fixed" calls to be ignored.
	// but, since exits with 0 pops aren't in the map, we don't need an explicit check for them.
	for(
		map<Instruction_t*,int>::const_iterator it=pop_count_per_exit.begin();
		it!=pop_count_per_exit.end();
		++it
	   )
	{
		pair<Instruction_t*,int> map_pair=*it;
		Instruction_t* insn=map_pair.first;
		assert(insn);
		DISASM d;
		insn->Disassemble(d);

//cerr<<"Found "<<map_pair.second<<" pops in exit: \""<< d.CompleteInstr <<"\" func:"<<func->GetName()<<endl;

		// do the check
		if(prologue_pushes != map_pair.second)
		{
			cerr<<"Sanitizing function "<<func->GetName()<<" because pushes don't match pops for an exit"<<endl;
			return false;
		}
	}

	return true;
}


static ELFIO::section*  find_section(unsigned int addr, ELFIO::elfio *elfiop)
{
         for ( int i = 0; i < elfiop->sections.size(); ++i )
         {
                 ELFIO::section* pSec = elfiop->sections[i];
                 assert(pSec);
                 if(pSec->get_address() > addr)
                         continue;
                 if(addr >= pSec->get_address()+pSec->get_size())
                         continue;

                 return pSec;
        }
        return NULL;
}

bool PNTransformDriver::check_jump_tables(Instruction_t* insn)
{
	/* quick check to skip most instructions */
	if(insn->GetTarget() || insn->GetFallthrough())
		return true;

	DISASM d;
	insn->Disassemble(d);

	/* only look at jumps */
	int branch_type = d.Instruction.BranchType;
	if(branch_type!=JmpType)
		return true;

	/* make sure this is an indirect branch */
	if((d.Argument1.ArgType&MEMORY_TYPE)==0)
		return true;

	if(d.Argument1.Memory.Scale<4)
		return true;

	int displacement=d.Argument1.Memory.Displacement;

	ELFIO::section* pSec=find_section(displacement,elfiop);

	if(!pSec)
		return true;	

	const char *secdata=pSec->get_data();

	int offset=displacement-pSec->get_address();

	set<int> jump_tab_entries;
	for(int i=0;i<5;i++)
	{
		if(offset+i*4+sizeof(int) > pSec->get_size())
			break;

		const int *table_entry_ptr=(const int*)&(secdata[offset+i*4]);
		int table_entry=*table_entry_ptr;

		if(table_entry!=0)
		{
//			cout << "found possible table entry "<<hex<<table_entry<<" from func "<<insn->GetFunction()->GetName()<<endl;
			jump_tab_entries.insert(table_entry);
		}

	}

	for(
		set<Instruction_t*>::const_iterator it=orig_virp->GetInstructions().begin();
		it!=orig_virp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* ftinsn=*it;;
		AddressID_t* addr=ftinsn->GetAddress();
		int voff=addr->GetVirtualOffset();
	
		// check to see if this instruction is in my table 
		if(jump_tab_entries.find(voff)!=jump_tab_entries.end())
		{
			// it is!
	
			// now, check to see if the instruction is in my function 
			if(insn->GetFunction()!=ftinsn->GetFunction())
			{
				cout<<"Sanitizing function "<< insn->GetFunction()->GetName()<< "due to jump-table to diff-func detected."<<endl;
				return false;
			}
			else
			{
				// cout<<"Verified that the instruction is in my function, yay!"<<endl;
			}
		}
	}
	return true;

}

void PNTransformDriver::SanitizeFunctions()
{
	//TODO: for now, the sanitized list is only created for an individual IR file
	sanitized.clear();

	for(
		set<Function_t*>::const_iterator func_it=orig_virp->GetFunctions().begin();
		func_it!=orig_virp->GetFunctions().end();
		++func_it
		)
	{

		
		Function_t *func = *func_it;
		assert(func);

		if(func == NULL)
			continue;

		for(
			set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
			it!=func->GetInstructions().end();
			++it
			)
		{
			DISASM disasm;
			Instruction_t* instr = *it;

			if(instr == NULL)
				continue;

			instr->Disassemble(disasm);
			string disasm_str = disasm.CompleteInstr;

/*
			if(disasm_str.find("nop")!=string::npos && instr->GetFunction() == NULL)
				continue;
*/

			// check if there is a fallthrough from this function 	
			// into another function.  if so, sanitize both.
			if(FallthroughFunctionCheck(instr,instr->GetFallthrough()))
			{
				//check if instruciton is a call, unconditional jump, or ret
				//all other instructions should have targets within the same function
				//if not, filter the functions
				int branch_type = disasm.Instruction.BranchType;
				if(branch_type!=RetType && branch_type!=JmpType && branch_type!=CallType)
					// check if instr->GetTarget() (for cond branches) 
					// exits the function.  If so, sanitize both funcs.
					TargetFunctionCheck(instr,instr->GetTarget());

			
			}

			// if it's not already sanitized
			if(sanitized.find(func)==sanitized.end())
			{
				// check for push/pop coherence.
				if(!check_jump_tables(instr))
				{
					jump_table_sanitized++;
					sanitized.insert(func);
					continue;
				}
			}
		}

		// if it's not already sanitized
		if(sanitized.find(func)==sanitized.end())
		{
			// check for push/pop coherence.
			if(!check_for_push_pop_coherence(func))
			{
				push_pop_sanitized_funcs++;	
				sanitized.insert(func);
				continue;
			}
		}

	}
	//TODO: print sanitized list. 

	cerr<<"Sanitization Report:"<<"\nThe following "<<sanitized.size()<<
		" functions were sanitized from this file:"<<endl;
	for(
		set<Function_t*>::const_iterator it=sanitized.begin();
		it!=sanitized.end();
		++it
		)
	{
		Function_t *func=*it;
		if(func != NULL)
			cerr<<"\t"<<func->GetName()<<endl;
	}
}

inline bool PNTransformDriver::FallthroughFunctionCheck(Instruction_t* a, Instruction_t* b)
{
	if(a == NULL || b == NULL)
		return true;

	if(a->GetFunction() == NULL || b->GetFunction() == NULL)
		return true;

	return FunctionCheck(a->GetFunction(),b->GetFunction());

}

inline bool PNTransformDriver::FunctionCheck(Function_t* a,Function_t* b)
{
	if(a != b)
	{
		//To avoid null pointer exceptions I am not going to sanitize the
		//null function
		if(a != NULL)
			sanitized.insert(a);
		if(b != NULL)
			sanitized.insert(b);
		return false;
	}
	return true;
}


inline bool PNTransformDriver::TargetFunctionCheck(Instruction_t* a, Instruction_t* b)
{
	if(a == NULL || b == NULL)
		return true;

	return FunctionCheck(a->GetFunction(),b->GetFunction());
}

//Speculation note:
//Hypothesis generation assessment and refinement prior to modification.
//hypothesis assessment and refinement after modification is performed by the recursive validation subroutine. 
void PNTransformDriver::GenerateTransformsHidden(map<string,double> &file_coverage_map)
{
	SanitizeFunctions();

	vector<validation_record> high_covered_funcs, low_covered_funcs, not_covered_funcs,shuffle_validate_funcs;

	int funcs_attempted=-1;

	//For each function
	//Loop through each level, find boundaries for each, sort based on
	//the number of boundaries, attempt transform in order until successful
	//or until all inferences have been exhausted
	for(
		set<Function_t*>::const_iterator it=orig_virp->GetFunctions().begin();
		it!=orig_virp->GetFunctions().end()&&!timeExpired;
		++it
		)

	{
		Function_t *func = *it;

		funcs_attempted++;
		if(getenv("PN_ONLYTRANSFORM") && funcs_attempted!=atoi(getenv("PN_ONLYTRANSFORM")))
		{
			cout<<"Skipping function "<<dec<<funcs_attempted<<", named: "<<func->GetName()<<endl;
			continue;
		}
		if(getenv("PN_NUMFUNCSTOTRY") && funcs_attempted>=atoi(getenv("PN_NUMFUNCSTOTRY")))
		{
			cerr<<dec<<"Aborting transforms after func number "<<funcs_attempted<<", which is: "<<
				func->GetName()<<endl;
			break;
		}
		
		

		//TODO: remove this at some point when I understand if this can happen or not
		assert(func != NULL);

		cerr<<"PNTransformDriver: Function: "<<orig_virp->GetFile()->GetURL()<<" "<<func->GetName()<<endl;

		//Check if in blacklist
		if(IsBlacklisted(func))
		{
			continue;
		}

		total_funcs++;

		//Level in the hierarchy to start at
		int level=0;
		double func_coverage = 0;

		//see if the function is in the coverage map
		if(file_coverage_map.find(func->GetName()) != file_coverage_map.end())
		{
			//if coverage exists, if it is above or equal to the threshold
			//do nothing, otherwise set hierachy to start at the level
			//passed. 
			func_coverage = file_coverage_map[func->GetName()];
		}

		//Function coverage must be strictly greater than the threshold
		if(func_coverage <= coverage_threshold)
		{
			if(no_validation_level < 0)
			{
				cout<<"PNTransformDriver: Function "<<func->GetName()<<
					" has insufficient coverage, aborting transformation"<<endl;
				continue;
			}
			else
			{
				level = no_validation_level;

				cout<<"PNTransformDriver: Function "<<func->GetName()<<
					" has insufficient coverage, starting at hierarchy at "
					" level "<<level<<endl;
			}
		}


		//Get a layout inference for each level of the hierarchy. Sort these layouts based on
		//on the number of memory objects detected (in descending order). Then try each layout
		//as a basis for transformation until one succeeds or all layouts in each level of the
		//hierarchy have been exhausted. 

		//TODO: need to properly handle not_transformable and functions failing all transforms. 
		vector<PNStackLayout*> layouts; 
		for(;level<(int)transform_hierarchy.size() && layouts.size()==0;level++)
		{
			layouts = GenerateInferences(func, level);
		}


		//If the number of null inferences encountered equals the number of inferences
		//that are possible (based on the starting level which may not be 0), then
		//the function is not transformable. 
//		if((int)transform_hierarchy.size()-starting_level == null_inf_count)
//			not_transformable.push_back(func->GetName());
		//TODO: is there a better way of checking this?
//		else

		if(layouts.size() == 0)
		{
			not_transformable.push_back(func->GetName());
			continue;
		}


		sort(layouts.begin(),layouts.end(),CompareBoundaryNumbersDescending);

		validation_record vr;
		vr.hierarchy_index=level;
		vr.layout_index=0;
		vr.layouts=layouts;
		vr.func=func;

		//For logging purposes I want to know if the layout for with the
		//PN stack layout is a static stack frame. Since all inferences
		//based the same stack frame should record the same result, taking
		//the result from the first inference is okay. 
		if(!layouts[0]->IsStaticStack())
			dynamic_frames++;

		//Handle covered Shuffle Validate functions separately. 
		//I only suspect to see one function needing shuffle validation (main).
		//Don't shuffle here. 
		if(layouts[0]->DoShuffleValidate() && func_coverage != 0)
		{
			//Note: Do not transform these functions, handled that elsewhere.
			shuffle_validate_funcs.push_back(vr);

			if(func_coverage > coverage_threshold)
				high_coverage_count++;
			else
				low_coverage_count++;
		}
		else
		{
			//Else go ahead and transform the layout. 
			layouts[0]->Shuffle();
			layouts[0]->AddRandomPadding(do_align);

			if(func_coverage == 0)
				not_covered_funcs.push_back(vr);
			else if(func_coverage <= coverage_threshold)
				low_covered_funcs.push_back(vr);
			else
				high_covered_funcs.push_back(vr);
		}

	}

	//TODO: I have seen some functions not require any modifications. In this case we should remove them from the
	//vector. Not sure the best way to do this yet. For now ignore, because I have only seen it happen once (function start)

	//sort the validation records by the number of variables detected,
	//to optimize validation time. 
	sort(high_covered_funcs.begin(),high_covered_funcs.end(),CompareValidationRecordAscending);
	sort(low_covered_funcs.begin(),low_covered_funcs.end(),CompareValidationRecordAscending);

	//TODO: maybe validate those layouts with 3 or fewer variables first?
	//the sorting approach I now do might sufficiently optimize that this
	//isn't necessary. 
	Validate_Recursive(high_covered_funcs,0,high_covered_funcs.size());


	//In theory, functions with no coverage will not have any benefit from validation
	//but coverage can sometimes be wrong. For example, if PIN failed, perhaps
	//the coverage reflects functions that were executed only if the test
	//run fails. Go ahead and attempt binary validation on non-covered functions.
	//As an optimization I will validate non-covered funcs with functions with low coverage
	//but in case a validation failure does occur, functions with no coverage
	//are append to the low_covered_funcs vector. Appending to the end, as opposed to
	//using one data structure for both should optimize binary search in the 
	//even of a validation failure, since all functions without coverage are clustered,
	//and it is assumed functions with no coverage _SHOULD_ validate all the time. 
	low_covered_funcs.insert(low_covered_funcs.end(),not_covered_funcs.begin(),not_covered_funcs.end());
	Validate_Recursive(low_covered_funcs,0,low_covered_funcs.size());

	//NOTE: if you decide to handle not_covered_funcs separately,
	//make sure you either use Validate_Recursive, or rewrite
	//the instructions yourself.

	cerr<<"Functions to shuffle validate: "<<shuffle_validate_funcs.size()<<endl;
	ShuffleValidation(shuffle_validate_funcs);

	if(!Validate(NULL,string(basename(orig_virp->GetFile()->GetURL().c_str()))+"_accum"))
	{
		cerr<<"TEST ERROR: File: "<<orig_virp->GetFile()->GetURL()<<" does not pass accumulation validation, ignoring the file for now."<<endl;

		//TODO: carefully undo all finalization for this file, for now, 
		//just remove the FileIR_t from the registered_firps 
		registered_firps.erase(orig_virp);

		return;
	}
	else
	{
		cerr<<"File Accumulation Validation Success"<<endl;
	}

	high_coverage_count +=high_covered_funcs.size();
	low_coverage_count +=low_covered_funcs.size();
	no_coverage_count +=not_covered_funcs.size();

	//TODO: print file report
}

//returns true if all validate vrs validate without any failures. 
void PNTransformDriver::ShuffleValidation(vector<validation_record> &vrs)
{
//	bool success = true;
	for(unsigned int i=0;i<vrs.size()&&!timeExpired;i++)
	{
		PNStackLayout *layout = vrs[i].layouts[vrs[i].layout_index];
		
		while(layout != NULL&&!timeExpired)
		{
			//using padding transform handler since I now 
			//have PNStackLayout objects preven padding or shuffling
			//if it is not appropriate. In theory I could just use
			//one handler now, but since I know a canary should not
			//be used here, I will explicitly use a function that
			//does not even attempt it. 
			if(!PaddingTransformHandler(layout,vrs[i].func,true))
			{
//				success = false;
				//we will assume all subsequent layouts require shuffle validation. 
				//if it doesn't, the padding transform handler will add
				//padding and shuffle without shuffle validation.
				//Really this means if we have a P1 layout, we wont
				//shuffle it because it can't be, so padding will be
				//added and validated once. I make the assumption a layout
				//never can switch to suddenly being canary safe. 
				layout = Get_Next_Layout(vrs[i]);
			}
			else
			{
				//else transformation was success, register the validation record 
				Register_Finalized(vrs,i,1);
				break;
			}
		}

		if(layout == NULL)
		{
			failed.push_back(vrs[i].func);
			cout<<"Shuffle Validation: Function: "<<vrs[i].func->GetName()<<" has no additional inferences."<<endl;
		}
	}

//	return success;
}

//Alters the passed in validation record to record the layout information, and
//for simplicity returns the next layout object (PNStackLayout*) 
//returns NULL if no layout is next. 
PNStackLayout* PNTransformDriver::Get_Next_Layout(validation_record &vr)
{
    //if the layout is not the last in the vector of layouts for the hierarchy, set the layout to the next layout
	if(vr.layout_index != vr.layouts.size()-1)
	{
		vr.layout_index++;
	}
	//otherwise, continue looping through the hierarchy until inferences are generated. 
	else
	{
		vector<PNStackLayout*> layouts;
		unsigned int level=vr.hierarchy_index;
		for(;level<(unsigned int)transform_hierarchy.size()&&layouts.size()==0;level++)
		{
			layouts = GenerateInferences(vr.func, level);
		}

		//If no layouts are found, then this function fails all validation attempts
		if(layouts.size() == 0)
		{
			return NULL;
		}
		else
		{
			sort(layouts.begin(),layouts.end(),CompareBoundaryNumbersDescending);

			//update info in the validation record. 
			vr.layout_index=0;
			vr.layouts=layouts;
			vr.hierarchy_index=level;
		}
	}

	return vr.layouts[vr.layout_index];
}

int intermediate_report_count=0;
void PNTransformDriver::Register_Finalized(vector<validation_record> &vrs,unsigned int start, int length)
{
	if(length == 0)
		return;

	cout<<"Register Finalized: Registering "<<length<<" functions"<<endl;

	for(int i=0;i<length;i++)
	{
		unsigned int index=i+start;
		finalize_record fr;
		fr.layout = vrs[index].layouts[vrs[index].layout_index];
		fr.func = vrs[index].func;
		fr.firp = orig_virp;
		registered_firps.insert(orig_virp);
		finalization_registry.push_back(fr);
		//placing layout in the history here, although the information
		//could change when the modification is finalized. 
		transformed_history[fr.layout->GetLayoutName()].push_back(fr.layout);

//DEBUG: turning off undo to accumulate for now. A bug in modifying zsh
//seems to indicate this functionality (registration) is broken. 
//		undo(vrs[index].func); //make sure this function is registered only (undo any prior mods)
		cout<<"registering "<<fr.func->GetName()<<" layout "<<fr.layout->GetLayoutName()<<endl;
	}

	intermediate_report_count += length;

	//print out intermedaite report every if the report count exceeds or equals 50 registered funcs
	//since the last report. 
	if(intermediate_report_count >= 50)
	{
		cerr<<"############################INTERMEDIATE SUMMARY############################"<<endl;
		Print_Report();
		intermediate_report_count=0;
	}
}

//Assumed that passed in layouts have been transformed and are ready for validation. 
bool PNTransformDriver::Validate_Recursive(vector<validation_record> &vrs, unsigned int start, int length)
{
	if(timeExpired || length <= 0 || vrs.size()==0)
		return false;

	assert(start+length-1 != vrs.size());

	cout<<"Validate Recursive: validating "<<length<<" funcs of "<<vrs.size()<<endl;

	//Rewrite all funcs
	for(int i=0;i<length;i++)
	{
		unsigned int index=i+start;
		PNStackLayout  *layout = vrs[index].layouts[vrs[index].layout_index];

		//make sure the fucntions isn't already modified
		undo(vrs[index].func);

		//Canary rewrite will determine if layout can be canaried.
		//finalization will remove canaries if appropriate. 
		Canary_Rewrite(layout,vrs[index].func);
	}

	validation_count++;
	stringstream ss;
	ss<<"validation"<<validation_count;

	if(Validate(orig_virp,ss.str()))
	{
		Register_Finalized(vrs,start,length);
		return true;
	}
	else
	{
		//TODO: optimize here
		for(int i=0;i<length;i++)
		{
			unsigned int index = i+start;
			undo(vrs[index].func);
		}

		if(length == 1)
		{
			cout<<"Validate Recursive: Found problem function: "<<vrs[start].func->GetName()<<" validating linearly"<<endl;

			if(Get_Next_Layout(vrs[start])==NULL)
			{
				failed.push_back(vrs[start].func);
				cout<<"Validate Recursive: Function: "<<vrs[start].func->GetName()<<" has no additional inferences."<<endl;
				return false;
			}

			vrs[start].layouts[vrs[start].layout_index]->Shuffle();
			vrs[start].layouts[vrs[start].layout_index]->AddRandomPadding(do_align);

			Validate_Recursive(vrs,start,length);
			return false;
		}
		else
		{
			//Algorithm Idea: undo modifications half at a time, until the side of the vector of mods
			//containing the error is found. If found, send to Validate_Recursive.
			//Finalize all remaining. 

			bool left_val, right_val;
			left_val = Validate_Recursive(vrs,start,length/2);
			right_val = Validate_Recursive(vrs,start+length/2,(length-(length/2)));

			if(left_val && right_val)
				cerr<<"TESTING ERROR: TESTING THE WHOLE REPORTED FAILURE, TESTING THE HALVES REPORTS SUCCESS"<<endl;

			return false;
		}
		
	}
}





// 			//NOTE: this only_validate_list functionality may not be needed any more, consider removing
// 			//TODO: for now, only validate if the only_validate_list doesn't have any contents and
// 			//the level does not equal the never_validate level. I may want to allow the only validate
// 			//list to be empty, in which case a pointer is better, to check for NULL.
// 			bool do_validate = true;
// 			if(only_validate_list.size() != 0)
// 			{
// 				if(only_validate_list.find(func->GetName()) == only_validate_list.end())
// 					do_validate = false;
// 			}

// 			do_validate = do_validate && (level != no_validation_level);
		

// 			//Go through each layout in the level of the hierarchy in order. 
// 			for(unsigned int i=0;i<layouts.size()&&!timeExpired;i++)
// 			{
// 				//TODO: I need to have the ability to make transformations optional
// 				if(layouts[i]->IsCanarySafe())
// 					success = CanaryTransformHandler(layouts[i],func,do_validate);
// 				else if(layouts[i]->IsPaddingSafe())
// 					success = PaddingTransformHandler(layouts[i],func,do_validate);
// 				//if not canary or padding safe, the layout can only be randomized
// 				else
// 					success = LayoutRandTransformHandler(layouts[i],func,do_validate);

// 				if(!success && (int)transform_hierarchy.size()-1 == level && i == layouts.size()-1)
// 						failed.push_back(func);
// 					else if(success)
// 						break;
// 			}
// 		}	
// 	}
// }

void PNTransformDriver::Finalize_Transformation()
{
	cout<<"Finalizing Transformation: Committing all previously validated transformations ("<<finalization_registry.size()<<" functions)"<<endl;
// 	set<FileIR_t*> firps;

// 	for(vector<finalize_record>::iterator it = finalization_registry.begin(); it != finalization_registry.end(); it++)
// 	{
// 		finalize_record fr = *it;
// 		Function_t *func;
// 		PNStackLayout *layout;
// 		FileIR_t *firp;

// 		func = fr.func;
// 		layout = fr.layout;
// 		firp = fr.firp;

// 		assert(func != NULL && layout != NULL && firp != NULL);
// 		firps.insert(firp);

// //DEBUG: This code needs to be put back, especially for removing canaries
// //if do_canaries is false, but at the moment, accumulating modifications
// //works for zsh, making it appear that delayed modification is broken. 

// 		// orig_virp = firp;

// 		//Make sure any previous modificaitons are undone. 
// 		//undo(func);

// 		// //TODO: really there should be no need to retransform, but it
// 		// //is much easier to retransform than to retain the modified
// 		// //instructions previously made. 
// 		// if(do_canaries)
// 		// 	Canary_Rewrite(layout,func);
// 		// else
// 		// 	Sans_Canary_Rewrite(layout,func);
// 	}

	//TODO: one more validation? 
	cerr<<"Sanity validation check....."<<endl;

	// string dirname = "p1.xform/validation_final";
	// string cmd = "mkdir -p " + dirname;
	// system(cmd.c_str());

	// string aspri_filename = string(get_current_dir_name()) + "/" + dirname + "/a.irdb.aspri";
	// string bspri_filename = string(get_current_dir_name()) + "/" + dirname + "/a.irdb.bspri";

	// ofstream aspriFile;
	// aspriFile.open(aspri_filename.c_str(),ios_base::out);
	
	// if(!aspriFile.is_open())
	// {
	// 	assert(false);
	// }

	// for(set<FileIR_t*>::iterator it=firps.begin();
	// 	it!=firps.end();
	// 	++it
	// 	)
	// {
	// 	FileIR_t *firp = *it;
	// 	firp->GenerateSPRI(aspriFile,false);
	// }

	// aspriFile.close();

	// char new_instr[1024];
	// //This script generates the aspri and bspri files; it also runs BED
	// sprintf(new_instr, "%s %d %s %s", BED_script.c_str(), orig_progid, aspri_filename.c_str(), bspri_filename.c_str());
	
	// //If OK=BED(func), then commit	
	// int rt=system(new_instr);
	// int actual_exit = -1, actual_signal = -1;
	// if (WIFEXITED(rt)) actual_exit = WEXITSTATUS(rt);
	// else actual_signal = WTERMSIG(rt);
	// int retval = actual_exit;

	if(	!Validate(NULL,"validation_final"))
	{
		cerr<<"TEST ERROR: Sanity validation failed!! Backing off all transformations (PN disabled for this program)."<<endl;
		return;
	}
	else
		cerr<<"Sanity validation passed."<<endl;

	//Commit changes for each file. 
	//TODO: is this necessary, can I do one write?
	for(set<FileIR_t*>::iterator it=registered_firps.begin();
		it!=registered_firps.end();
		++it
		)
	{
		FileIR_t *firp = *it;
		cout<<"Writing to DB: "<<firp->GetFile()->GetURL()<<endl;
		firp->WriteToDB();
	}
}

void PNTransformDriver::Print_Report()
{
	cerr<<dec<<endl;
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
			if(layouts[laynum]->IsP1())
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
	cerr<<"Functions validated exceeding threshold: "<<high_coverage_count<<endl;
	cerr<<"Functions validated with non-zero coverage below or equal to threshold: "<<low_coverage_count<<endl;
	cerr<<"Functions modified with no coverage: "<<no_coverage_count<<endl;
	cerr<<"Total recursive validations performed: "<<validation_count<<endl;
	cerr<<"----------------------------------------------"<<endl;
	cerr<<"Total dynamic stack frames: "<<dynamic_frames<<endl;

	cerr<<"----------------------------------------------"<<endl;
	cerr<<"Non-Blacklisted Functions \t"<<total_funcs<<endl;
	cerr<<"Blacklisted Functions \t\t"<<blacklist_funcs<<endl;
	cerr<<"Sanitized Functions \t\t"<<sanitized_funcs<<endl;
	cerr<<"Push/Pop Sanitized Functions \t\t"<<push_pop_sanitized_funcs<<endl;
	cerr<<"Jump table Sanitized Functions \t\t"<<jump_table_sanitized<<endl;
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
	if(!layout->DoShuffleValidate())
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
		else if(!Validate(orig_virp,func->GetName()))
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

// Validate the modifications for the passed in FileIR_t*, creating
// a directory structure for the generated spri files in a directory
// reflecting the passed in string. 
//
// If the FileIR_t* is null, Validate will generate spri
// for as FileIR_t* that have been registered in the FileIR_t*
// registry (entries in this structure are made whenever 
// a function has been registered for finalization). 
// Passing NULL essentially provides a mechanism of revalidating all
// previously modified and validated functions. 
bool PNTransformDriver::Validate(FileIR_t *virp, string name)
{
	cerr<<"PNTransformDriver: Validate(): "<<name<<endl;

	string dirname = "p1.xform/" + name;
	string cmd = "mkdir -p " + dirname;
	int res=system(cmd.c_str());
	assert(res!=-1);
	
	string aspri_filename = string(get_current_dir_name()) + "/" + dirname + "/a.irdb.aspri";
	string bspri_filename = string(get_current_dir_name()) + "/" + dirname + "/a.irdb.bspri";
	ofstream aspriFile;
	aspriFile.open(aspri_filename.c_str());
	
	if(!aspriFile.is_open())
	{
		assert(false);
	}
	
	cerr<<"Pre genreate SPRI"<<endl;

	if(virp == NULL)
	{
		//Generate spri for previous files
		for(set<FileIR_t*>::iterator it=registered_firps.begin();
			it!=registered_firps.end();
			++it
			)
		{
			FileIR_t *firp = *it;
			firp->GenerateSPRI(aspriFile,false);
		}
	}
	//generate spri for the current file
	else
		virp->GenerateSPRI(aspriFile,false); // p1.xform/<function_name>/a.irdb.aspri
	cerr<<"Post genreate SPRI"<<endl;
	aspriFile.close();

	char new_instr[1024];
	//This script generates the aspri and bspri files; it also runs BED
	sprintf(new_instr, "%s %d %s %s", BED_script.c_str(), orig_progid, aspri_filename.c_str(), bspri_filename.c_str());
	
	//If OK=BED(func), then commit	
	int rt=system(new_instr);
	int actual_exit = -1;
//	int actual_signal = -1;
	if (WIFEXITED(rt)) actual_exit = WEXITSTATUS(rt);
//	else actual_signal = WTERMSIG(rt);
	int retval = actual_exit;

	//TODO: I have set exit code status 3 to indicate spasm failure
	//if spasm fails, there is no point in continuing. 
	assert(retval != 3);

	//TODO: was I supposed to do something with actual_signal?
	
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
	ControlFlowGraph_t cfg(func);
	
	string esp_reg;
	if(FileIR_t::GetArchitectureBitWidth()==32)
		esp_reg="esp";
	else
		esp_reg="rsp";

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
			c.ret_offset += get_saved_reg_size(); 	

		//Now with the total size, subtract off the esp offset to the canary
		c.ret_offset = c.ret_offset - c.esp_offset;
		//The number should be positive, but we want negative so 
		//convert to negative
		c.ret_offset = c.ret_offset*-1;
	
		if(verbose_log)
			cerr << "c.canary_val = " << c.canary_val << "	c.ret_offset = " <<	 c.ret_offset << endl;

		canaries.push_back(c);
	}
	
	//bool stack_alloc = false;
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
		//if(!stack_alloc && regexec(&(pn_regex->regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
		if(regexec(&(pn_regex->regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
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
					Instruction_Rewrite(layout,instr,&cfg);
					continue;			
				}
			}

			//TODO: I need a check to see if the previous amount is equal to
			//the expect stack frame, a check is done is the inference
			//generation now, so it should be okay without it,

			stringstream ss;
			ss << hex << layout->GetAlteredAllocSize();
		
			disasm_str = "sub " + esp_reg +", 0x"+ss.str();

			if(verbose_log)
				cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;		
			//undo_list[instr] = instr->GetDataBits();
			//undo_list[instr] = copyInstruction(instr);
			undo_list[func][instr] = copyInstruction(instr);

			virp->RegisterAssembly(instr,disasm_str);
/*
  if(!instr->Assemble(disasm_str))
  return false;
*/
			//stack_alloc = true;

			for(unsigned int i=0;i<canaries.size();i++)
			{
				ss.str("");
				ss<<"mov dword ["<<esp_reg<<"+0x"<<hex<<canaries[i].esp_offset<<"], 0x"<<hex<<canaries[i].canary_val;
				instr = insertAssemblyAfter(virp,instr,ss.str());
				if(i==0)
					instr->SetComment("Canary Setup Entry: "+ss.str());
				else
					instr->SetComment("Canary Setup: "+ss.str());
			}
		}
		else if(regexec(&(pn_regex->regex_ret), disasm_str.c_str(),5,pmatch,0)==0)
		{
			if(verbose_log)
				cerr<<"PNTransformDriver: Canary Rewrite: inserting ret canary check"<<endl;

			//undo_list[instr] = instr->GetDataBits();
			//undo_list[instr] = copyInstruction(instr);
			undo_list[instr->GetFunction()][instr] = copyInstruction(instr);



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
		else if(layout->IsStaticStack() && regexec(&(pn_regex->regex_call), disasm_str.c_str(),5,pmatch,0)==0)
		{
		
			if(verbose_log)
				cerr<<"PNTransformDriver: Canary Rewrite: inserting call canary check"<<endl;
			//undo_list[instr] = copyInstruction(instr);
			undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

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
			if(!Instruction_Rewrite(layout,instr, &cfg))
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

	ControlFlowGraph_t cfg(func);

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

		if(!Instruction_Rewrite(layout,instr,&cfg))
			return false;
	}

	return true;
}



static Instruction_t* GetNextInstruction(Instruction_t *prev, Instruction_t* insn, Function_t* func)
{
	Instruction_t* ft=insn->GetFallthrough();
	Instruction_t* targ=insn->GetTarget();
	
	/* if there's a fallthrough, but no targ, and the fallthrough is in the function */
	if(ft &&  !targ && func->GetInstructions().find(ft)!=func->GetInstructions().end())
		return ft;

	/* if there's a target, but no fallthrough, and the target is in the function */
	if(!ft && targ && func->GetInstructions().find(targ)!=func->GetInstructions().end())
		return targ;

	return NULL;
}

//
// PNTransformDriver::prologue_offset_to_actual_offset - 
// 	Look in the CFG to see if instr  is in the prologue before the stack allocation instruction.  
//	If so, attempt to adjust offset so that it is relative to the offset after the stack has 
//	been allocated.  Return adjusted offset if succesfful, else unadjusted offset.
//
int PNTransformDriver::prologue_offset_to_actual_offset(ControlFlowGraph_t* cfg, Instruction_t *instr,int offset)
{
	Function_t* func=cfg->GetFunction();
	assert(func);
	BasicBlock_t* entry_block=cfg->GetEntry();
	if(!entry_block)
		return offset;

	/* find the instruction in the vector */
	set<Instruction_t*>::iterator it= func->GetInstructions().find(instr);

	/* if the instruction isn't in the entry block, give up now */
	if( it == func->GetInstructions().end())
		return offset;

	
	Instruction_t* insn=*it, *prev=NULL;

	for(;insn!=NULL; insn=GetNextInstruction(prev,insn, func))
	{
		assert(insn);
		DISASM d;
		insn->Disassemble(d);
		string disasm_str=d.CompleteInstr;
		
		if(strstr(d.CompleteInstr, "push")!=NULL)
			return offset;
	
		int max = PNRegularExpressions::MAX_MATCHES;
		regmatch_t pmatch[max];

		/* check for a stack alloc */
                if(regexec(&(pn_regex->regex_stack_alloc), d.CompleteInstr, 5, pmatch, 0)==0)
		{

                        if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0)
                        {
				string matched="";
                                int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
                                matched = disasm_str.substr(pmatch[1].rm_so,mlen);
                                //extract K
                                unsigned int ssize;
                                if(str2uint(ssize, matched.c_str()) != SUCCESS)
                                {
					return offset;
                                }
				// found! 
				// mov ... [rsp+offset] ...
				// sub rsp, ssize
				// note: offset is negative
			
				// sanity check that the allocation iss bigger than the neg offset
				assert((int)ssize==(unsigned)ssize);
				if(-offset>(int)ssize)
					return offset;

				// success
				return offset+ssize;

                        }
			return offset;

		}
		// else, check next instruction 

		
	}

	return offset;

}

inline bool PNTransformDriver::Instruction_Rewrite(PNStackLayout *layout, Instruction_t *instr, ControlFlowGraph_t* cfg)
{
	FileIR_t* virp = orig_virp;

	string esp_reg="esp";
	if(FileIR_t::GetArchitectureBitWidth()==64)
		esp_reg="rsp";

	int max = PNRegularExpressions::MAX_MATCHES;
	regmatch_t pmatch[max];
	memset(pmatch, 0,sizeof(regmatch_t) * max);

	string matched="";
	string disasm_str = "";
	DISASM disasm;

	instr->Disassemble(disasm);
	disasm_str = disasm.CompleteInstr;
	
	//the disassmebly of lea has extra tokens not accepted by nasm, remove those tokens
	if(regexec(&(pn_regex->regex_lea_hack), disasm_str.c_str(), max, pmatch, 0)==0)
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
		

	if(regexec(&(pn_regex->regex_stack_alloc), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(verbose_log)
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

				undo_list[instr->GetFunction()][instr] = copyInstruction(instr);
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
		
		disasm_str = "sub "+esp_reg+", 0x"+ss.str();

		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;		
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str))
  return false;
*/

		//stack_alloc = true;
	} 
	else if(regexec(&(pn_regex->regex_and_esp), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr << "PNTransformDriver: and_esp pattern matched, ignoring"<<endl;
/*
  cerr<<"PNTransformDriver: Transforming AND ESP instruction"<<endl;

  disasm_str = "nop";

  cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;

  undo_list[instr] = copyInstruction(instr);
  if(!instr->Assemble(disasm_str))
  return false;
*/
	}
	else if(regexec(&(pn_regex->regex_esp_scaled_nodisp), disasm_str.c_str(), max, pmatch, 0)==0) 
	{
		if(verbose_log)
			cerr<<"PNTransformDriver: Transforming ESP +scale w/o displacement Instruction ([esp+reg*scale])"<<endl;

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
				cerr<<"PNTransformDriver: Displacement of [esp+reg*scale] is Zero, Ignoring Transformation"<<endl;
			
			return true;
		}

		stringstream ss;
		ss<<hex<<new_offset;

		matched=((string)"+ 0x")+ss.str()+"]";

		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;	
		assert(mlen==1);

		disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;		
*/
		
	}
	else if(regexec(&(pn_regex->regex_esp_only), disasm_str.c_str(), max, pmatch, 0)==0) 
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

		matched = esp_reg+"+0x"+ss.str();
		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;	
		disasm_str.replace(pmatch[1].rm_so,mlen,matched);
		
		if(verbose_log)
			cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;		
*/
		
	}
#if 1
	else if(regexec(&(pn_regex->regex_esp_direct_negoffset), disasm_str.c_str(), 5, pmatch, 0)==0)
	{

		if(verbose_log)
		{
			cerr<<"PNTransformDriver: Transforming ESP-k Relative Instruction"<<endl;
		}

		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
		matched = disasm_str.substr(pmatch[1].rm_so,mlen);

		// extract displacement 
		int offset = strtol(matched.c_str(),NULL,0);

		//TODO: I don't think this can happen but just in case
		assert(offset > 0);

		int revised_offset=prologue_offset_to_actual_offset(cfg,instr,-offset);

		if(revised_offset<0)
		{	
			if(verbose_log)
				cerr<<"PNTransformDriver: ignoring, not in prologue "<<endl;
			
		}
		else
		{
			int new_location = layout->GetNewOffsetESP(revised_offset);
			int new_offset = new_location-layout->GetAlteredAllocSize();

			// sanity 
			assert(new_offset<0);
			
			stringstream ss;
			ss<<hex<<(- new_offset); 	// neg sign already in string
	
			matched = "0x"+ss.str();
			
			disasm_str.replace(pmatch[1].rm_so,mlen,matched);
			
			if(verbose_log)
				cerr<<"PNTransformDriver: New Instruction = "<<disasm_str<<endl;
			undo_list[instr->GetFunction()][instr] = copyInstruction(instr);
	
			virp->RegisterAssembly(instr,disasm_str);
		}
	}
#endif
//TODO: the regular expression order does matter, scaled must come first, change the regex so this doesn't matter  
	else if(regexec(&(pn_regex->regex_esp_scaled), disasm_str.c_str(), 5, pmatch, 0)==0 ||
			regexec(&(pn_regex->regex_esp_direct), disasm_str.c_str(), 5, pmatch, 0)==0)
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
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;
*/
	}
	//TODO: the regular expression order does matter, scaled must come first, change the regex so this doesn't matter
	//for lea esp, [ebp-<const>] it is assumed the <const> will not be in the stack frame, so it should be ignored.
	//this should be validated prior to rewrite (i.e., this is a TODO, it hasn't been done yet).
	else if(regexec(&(pn_regex->regex_ebp_scaled), disasm_str.c_str(), 5, pmatch, 0)==0 ||
			regexec(&(pn_regex->regex_ebp_direct), disasm_str.c_str(), 5, pmatch, 0)==0)
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
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

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
	else if(regexec(&(pn_regex->regex_scaled_ebp_index), disasm_str.c_str(), 5, pmatch, 0)==0)
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
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

		virp->RegisterAssembly(instr,disasm_str);

/*
  if(!instr->Assemble(disasm_str.c_str()))
  return false;
*/
	}
	else if(regexec(&(pn_regex->regex_stack_dealloc), disasm_str.c_str(), 5, pmatch, 0)==0)
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

		disasm_str = "add "+esp_reg+", 0x"+ss.str();
		
		//undo_list[instr] = instr->GetDataBits();
		//undo_list[instr] = copyInstruction(instr);
		undo_list[instr->GetFunction()][instr] = copyInstruction(instr);

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
		{
			cerr<<"PNTransformDriver: No Pattern Match";
			if(strstr(disasm_str.c_str(), "rsp")!=NULL || 
		  	   strstr(disasm_str.c_str(), "esp")!=NULL)
				cerr<<"BUT CAUTION *******************  esp/rsp found in instruction.";
			cerr<<endl;
		}
	}
	return true;
}



//TODO: there is a memory leak, I need to write a undo_list clear to properly cleanup
//void PNTransformDriver::undo(map<Instruction_t*, Instruction_t*> undo_list, Function_t *func)
void PNTransformDriver::undo(Function_t *func)
{
	string func_name = func->GetName();

	//rollback any changes
	cerr<<"PNTransformDriver: Undo Transform: "<<undo_list[func].size()<<" instructions to rollback for function "<<func_name<<endl;
	for(
		map<Instruction_t*, Instruction_t*>::const_iterator mit=undo_list[func].begin();
		mit != undo_list[func].end();
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

	for(set<Instruction_t*>::const_iterator it=inserted_instr[func].begin();
		it != inserted_instr[func].end();
		++it
		)
	{
		orig_virp->UnregisterAssembly(*it);
		orig_virp->GetInstructions().erase(*it);
		delete *it;
	}

	for(set<AddressID_t*>::const_iterator it=inserted_addr[func].begin();
		it != inserted_addr[func].end();
		++it
		)
	{
		orig_virp->GetAddresses().erase(*it);
		delete *it;
	}
	//reset_undo(func->GetName());

	undo_list.erase(func);
	inserted_instr.erase(func);
	inserted_addr.erase(func);
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


bool PNTransformDriver::WriteStackIRToDB()
{
    // DEBUG
    cerr << "Writing stack IR to IRDB!" << endl;

    PNIrdbManager irdb_manager(this->pidp->GetOriginalVariantID());
    if (!irdb_manager.TableExists())
    {
        irdb_manager.CreateTable();
    }
    else
    {
        irdb_manager.DeleteSource(PNIrdbManager::PEASOUP);
    }

    std::map< std::string,std::vector<PNStackLayout*> >::const_iterator it =
        transformed_history.begin();
    while (it != transformed_history.end())
    {
        vector<PNStackLayout*> layouts = it->second;
        for (unsigned int laynum = 0; laynum < layouts.size(); ++laynum)
        {
            // DEBUG
            cerr << "Function: " << layouts[laynum]->GetFunctionName() << endl;

            std::vector<PNRange*> mem_objects = layouts[laynum]->GetRanges();
            for(unsigned int j = 0; j < mem_objects.size(); j++)
            {
                libIRDB::db_id_t new_id = irdb_manager.InsertStackObject(
                    layouts[laynum]->GetFunctionName(),
                    mem_objects[j]->GetOffset(),
                    mem_objects[j]->GetSize(),
                    PNIrdbManager::PEASOUP);

                // DEBUG
                cerr<< "\tOffset = " << mem_objects[j]->GetOffset() << " Size = "<<mem_objects[j]->GetSize() << endl;

                // return with failure if any insertion fails
                if (new_id == -1)
                    return false;
            }
        }

        it++;
    }

    return true;
}

void sigusr1Handler(int signum)
{
	PNTransformDriver::timeExpired = true;
}
