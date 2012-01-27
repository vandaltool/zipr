
#include "OffsetInference.hpp"
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

 //TODO: I am always assuming lower case reg expressions, perhaps I should tolower all strings
PNStackLayout* OffsetInference::SetupLayout(BasicBlock_t *entry, Function_t *func)
{
    int stack_frame_size = 0;
    int saved_regs_size = 0;
    int out_args_size = func->GetOutArgsRegionSize();

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

		cerr<<"OffsetInference: LayoutSetup(): Stack alloc Size = "<<stack_frame_size<<
		    " Saved Regs Size = "<<saved_regs_size<<" out args size = "<<out_args_size<<endl;

		//There is now enough information to create the PNStackLayout objects
		return new PNStackLayout("All Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,out_args_size);
	    }
	}
    }

    return NULL;
}

//TODO: what about moving esp into a register?

//TODO: Try catches for exceptions thrown by PNStackLayout, for now asserts will fail in PNStackLayout

//TODO: For t&e this function will 
void OffsetInference::FindAllOffsets(Function_t *func)
{
    PNStackLayout *pn_all_offsets = NULL;
    PNStackLayout *pn_direct_offsets = NULL;
    PNStackLayout *pn_scaled_offsets = NULL;
    PNStackLayout *pn_p1_offsets = NULL;
    //int out_args_size;

    int max = PNRegularExpressions::MAX_MATCHES;
    regmatch_t pmatch[max];
    memset(pmatch, 0,sizeof(regmatch_t) * max);  
    //int stack_frame_size = 0;
    //int saved_regs_size = 0;

/*
    out_args_size = func->GetOutArgsRegionSize();
    assert(out_args_size >= 0);
*/

    cerr<<"OffsetInference: FindAllOffsets(): Looking at Function = "<<func->GetName()<<endl;

    ControlFlowGraph_t cfg(func);

    BasicBlock_t *block = cfg.GetEntry();

    pn_all_offsets = SetupLayout(block,func);

    //TODO: this is just for t&e, remove for other versions,
    //for t&e don't produce an inference if not dealloc is found
    bool dealloc_flag=false;

    //TODO: hacked to this location because I now need it to check for
    //deallocations of different size than the stack frame size. 
    unsigned int stack_frame_size = 0;
    if(pn_all_offsets != NULL)
    {
	stack_frame_size = pn_all_offsets->GetOriginalAllocSize();
	unsigned int saved_regs_size = pn_all_offsets->GetSavedRegsSize();
	int out_args_size = func->GetOutArgsRegionSize();
	assert(out_args_size >=0);

	pn_direct_offsets = new PNStackLayout("Direct Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,out_args_size);
	pn_scaled_offsets = new PNStackLayout("Scaled Offset Layout",func->GetName(),stack_frame_size,saved_regs_size,out_args_size);
	//do not consider out args for p1
	pn_p1_offsets = new PNStackLayout("p1 Layout",func->GetName(),stack_frame_size,saved_regs_size,0);
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

    //TODO: hack to count the number of times the stack is allocated 
    int alloc_count = 0;
    for(
	set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
	it!=func->GetInstructions().end() && pn_all_offsets != NULL;
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
	if(regexec(&(pn_regex.regex_stack_alloc), disasm_str.c_str(), max, pmatch, 0)==0)
	{
	    alloc_count++;
	}

	if(alloc_count > 1)
	{
	    cerr<<"OffsetInference: stack allocation exceeded 1, abandon inference"<<endl;
	    break;
	}

	if(regexec(&(pn_regex.regex_esp_scaled), disasm_str.c_str(), max, pmatch, 0)==0)
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

	    if(offset != stack_frame_size)
	    {
		cerr<<"OffsetInference: stack deallocation detected with different size of allocation, abandon inference"<<endl;
		dealloc_flag = false;
		break;
	    }

	}
	else if(disasm_str.find("leave") != string::npos)
	{
	    dealloc_flag = true;
	}
	else
	{
	    cerr<<"OffsetInference: FindAllOffsets: No Pattern Match"<<endl;
	}
    }

    //if no dealloc is found, set all inferences to null
    //TODO: this was hacked together quickly, one flag is preferable. 
    if(!dealloc_flag || alloc_count>1)
    {
	cerr<<"OffsetInference: FindAllOffsets: No Dealloc Pattern Found, returning null inference"<<endl;
	pn_direct_offsets = NULL;
	pn_scaled_offsets = NULL;
	pn_all_offsets = NULL;
	pn_p1_offsets = NULL;
    }

    direct[func->GetName()] = pn_direct_offsets;
    scaled[func->GetName()] = pn_scaled_offsets;
    all_offsets[func->GetName()] = pn_all_offsets;
    p1[func->GetName()] = pn_p1_offsets;
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
