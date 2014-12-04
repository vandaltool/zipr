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


#include "OffsetInference.hpp"
#include "General_Utility.hpp"
#include "beaengine/BeaEngine.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <set>
#include <fstream>
#include "globals.h"

using namespace std;
using namespace libIRDB;

extern int get_saved_reg_size();

//TODO: Use cfg entry point only, then use func instructions,

//TODO: matching reg expressions use max match constant

//TODO: negative offsets?

//TODO: what if func is null?

//TODO: everying operates on regex because when I first wrote this, I didn't
//know DISASM had much of this information. We should migrate to using
//this struct more. That goes for the entire PN code base as well. 

//TODO: The inferences generated are highly conservative in what functions
//are considered transformable. Look at how the dealloc_flag and alloc_count

OffsetInference::~OffsetInference()
{
	//It is assumed that all pointers in the maps are unique
	//this is supposed to be guaranteed by the mechanisms of this
	//object
	//TODO: add some asserts to ensure no double delete

	map<Function_t*,PNStackLayout*>::iterator it;

	for(it=direct.begin();it !=direct.end();it++)
	{
		delete (*it).second;
	}

	for(it=scaled.begin();it !=scaled.end();it++)
	{
		delete (*it).second;
	}

	for(it=all_offsets.begin();it !=all_offsets.end();it++)
	{
		delete (*it).second;
	}
}

/*
  void OffsetInference::GetInstructions(vector<Instruction_t*> &instructions,BasicBlock_t *block,set<BasicBlock_t*> &block_set)
  {
  instructions.insert(instructions.end(),block->GetInstructions().begin(),block->GetInstructions().end());
  block_set.insert(block);

  // cerr<<"OffsetInference: GetInstructions(): predecessors = "<<block->GetPredecessors().size()<<" successors = "<<block->GetSuccessors().size()<<endl;
	
  for(
  set<BasicBlock_t*>::const_iterator it = block->GetSuccessors().begin();
  it != block->GetSuccessors().end();
  ++it
  )
  {
  if(block_set.find(*it) == block_set.end())
  GetInstructions(instructions,*it,block_set); 
  }

  for(
  set<BasicBlock_t*>::const_iterator it = block->GetPredecessors().begin();
  it != block->GetPredecessors().end();
  ++it
  )
  {
  if(block_set.find(*it) == block_set.end())
  GetInstructions(instructions,*it,block_set); 
  }
  }
*/

