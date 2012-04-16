
#include "OffsetInference.hpp"
#include "General_Utility.hpp"
#include "beaengine/BeaEngine.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <set>


using namespace std;
using namespace libIRDB;

//TODO: Use cfg entry point only, then use func instructions,

//TODO: matching reg expressions use max match constant

//TODO: negative offsets?

//TODO: what if func is null?

//TODO: The inferences generated are highly conservative in what functions
//are considered transformable. Look at how the dealloc_flag and alloc_count

OffsetInference::~OffsetInference()
{
    //It is assumed that all pointers in the maps are unique
    //this is supposed to be guaranteed by the mechanisms of this
    //object
    //TODO: add some asserts to ensure no double delete

    map<string,PNStackLayout*>::iterator it;

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

StackLayout* OffsetInference::SetupLayout(BasicBlock_t *entry, Function_t *func)
{
    unsigned int stack_frame_size = 0;
    int saved_regs_size = 0;
    int out_args_size = func->GetOutArgsRegionSize();
    bool has_frame_pointer = false;

    int max = PNRegularExpressions::MAX_MATCHES;
    regmatch_t pmatch[max];
    memset(pmatch, 0,sizeof(regmatch_t) * max);  

    assert(out_args_size >=0);

    for(
	vector<Instruction_t*>::const_iterator it=entry->GetInstructions().begin();
	it!=entry->GetInstructions().end();
	++it
	)
    {
	string matched;

	Instruction_t* instr=*it;
	string disasm_str;

	DISASM disasm;
	instr->Disassemble(disasm);
	disasm_str = disasm.CompleteInstr;

	cerr << "OffsetInference: SetupLayout(): disassembled line =  "<<disasm_str<< endl;
	
	//TODO: find push ebp, then count pushes to sub esp, stack alloc size and pushed size are fed to layout objects
	//TODO: for now I assume all pushes are 32 bits, is this a correct assumption?
	if(regexec(&(pn_regex.regex_push_ebp), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    cerr << "OffsetInference: SetupLayout(): Push EBP Found"<<endl;
	    has_frame_pointer = true;//if I see push ebp at all, then the frame pointer exists

	    if(stack_frame_size != 0)
	    {
		//TODO: handle this better
		cerr<<"OffsetInference: SetupLayout(): Stack Frame Already Allocated, Ignoring Push EBP"<<endl;
		continue;
	    }
	    else
	    {
		saved_regs_size = 0;
	    }
	}
	else if(regexec(&(pn_regex.regex_push_anything), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    cerr<<"OffsetInference: SetupLayout(): Push (anything) Found"<<endl;

	    if(stack_frame_size != 0)
	    {
		//TODO: handle this better
		cerr<<"OffsetInference: SetupLayout(): Stack Frame Already Allocated, Ignoring Push Instruction"<<endl;
		continue;
	    }
	    else
	    {
		//TODO: assuming 4 bytes here for saved regs
		saved_regs_size += 4;
	    }
	}
	else if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    cerr << "OffsetInference: FindAllOffsets(): Found Stack Alloc"<<endl;

	    //TODO: Is this the way this situation should be handled?
	    //The first esp sub instruction is considered the stack allocation, all other subs are ignored
	    //Given that I return when the first one is found, this is probably a useless check. 
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
		//stack_frame_size = strtol(matched.c_str(),NULL,0);
		if(str2uint(stack_frame_size, matched.c_str())!= SUCCESS)
		{
		    //If this occurs, then the found stack size is not a 
		    //constant integer, so it must be a register. 
		    
		    //TODO: is this what I really want to do?
		    cerr<<"OffsetInference: LayoutSetup(): Found non-integral stack allocation ("<<matched<<") before integral stack allocation, generating a null layout inference for function "<<func->GetName()<<endl; 
		    return NULL;
		}

		//else

		cerr<<"OffsetInference: LayoutSetup(): Stack alloc Size = "<<stack_frame_size<<
		    " Saved Regs Size = "<<saved_regs_size<<" out args size = "<<out_args_size<<endl;

		//There is now enough information to create the PNStackLayout objects
		return new StackLayout("All Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,out_args_size);
	    }
	}
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

    cerr<<"OffsetInference: FindAllOffsets(): Looking at Function = "<<func->GetName()<<endl;

    ControlFlowGraph_t cfg(func);

    BasicBlock_t *block = cfg.GetEntry();

    pn_all_offsets = SetupLayout(block,func);

    if(pn_all_offsets != NULL)
    {
	stack_frame_size = pn_all_offsets->GetAllocSize();
	saved_regs_size = pn_all_offsets->GetSavedRegsSize();
	int out_args_size = func->GetOutArgsRegionSize();
	bool has_frame_pointer = pn_all_offsets->HasFramePointer();
	assert(out_args_size >=0);

	pn_direct_offsets = new StackLayout("Direct Offset Inference", func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,out_args_size);
	pn_scaled_offsets = new StackLayout("Scaled Offset Inference", func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,out_args_size);
	//do not consider out args for p1
	pn_p1_offsets = new StackLayout("P1 Offset Inference", func->GetName(),stack_frame_size,saved_regs_size,has_frame_pointer,0);
    }
    else
    {
	direct[func->GetName()] = NULL;
	scaled[func->GetName()] = NULL;
	all_offsets[func->GetName()] = NULL;
	p1[func->GetName()] = NULL;
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

	cerr << "OffsetInference: FindAllOffsets(): disassembled line =  "<<disasm_str<< endl;

/*	
	//TODO: find push ebp, then count pushes to sub esp, stack alloc size and pushed size are fed to layout objects
	//TODO: for now I assume all pushes are 32 bits, is this a correct assumption?
	if(regexec(&(pn_regex.regex_push_ebp), disasm_str.c_str(), max, pmatch, 0)==0)
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
	else if(regexec(&(pn_regex.regex_push_anything), disasm_str.c_str(), max, pmatch, 0)==0)
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
		saved_regs_size += 4;
	    }
	}
	else if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
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
	if(regexec(&(pn_regex.regex_stack_dealloc_implicit), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    dealloc_flag = true;
	    //TODO: there needs to be a check of lea esp, [ebp-<const>] to make sure const is not in the current stack frame. 
	}
	else if(regexec(&(pn_regex.regex_ret), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    ++ret_cnt;
	}
	else if(regexec(&(pn_regex.regex_and_esp), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    //TODO: decide how to better handle this option.
	    //Right now I am going to enforce in PNTransformDriver that
	    //the alignment instruction is removed. 


	    cerr<<"OffsetInference: FindAllOffsets(): Layout is not canary safe"<<endl;
	    pn_direct_offsets->SetCanarySafe(false);
	    pn_scaled_offsets->SetCanarySafe(false);
	    pn_all_offsets->SetCanarySafe(false);
	    pn_p1_offsets->SetCanarySafe(false);
	}
	else if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
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
		    continue;
		    
		}
	    }

	    alloc_count++;
	    if(alloc_count >1)
	    {
		cerr<<"OffsetInference: integral stack allocations exceeded 1, abandon inference"<<endl;
		break;
	    }

	}
	else if(regexec(&(pn_regex.regex_esp_scaled), disasm_str.c_str(), max, pmatch, 0)==0)
	{
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
		cerr<<"OffsetInference: FindAllOffsets(): ESP Offset = "<<offset<<endl;
	    }
	}
	else if(regexec(&(pn_regex.regex_esp_direct), disasm_str.c_str(), max, pmatch, 0)==0)
	{
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
		cerr<<"OffsetInference: FindAllOffsets(): ESP Offset = "<<offset<<endl;
	    }
	}
	else if(regexec(&(pn_regex.regex_ebp_scaled), disasm_str.c_str(), max, pmatch, 0)==0)
	{
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
	else if(regexec(&(pn_regex.regex_ebp_direct), disasm_str.c_str(), max, pmatch, 0)==0)
	{
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
	else if(regexec(&(pn_regex.regex_stack_dealloc), disasm_str.c_str(), max, pmatch, 0)==0)
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
	    if(offset != stack_frame_size && offset != 0)
	    {
		cerr<<"OffsetInference: stack deallocation detected with different size of allocation, abandon inference"<<endl;
		//dealloc_flag = false;

		//TODO: hacked in for TNE, rewrite. 
		direct[func->GetName()] = NULL;
		scaled[func->GetName()] = NULL;
		all_offsets[func->GetName()] = NULL;
		p1[func->GetName()] = NULL;
		return;
//		break;
	    }

	}
	
	//TODO: this is a hack for cases when ebp is used as an index,
	//in these cases, only attempt P1 for now, but in the future
	//dynamic checks can be used to dermine what object is referred to. 
	else if(regexec(&(pn_regex.regex_scaled_ebp_index), disasm_str.c_str(), 5, pmatch, 0)==0)
	{
	    PN_safe = false;
	    //TODO: at this point I could probably break the loop, 
	}
	else
	{
	    cerr<<"OffsetInference: FindAllOffsets: No Pattern Match"<<endl;
	}
    }

//TODO: everything is horribly hacked and messy, redo this function. 

    //if no dealloc is found, set all inferences to null
    //TODO: this was hacked together quickly, one flag is preferable. 
    //TODO: there might be a memory leak here, see the objects deleted
    //at the end of this function.
    if(alloc_count>1)
    {

	cerr<<"OffsetInference: FindAllOffsets: Multiple integral stack allocations found, returning null inferences"<<endl;
	direct[func->GetName()] = NULL;
	scaled[func->GetName()] = NULL;
	all_offsets[func->GetName()] = NULL;
	p1[func->GetName()] = NULL;
	return;

    }
    else
    {

	if(!dealloc_flag && ret_cnt == 0)
	{
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

	//TODO: causes a memory leak since I may reset to NULL, redo
	direct[func->GetName()] = new PNStackLayout(*pn_direct_offsets);
	scaled[func->GetName()] = new PNStackLayout(*pn_scaled_offsets);
	all_offsets[func->GetName()] = new PNStackLayout(*pn_all_offsets);
	p1[func->GetName()] = new PNStackLayout(*pn_p1_offsets);

	if(!dealloc_flag)
	{
	    cerr<<"OffsetInference: FindAllOffsets: No Stack Deallocation Found"<<endl;  
	    if(!direct[func->GetName()]->CanShuffle())
	    {
		cerr<<"OffsetInference: FindAllOffsets: direct offset inference cannot be shuffled, generating null inference"<<endl;
		direct[func->GetName()] = NULL;
	    }

	    if(!scaled[func->GetName()]->CanShuffle())
	    {
		cerr<<"OffsetInference: FindAllOffsets: scaled offset inference cannot be shuffled, generating null inference"<<endl;
		scaled[func->GetName()] = NULL;
	    }

	    if(!all_offsets[func->GetName()]->CanShuffle())
	    {
		cerr<<"OffsetInference: FindAllOffsets: all offset inference cannot be shuffled, generating null inference"<<endl;
		all_offsets[func->GetName()] = NULL;
	    }

	    p1[func->GetName()] = NULL;
	    cerr<<"OffsetInference: FindAllOffsets: p1 inference by default cannot be shuffled, generating null inference"<<endl;
	}
	
	if(!PN_safe)
	{
	    cerr<<"OffsetInference: FindAllOffsets: Function not pn_safe, using only p1 (p1 may have been previously disabled)"<<endl;
	    direct[func->GetName()] = NULL;
	    scaled[func->GetName()] = NULL;
	    all_offsets[func->GetName()] = NULL;
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


PNStackLayout* OffsetInference::GetLayout(map<string,PNStackLayout*> &mymap,Function_t *func)
{
    //No layout found, find all offset boundaries
    if(mymap.find(func->GetName()) == mymap.end())
    {
	FindAllOffsets(func);
    }

    //At this point an entry should be made for the function
    assert(mymap.find(func->GetName()) != mymap.end());

    return mymap.find(func->GetName())->second;
}

string OffsetInference::GetInferenceName() const
{
    return "All Offsets Inference";
}