StackLayout* OffsetInference::SetupLayout(Function_t *func)
{
	unsigned int stack_frame_size = 0;
	int saved_regs_size = 0;
	int out_args_size = func->GetOutArgsRegionSize();
	bool push_frame_pointer = false;
	bool save_frame_pointer = false;

	Instruction_t *entry = func->GetEntryPoint();

	if(pn_regex==NULL)
		pn_regex=new PNRegularExpressions;

	//	 bool has_frame_pointer = false;

	int max = PNRegularExpressions::MAX_MATCHES;
	regmatch_t pmatch[max];
	memset(pmatch, 0,sizeof(regmatch_t) * max);	 

	assert(out_args_size >=0);

	//TODO: find the fallthrough of the entry block, and loop to it if necessary. 

/*
	for(
		vector<Instruction_t*>::const_iterator it=entry->GetInstructions().begin();
		it!=entry->GetInstructions().end();
		++it
		)
*/
	//loop through fallthroughs of the entry (entry will be update on every iteration)
	//until entry is null, or entry has left the function. 
	while(entry != NULL && (entry->GetFunction()==func))
	{

		in_prologue[entry]=true;
		string matched;

		//Instruction_t* instr=*it;
		Instruction_t* instr = entry;
		string disasm_str;

		DISASM disasm;
		instr->Disassemble(disasm);
		disasm_str = disasm.CompleteInstr;

		if(verbose_log)
			cerr << "OffsetInference: SetupLayout(): disassembled line =  "<<disasm_str<< endl;
	
		//TODO: find push ebp, then count pushes to sub esp, stack alloc size and pushed size are fed to layout objects
		//TODO: for now I assume all pushes are 32 bits, is this a correct assumption?
		if(regexec(&(pn_regex->regex_push_ebp), disasm_str.c_str(), max, pmatch, 0)==0)
		{
			if(verbose_log)
				cerr << "OffsetInference: SetupLayout(): Push EBP Found"<<endl;

			push_frame_pointer = true;

			if(stack_frame_size != 0)
			{
				//TODO: handle this better
				if(verbose_log)
					cerr<<"OffsetInference: SetupLayout(): Stack Frame Already Allocated, Ignoring Push EBP"<<endl;

				entry = entry->GetFallthrough();
				continue;
			}

//TODO: ignoring this code for now, although it appears this code no longer
//makes sense anyway. Don't reset the saved regs if yous ee another push ebp
//just ignore it for now. EBP is usually pushed first, if it isn't
//it is likely not going to be used as a base pointer, in which case I really
//don't want to reset the saved regs count anyway. If it is a base pointer
//and pushed other than first, then I don't know how this func will work 
//anyway. 

//		else
//		{
//		saved_regs_size = 0;
//		}
		}
		else if(regexec(&(pn_regex->regex_save_fp), disasm_str.c_str(), max, pmatch, 0)==0)
		{
			save_frame_pointer = true;
		}
		else if(regexec(&(pn_regex->regex_push_anything), disasm_str.c_str(), max, pmatch, 0)==0)
		{
			if(verbose_log)
				cerr<<"OffsetInference: SetupLayout(): Push (anything) Found"<<endl;

			if(stack_frame_size != 0)
			{
				//TODO: handle this better
				if(verbose_log)
					cerr<<"OffsetInference: SetupLayout(): Stack Frame Already Allocated, Ignoring Push Instruction"<<endl;

				entry = entry->GetFallthrough();
				continue;
			}

//			cerr<<"PUSH FOUND: "<<disasm.CompleteInstr<<endl;
//			cerr<<"PUSH Argument1: "<<hex<<(disasm.Argument1.ArgType & 0xF0000000)<<endl;
//			cerr<<"PUSH Argument2: "<<hex<<(disasm.Argument2.ArgType & 0xF0000000)<<endl;
			//		cerr<<"CONST_TYPE = "<<hex<<CONSTANT_TYPE<<endl;

			//if the push is a constant, then check if the next instruction
			//is an unconditional jmp, if so, ignore the push, assume 
			//the push is part of fixed calls. 
			if((disasm.Argument2.ArgType & 0xF0000000) == CONSTANT_TYPE)
			{
				//Grab the pushed value
				assert(pmatch[1].rm_so >=0 && pmatch[1].rm_eo >=0);
				int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
				matched = disasm_str.substr(pmatch[1].rm_so,mlen);

				//cerr<<"DEBUG DEBUG: Disasm match: "<<disasm.CompleteInstr<<endl;

//				if((it+1) != entry->GetInstructions().end())
				if(entry->GetFallthrough() != NULL)
				{
//					Instruction_t* next=*(it+1);
					Instruction_t* next = entry->GetFallthrough();
					DISASM next_disasm;
					next->Disassemble(next_disasm);

					//cerr<<"DEBUG DEBUG: Disasm next match: "<<next_disasm.CompleteInstr<<endl;
					
					if(next_disasm.Instruction.BranchType == JmpType)
					{
						
						if(verbose_log)
							cerr<<"OffsetInference: SetupLayout(): Found push matching fix calls pattern, ignoring the push (i.e., not recording the bytes pushed)."<<endl;


						//find the indirect branch target instruction, and reset entry to this instruction, then continue execution of the loop. 

						int target_addr_offset;
						assert(str2int(target_addr_offset, matched.c_str())==SUCCESS);

						//TODO: it is better to make a map of ind branch targets, but this efficient enough for now. 
						
						//Setting entry to null is a primitive way of checking if the target is in the same function
						//if it isn't, entry will be NULL at the end of the loop.
						entry=NULL;
						for(
							set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
							it!=func->GetInstructions().end();
							++it
							)
						{
							Instruction_t *cur = *it;

							if(cur->GetIndirectBranchTargetAddress() == NULL)
								continue;
							
							int cur_ibta = (int)cur->GetIndirectBranchTargetAddress()->GetVirtualOffset();

							//The target instruction is found, set entry to point to this instruction
							//continue analysis from this instruction. 
							if(cur_ibta == target_addr_offset)
							{
								entry = cur;
								break;
							}
						}

						continue;
					}
				}
			}
			//else the push value is registered

			//TODO: assuming 4 bytes here for saved regs
			saved_regs_size += get_saved_reg_size();
		}
		else if(regexec(&(pn_regex->regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
		{
			if(verbose_log)
				cerr << "OffsetInference: FindAllOffsets(): Found Stack Alloc"<<endl;

			//TODO: Is this the way this situation should be handled?
			//The first esp sub instruction is considered the stack allocation, all other subs are ignored
			//Given that I return when the first one is found, this is probably a useless check. 
			if(stack_frame_size != 0)
			{
				if(verbose_log)
					cerr <<"OffsetInference: FindAllOffsets(): Stack Alloc Previously Found, Ignoring Instruction"<<endl;

				entry = entry->GetFallthrough();
				continue;
			}

			//extract K from: sub esp, K 
			if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
			{
				int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
				matched = disasm_str.substr(pmatch[1].rm_so,mlen);
				//extract K 
				//stack_frame_size = strtol(matched.c_str(),NULL,0);
				if(str2uint(stack_frame_size, matched.c_str())!= SUCCESS)
				{
					//If this occurs, then the found stack size is not a 
					//constant integer, so it must be a register. 
			
					//TODO: is this what I really want to do?
					if(verbose_log)
						cerr<<"OffsetInference: LayoutSetup(): Found non-integral stack allocation ("<<matched<<") before integral stack allocation, generating a null layout inference for function "<<func->GetName()<<endl; 
					return NULL;
				}

				//else

				if(verbose_log)
					cerr<<"OffsetInference: LayoutSetup(): Stack alloc Size = "<<stack_frame_size<<
						" Saved Regs Size = "<<saved_regs_size<<" out args size = "<<out_args_size<<endl;

				//TODO: with the new code for determine if a frame pointer exists
				//I don't consider the case where the frame poitner is pushed but
				//ebp is not setup like a frame pointer. In this case, ebp acts
				//like a real general purpose register. 
				//The hack for now is to check if ebp is pushed, but the frame pointer
				//is not saved. In this case, consider ebp as another saved reg
				//and add to the save of the saved regs.
				//When you fix this, look at PNTransformDriver in the canary_rewrite
				//subroutine. You will see a case where there is a check for the frame
				//pointer, and an additional 4 bytes is added. This should be removed
				//in the future to only use the size of the saved regs, but 
				//this changes any ebp relative offset calculations to remove 4 bytes. 
				//Confusing, I know. 
				if(push_frame_pointer&&!save_frame_pointer)
					saved_regs_size +=get_saved_reg_size();

				//There is now enough information to create the PNStackLayout objects
				if((unsigned)stack_frame_size<(unsigned)out_args_size)  // what?
				{
					cerr<<"****************************************************************"<<endl;
					cerr<<"****************************************************************"<<endl;
					cerr<<"**Insanity coming from STARS, out_args_size > stack_frame_size**"<<endl;
					cerr<<"****************************************************************"<<endl;
					cerr<<"****************************************************************"<<endl;
					return NULL;
				}
				return new StackLayout("All Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,(push_frame_pointer&&save_frame_pointer),out_args_size);
		
			}
		}
		entry = entry->GetFallthrough();
	}

	return NULL;
}

//TODO: what about moving esp into a register?

//TODO: Try catches for exceptions thrown by PNStackLayout, for now asserts will fail in PNStackLayout
void OffsetInference::FindAllOffsets(Function_t *func)
{
	StackLayout *pn_all_offsets = NULL;
	StackLayout *pn_direct_offsets = NULL;
	StackLayout *pn_scaled_offsets = NULL;
	StackLayout *pn_p1_offsets = NULL;
	//int out_args_size;

	int max = PNRegularExpressions::MAX_MATCHES;
	regmatch_t pmatch[max];
	memset(pmatch, 0,sizeof(regmatch_t) * max);	 
	unsigned int stack_frame_size = 0;
	unsigned int saved_regs_size = 0;
	int ret_cnt = 0;
	bool lea_sanitize=false;

/*
  out_args_size = func->GetOutArgsRegionSize();
  assert(out_args_size >= 0);
*/

	//TODO: hack for T&E to make inferences more conservative
	bool dealloc_flag=false;
	int alloc_count=0;

	//TODO: a hack for when ebp is used as an index. If found
	//only p1 should be attempted. 
	bool PN_safe = true;

	if(verbose_log)
		cerr<<"OffsetInference: FindAllOffsets(): Looking at Function = "<<func->GetName()<<endl;

//	ControlFlowGraph_t cfg(func);

//	BasicBlock_t *block = cfg.GetEntry();

	//TODO: this is an addition for TNE to detect direct recursion,
	//in the future the call graph should be analyzed to find all recursion. 
//	Instruction_t *first_instr = *(block->GetInstructions().begin());

	Instruction_t *first_instr = func->GetEntryPoint();

//	pn_all_offsets = SetupLayout(block,func);
	pn_all_offsets = SetupLayout(func);

	int out_args_size = func->GetOutArgsRegionSize();
	if(pn_all_offsets != NULL)
	{
		stack_frame_size = pn_all_offsets->GetAllocSize();
		saved_regs_size = pn_all_offsets->GetSavedRegsSize();
		bool has_frame_pointer = pn_all_offsets->HasFramePointer();
		assert(out_args_size >=0);

		pn_direct_offsets = new StackLayout("Direct Offset Inference", func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,out_args_size);
		pn_scaled_offsets = new StackLayout("Scaled Offset Inference", func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,out_args_size);
		//do not consider out args for p1
		pn_p1_offsets = new StackLayout("P1 Offset Inference", func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,0);
	}
	else
	{
		direct[func] = NULL;
		scaled[func] = NULL;
		all_offsets[func] = NULL;
		p1[func] = NULL;
		return;
	}

	//Just checking that the entry point has no predecessors
	//assert(block->GetPredecessors().size() !=0);
/*
//put all instructions into one vector
vector<Instruction_t*> instructions;
set<BasicBlock_t*> block_set;

GetInstructions(instructions,block,block_set);
if(instructions.size() != func->GetInstructions().size())
{
cerr<<"OffsetInference: FindAllOffsets(): Number of CFG found instructions does not equal Function_t found instructions"<<endl;
}

//Checking that GetInstructions hasn't screwed up
assert(instructions.size() != 0);
*/

//TODO: should I start modifying at the entry point? 
	for(
		set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
		)
		/*
		  for(
		  vector<Instruction_t*>::const_iterator it=instructions.begin();
		  it!=instructions.end();
		  ++it
		  )
		*/
	{
		string matched;

		Instruction_t* instr=*it;
		string disasm_str;

		DISASM disasm;
		instr->Disassemble(disasm);
		disasm_str = disasm.CompleteInstr;

		if(verbose_log)
			cerr << "OffsetInference: FindAllOffsets(): disassembled line =	 "<<disasm_str<< endl;

/*	
//TODO: find push ebp, then count pushes to sub esp, stack alloc size and pushed size are fed to layout objects
//TODO: for now I assume all pushes are 32 bits, is this a correct assumption?
if(regexec(&(pn_regex->regex_push_ebp), disasm_str.c_str(), max, pmatch, 0)==0)
{
cerr << "OffsetInference: FindAllOffsets(): Push EBP Found"<<endl;

if(stack_frame_size != 0)
{
//TODO: handle this better
cerr<<"OffsetInference: FindAllOffsets(): Stack Frame Already Allocated, Ignoring Push EBP"<<endl;
continue;
}
else
{
saved_regs_size = 0;
}
}
else if(regexec(&(pn_regex->regex_push_anything), disasm_str.c_str(), max, pmatch, 0)==0)
{
cerr<<"OffsetInference: FindAllOffsets(): Push (anything) Found"<<endl;

if(stack_frame_size != 0)
{
//TODO: handle this better
cerr<<"OffsetInference: FindAllOffsets(): Stack Frame Already Allocated, Ignoring Push Instruction"<<endl;
continue;
}
else
{
//TODO: assuming 4 bytes here for saved regs
saved_regs_size += get_saved_reg_size();
}
}
else if(regexec(&(pn_regex->regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
{
cerr << "OffsetInference: FindAllOffsets(): Found Stack Alloc"<<endl;

//TODO: Is this the way this situation should be handled?
//The first esp sub instruction is considered the stack allocation, all other subs are ignored
if(stack_frame_size != 0)
{
cerr <<"OffsetInference: FindAllOffsets(): Stack Alloc Previously Found, Ignoring Instruction"<<endl;
continue;
}
		

//extract K from: sub esp, K 
if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
{
int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
matched = disasm_str.substr(pmatch[1].rm_so,mlen);
//extract K 
stack_frame_size = strtol(matched.c_str(),NULL,0);

cerr<<"OffsetInference: FindAllOffsets(): Stack alloc Size = "<<stack_frame_size<<
" Saved Regs Size = "<<saved_regs_size<<" out args size = "<<out_args_size<<endl;

//There is now enough information to create the PNStackLayout objects
pn_all_offsets = new PNStackLayout("All Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,out_args_size);
pn_direct_offsets = new PNStackLayout("Direct Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,out_args_size);
pn_scaled_offsets = new PNStackLayout("Scaled Offset Layout", func->GetName(),stack_frame_size,saved_regs_size,out_args_size);
pn_p1_offsets = new PNStackLayout("P1 Layout",func->GetName(),stack_frame_size,saved_regs_size,out_args_size);	
}
}
else 
*/

	if(regexec(&(pn_regex->regex_push_anything), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		Instruction_t* ft=instr->GetFallthrough();
		if(ft && !ft->GetFallthrough() && 
			(ft->GetTarget()==NULL || ft->GetTarget()->GetFunction()!=instr->GetFunction()))
		{
			/* probably a push/jmp converted by fix calls */
			/* can ignore this push */
		}
		else if(!in_prologue[instr])
		{
			cerr<<"Found push instruction not in prologue, marking as not canary safe";
			cerr<<"Insn="<<disasm_str<<endl;
			pn_direct_offsets->SetCanarySafe(false);
			pn_scaled_offsets->SetCanarySafe(false);
			pn_all_offsets->SetCanarySafe(false);
			pn_p1_offsets->SetCanarySafe(false);
			
		}
	}


	/* check for an lea with an rsp in it -- needs to be done before other regex's */
	if(regexec(&(pn_regex->regex_lea_rsp), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"OffsetInference: lea_rsp found "<<endl;

                if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0)
                {
			if(verbose_log)
				cerr<<"OffsetInference: lea_rsp found const"<<endl;
                        int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
                        matched = disasm_str.substr(pmatch[1].rm_so,mlen);
                        // extract displacement

                        int offset = disasm.Argument2.Memory.Displacement;
			if(offset<0) /* huh? */
			{
				if(verbose_log)
					cerr<<"OffsetInference: lea_rsp neg offset sanitize"<<endl;
				lea_sanitize=true;
			}
			unsigned uoffset=(unsigned)offset;
			/* if this lea is pointing to saved regs -- huh? */
			if(uoffset>=stack_frame_size && uoffset<saved_regs_size+stack_frame_size)
			{
				if(verbose_log)
					cerr<<"OffsetInference: lea_rsp found in saved regs area"<<endl;
				lea_sanitize=true;
			}
			
		}

		
	}

	/* now, on to doing offset identification */
	if(regexec(&(pn_regex->regex_stack_dealloc_implicit), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		dealloc_flag = true;
		//TODO: there needs to be a check of lea esp, [ebp-<const>] to make sure const is not in the current stack frame. 
	}
	else if(regexec(&(pn_regex->regex_ret), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		++ret_cnt;
	}
	else if(regexec(&(pn_regex->regex_and_esp), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		//TODO: decide how to better handle this option.
		//Right now I am going to enforce in PNTransformDriver that
		//the alignment instruction is removed. 

		if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets(): Layout is not canary safe"<<endl;

		pn_direct_offsets->SetCanarySafe(false);
		pn_scaled_offsets->SetCanarySafe(false);
		pn_all_offsets->SetCanarySafe(false);
		pn_p1_offsets->SetCanarySafe(false);
	}
	else if(regexec(&(pn_regex->regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		//check if the stack allocation uses an integral offset. 

		//extract K from: sub esp, K 
		if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
		{
			int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
			matched = disasm_str.substr(pmatch[1].rm_so,mlen);
			//extract K 
			unsigned int scheck;
			if(str2uint(scheck, matched.c_str()) != SUCCESS)
			{
				//If this occurs, then the found stack size is not a 
				//constant integer, so it must be a register.

				//even though I am specifying only p1 should be performed
				//I am still going to set this flag for all transforms.
				pn_direct_offsets->SetStaticStack(false);
				pn_scaled_offsets->SetStaticStack(false);
				pn_all_offsets->SetStaticStack(false);
				pn_p1_offsets->SetStaticStack(false);
				PN_safe = false;

				//Consider this case not canary safe for now
				//TODO: can I make this canary safe?
				pn_direct_offsets->SetCanarySafe(false);
				pn_scaled_offsets->SetCanarySafe(false);
				pn_all_offsets->SetCanarySafe(false);
				pn_p1_offsets->SetCanarySafe(false);

				if(verbose_log)
					cerr<<"OffsetInferece:	instruction contains a dynamic stack allocation, not pn_safe"<<endl;

				//TODO: this output should be removed after TNE
				//only used to give Jason an indication that a 
				//non-static func has been detected. 

				ofstream dynstackfile;
				dynstackfile.open("dynamic_stack.log",fstream::out|fstream::app);
				if(dynstackfile.is_open())
				{
					//I don't think this can happen, but I really don't want
					//to add a null pointer exception to TNE
					if(instr == NULL || instr->GetAddress() == NULL)
					{
						dynstackfile<<func->GetName()<<" : "<<disasm_str<<endl;
					}
					else
					{
						dynstackfile<<func->GetName()<<" : "<<hex<<instr->GetAddress()->GetVirtualOffset()<<" : "<<disasm_str<<endl;
					}
					dynstackfile.close();
				}

				continue;
			
			}
		}

		alloc_count++;
		if(alloc_count >1)
		{
			if(verbose_log)
				cerr<<"OffsetInference: integral stack allocations exceeded 1, abandon inference"<<endl;
			break;
		}

	}
//TODO: hack for TNE 2, if we see a jmp to an esp or ebp relative address, ignore this function entirely
//The reasion is fix calls will fix an esp/ebp relative call by adding 4 to the original address and pushing
//before the inserted jmp. This gives the false impression that there is a boundary at this location
//and also gives a false impression that the location should be modified using the wrong boundary, even if
//p1 is used only. Specifically this occurred when the frame size was 0x20, and the call was to esp+0x1c
//the fix call because a jmp esp+0x20 which was outside the frame, and PN corrected by changing the offset
//to reflect the padding. 
	else if(disasm.Instruction.BranchType == JmpType)
	{
		if(regexec(&(pn_regex->regex_esp_scaled), disasm_str.c_str(), max, pmatch, 0)==0 ||
		   regexec(&(pn_regex->regex_esp_direct), disasm_str.c_str(), max, pmatch, 0)==0 ||
		   regexec(&(pn_regex->regex_ebp_scaled), disasm_str.c_str(), max, pmatch, 0)==0 ||
		   regexec(&(pn_regex->regex_ebp_direct), disasm_str.c_str(), max, pmatch, 0)==0)
		{
			cerr<<"OffsetInference: FindAllOffsets(): Layout contains a jmp relative to esp or ebp, ignore function for now"<<endl;
			
			direct[func] = NULL;
			scaled[func] = NULL;
			all_offsets[func] = NULL;
			p1[func] = NULL;

			//TODO: cleanup memory, since this is all so ugly at the moment, I'm inclined to leak memory than
			//to risk a segfault deleting a pointer. 
			return;
		}
		   
		   
	}
	else if(regexec(&(pn_regex->regex_esp_scaled), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets(): Found ESP Scaled Instruction"<<endl;
/*
  if(stack_frame_size <=0)
  {
  cerr<<"OffsetInference: FindAllOffsets(): Frame Alloc Not Found, Aborting Offset Search"<<endl;
  break;
  }
*/

		if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
		{
			int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
			matched = disasm_str.substr(pmatch[1].rm_so,mlen);
			// extract displacement 
			int offset = strtol(matched.c_str(),NULL,0);

			if(pn_all_offsets != NULL)
			{
				pn_all_offsets->InsertESPOffset(offset);
			}
			if(pn_scaled_offsets != NULL)
			{
				pn_scaled_offsets->InsertESPOffset(offset);
			}

			if(verbose_log)
				cerr<<"OffsetInference: FindAllOffsets(): ESP Offset = "<<offset<<endl;
		}
	}
	else if(regexec(&(pn_regex->regex_esp_direct), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets: Found ESP Direct Instruction"<<endl;
/*
  if(stack_frame_size <=0)
  {
  cerr<<"OffsetInference: FindAllOffsets(): Frame Alloc Not Found, Aborting Offset Search"<<endl;
  break;
  }
*/

		if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
		{
			int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
			matched = disasm_str.substr(pmatch[1].rm_so,mlen);
			// extract displacement 

			int offset = strtol(matched.c_str(),NULL,0);

			if(pn_all_offsets != NULL)
			{
				pn_all_offsets->InsertESPOffset(offset);
			}
			if(pn_direct_offsets != NULL)
			{
				pn_direct_offsets->InsertESPOffset(offset);
			}

			if(verbose_log)
				cerr<<"OffsetInference: FindAllOffsets(): ESP Offset = "<<offset<<endl;
		}
	}
	else if(regexec(&(pn_regex->regex_ebp_scaled), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets(): Found EBP Scaled Instruction"<<endl;
/*
  if(stack_frame_size <=0)
  {
  cerr<<"OffsetInference: FindAllOffsets(): Frame Alloc Not Found, Aborting Offset Search"<<endl;
  break;
  }
*/

		if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
		{
			int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
			matched = disasm_str.substr(pmatch[1].rm_so,mlen);

			// extract displacement 
			int offset = strtol(matched.c_str(),NULL,0);
		
/*
  if(stack_frame_size - offset < 0)
  {
  cerr<<"OffsetInference: FindAllOffsets: Detected Negative ESP Offset, Aborting Offset Search"<<endl;

  pn_all_offsets = NULL;
  pn_scaled_offsets = NULL;
  pn_direct_offsets = NULL;
  break;
  }
*/

			if(pn_all_offsets != NULL)
			{
				pn_all_offsets->InsertEBPOffset(offset);
			}
			if(pn_scaled_offsets != NULL)
			{
				pn_scaled_offsets->InsertEBPOffset(offset);
			}
		}
	}
	else if(regexec(&(pn_regex->regex_ebp_direct), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets(): Found EBP Direct Instruction"<<endl;
/*
  if(stack_frame_size <=0)
  {
  cerr<<"OffsetInference: FindAllOffsets(): Frame Alloc Not Found, Aborting Offset Search"<<endl;
  break;
  }
*/

		if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
		{
			int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
			matched = disasm_str.substr(pmatch[1].rm_so,mlen);

			// extract displacement 
			int offset = strtol(matched.c_str(),NULL,0);

/*
  if(stack_frame_size - offset < 0)
  {
  cerr<<"OffsetInference: FindAllOffsets: Detected Negative ESP Offset, Aborting Offset Search"<<endl;

  pn_all_offsets = NULL;
  pn_scaled_offsets = NULL;
  pn_direct_offsets = NULL;
  break;
  }
*/
			if(verbose_log)
				cerr<<"OffsetInference: FinadAllOffsets(): Extracted EBP offset = "<<offset<<endl;

			if(pn_all_offsets != NULL)
			{
				pn_all_offsets->InsertEBPOffset(offset);
			}
			if(pn_direct_offsets != NULL)
			{
				pn_direct_offsets->InsertEBPOffset(offset);
			}			
		}
	}
	else if(regexec(&(pn_regex->regex_stack_dealloc), disasm_str.c_str(), max, pmatch, 0)==0)
	{
		//if we find a dealloc, set a flag indicating as such
		dealloc_flag = true;

		//TODO: if the amount to dealloc is not equal to the stack frame size
		//exit inference
		int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
		matched = disasm_str.substr(pmatch[1].rm_so,mlen);

		// extract displacement 
		int offset = strtol(matched.c_str(),NULL,0);

		//NOTE: I have seen cases where there is an add esp, 0x0000000
		//in unoptimized code. In this case, the compiler must have
		//restored the stack already, ignore the instruction. 

		//TODO: casting stack_frame_size, make sure it isn't larger than
		//max int, I don't know what to do if I see this. 
		if(offset != (int)stack_frame_size && offset != 0)
		{
			if(verbose_log)
				cerr<<"OffsetInference: stack deallocation detected with different size of allocation, abandon inference"<<endl;
			//dealloc_flag = false;

			//TODO: hacked in for TNE, rewrite. 
			direct[func] = NULL;
			scaled[func] = NULL;
			all_offsets[func] = NULL;
			p1[func] = NULL;
			return;
//		break;
		}

	}
	
		//TODO: this is a hack for cases when ebp is used as an index,
		//in these cases, only attempt P1 for now, but in the future
		//dynamic checks can be used to dermine what object is referred to. 
	else if(regexec(&(pn_regex->regex_scaled_ebp_index), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		PN_safe = false;
		if(verbose_log)
			cerr<<"OffsetInference: instruction contains an ebp index, not pn_safe"<<endl;
		//TODO: at this point I could probably break the loop, 
	}
		//TODO: a hack for TNE to check for direct recursion to dial down padding
	else if(regexec(&(pn_regex->regex_call), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
		if(instr->GetTarget() != NULL && instr->GetTarget()->GetAddress() != NULL)
		{
			if(instr->GetTarget()->GetAddress()->GetVirtualOffset() == first_instr->GetAddress()->GetVirtualOffset())
			{
				if(verbose_log)
					cerr<<"OffsetInference: function contains a direct recursive call"<<endl;

				pn_direct_offsets->SetRecursive(true);
				pn_scaled_offsets->SetRecursive(true);
				pn_all_offsets->SetRecursive(true);
				pn_p1_offsets->SetRecursive(true);
			}
		}
	}
		
	else
	{
		if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets: No Pattern Match"<<endl;
	}
	}

//TODO: everything is horribly hacked and messy, redo this function. 

	//if no dealloc is found, set all inferences to null
	//TODO: this was hacked together quickly, one flag is preferable. 
	//TODO: there might be a memory leak here, see the objects deleted
	//at the end of this function.
	if(alloc_count>1 || lea_sanitize)
	{
		if(lea_sanitize)
			cerr<<"OffsetInference: FindAllOffsets: lea_rsp that points to saved regs found "<<endl;
		else if(verbose_log)
			cerr<<"OffsetInference: FindAllOffsets: Multiple integral stack allocations found, returning null inferences"<<endl;
		

		direct[func] = NULL;
		scaled[func] = NULL;
		all_offsets[func] = NULL;
		p1[func] = NULL;
		return;

	}
	else
	{

		if(!dealloc_flag && ret_cnt == 0)
		{
			if(verbose_log)
				cerr<<"OffsetInference: FindAllOffsets: Function is missing stack deallocaiton, but does not return, assuming transformable"<<endl;
			dealloc_flag = true;
		}
		//TODO: I need to revist this such that you can pass a pointer to PNStackLayout,
		//and handle NULL accordingly.

		//TODO: this has become to hacky, redo. 
		if(!dealloc_flag)
		{
			pn_direct_offsets->SetPaddingSafe(false);
			pn_scaled_offsets->SetPaddingSafe(false);
			pn_all_offsets->SetPaddingSafe(false);
			pn_p1_offsets->SetPaddingSafe(false);
		}

		unsigned int aoi_size = pn_all_offsets->GetRanges().size();
		//TODO: causes a memory leak since I may reset to NULL, redo

		//if the size of aoi is the same as any other inference
		//assume they are the same (insert a null layout entry)
		if(pn_direct_offsets->GetRanges().size() != aoi_size)
			direct[func] = new PNStackLayout(*pn_direct_offsets);
		else
			direct[func] = NULL;

		if(pn_scaled_offsets->GetRanges().size() != aoi_size)
			scaled[func] = new PNStackLayout(*pn_scaled_offsets);
		else
			scaled[func] = NULL;

		//TODO: BIG TODO: There is quite a delema here. If p1 is the same as
		//AOI, I don't want to generate it to save time, but what if a function
		//has no coverage, so p1 is used, if I set it null here because the
		//layouts are the same, I wont have any modification for that function. 
		p1[func] = new PNStackLayout(*pn_p1_offsets);

		all_offsets[func] = new PNStackLayout(*pn_all_offsets);

		if(!dealloc_flag)
		{
			if(verbose_log)
				cerr<<"OffsetInference: FindAllOffsets: No Stack Deallocation Found"<<endl;	 
			if(direct[func] != NULL && !direct[func]->IsShuffleSafe())
			{
				if(verbose_log)
					cerr<<"OffsetInference: FindAllOffsets: direct offset inference cannot be shuffled, generating null inference"<<endl;
				direct[func] = NULL;
			}

			if(scaled[func] != NULL && !scaled[func]->IsShuffleSafe())
			{
				if(verbose_log)
					cerr<<"OffsetInference: FindAllOffsets: scaled offset inference cannot be shuffled, generating null inference"<<endl;
				scaled[func] = NULL;
			}

			if(all_offsets[func] != NULL && !all_offsets[func]->IsShuffleSafe())
			{
				if(verbose_log)
					cerr<<"OffsetInference: FindAllOffsets: all offset inference cannot be shuffled, generating null inference"<<endl;
				all_offsets[func] = NULL;
			}

			p1[func] = NULL;
			if(verbose_log)
				cerr<<"OffsetInference: FindAllOffsets: p1 inference by default cannot be shuffled, generating null inference"<<endl;
		}
	
		if(!PN_safe)
		{
			if(verbose_log)
				cerr<<"OffsetInference: FindAllOffsets: Function not pn_safe, using only p1 (p1 may have been previously disabled)"<<endl;
			direct[func] = NULL;
			scaled[func] = NULL;
			all_offsets[func] = NULL;
		}
	}

	//memory clean up
	delete pn_direct_offsets;
	delete pn_scaled_offsets;
	delete pn_all_offsets;
	delete pn_p1_offsets;
}

//If map entry exists, return it, else perform boundary detection
//If no layout can be made, NULL is returned.
PNStackLayout* OffsetInference::GetPNStackLayout(Function_t *func)
{
	return GetLayout(all_offsets,func);
}

PNStackLayout* OffsetInference::GetDirectAccessLayout(Function_t *func)
{
	return GetLayout(direct,func);
}

PNStackLayout* OffsetInference::GetScaledAccessLayout(Function_t *func)
{
	return GetLayout(scaled,func);
}

PNStackLayout* OffsetInference::GetP1AccessLayout(Function_t *func)
{
	return GetLayout(p1,func);
}


PNStackLayout* OffsetInference::GetLayout(map<Function_t*,PNStackLayout*> &mymap,Function_t *func)
{
	//No layout found, find all offset boundaries
	if(mymap.find(func) == mymap.end())
	{
		FindAllOffsets(func);
	}

	//At this point an entry should be made for the function
	assert(mymap.find(func) != mymap.end());

	return mymap.find(func)->second;
}

string OffsetInference::GetInferenceName() const
{
	return "All Offsets Inference";
}
