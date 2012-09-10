#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <beaengine/BeaEngine.h>
#include <libIRDB-core.hpp>
#include "General_Utility.hpp"
#include "Rewrite_Utility.hpp"
#include <sstream>

#define ARG_CNT 3
#define INPUT_ERR_NUM 1
#define DB_ERR_NUM 2

#define EAX 0x1
#define ECX 0x2
#define EDX 0x3
#define EBX 0x4
#define ESP 0x5
#define EBP 0x6
#define ESI 0x7
#define EDI 0x8
#define SCALE_ONE 0x1
#define SCALE_TWO 0x2
#define SCALE_FOUR 0x3
#define SCALE_EIGHT 0x4
#define SIZE_BYTE 0x1
#define SIZE_WORD 0x2
#define SIZE_DWORD 0x3
#define SIZE_QWORD 0x4
#define SIZE_TWORD 0x5
#define SIZE_128 0x6

#define UNDEFINED 0x0

#define LOWORD(a) ((0x0000FFFF)&(a))

using namespace std;
using namespace libIRDB;

static string prog_name;
static ofstream annot_ofstream;

static string URLToFile(string url)
{
	int loc=0;

	loc=url.find('/');
	while(loc!=string::npos)
	{
		url=url.substr(loc+1,url.length()-loc-1);

		loc=url.find('/');
	}
	// maybe need to check filename for odd characters 

	return url;
}

void usage()
{
    cerr<<"Usage: "<<prog_name<<" <DB_Variant_ID> <output annot file>"<<endl;
}

/*
  REG0 = 0x1,  EAX 1
  REG1 = 0x2,  ECX 2
  REG2 = 0x4,  EDX 3
  REG3 = 0x8,  EBX 4
  REG4 = 0x10,  ESP 5
  REG5 = 0x20,  EBP 6
  REG6 = 0x40,  ESI 7
  REG7 = 0x80,  EDI 8
  REG8 = 0x100,
  REG9 = 0x200,
  REG10 = 0x400,
  REG11 = 0x800,
  REG12 = 0x1000,
  REG13 = 0x2000,
  REG14 = 0x4000,
  REG15 = 0x8000
*/

unsigned int encode_reg(unsigned int reg)
{
    switch(reg)
    {
    case REG0:
	return EAX;
	break;
    case REG1:
	return ECX;
	break;
    case REG2:
	return EDX;
	break;
    case REG3:
	return EBX;
	break;
    case REG4:
	return ESP;
	break;
    case REG5:
	return EBP;
	break;
    case REG6:
	return ESI;
	break;
    case REG7:
	return EDI;
	break;
    case 0:
	return UNDEFINED;
	break;
    default:
		assert(false);
	break;
    }

}

unsigned int encode_scale(unsigned int scale)
{
    switch(scale)
    {
    case 1:
	return SCALE_ONE;
	break;
    case 2:
	return SCALE_TWO;
	break;
    case 4:
	return SCALE_FOUR;
	break;
    case 8:
	return SCALE_EIGHT;
	break;
    case 0:
	return UNDEFINED;
	break;
    default:
	assert(false);
	break;
    }

}

unsigned int encode_size(unsigned int size)
{
    switch(size)
    {
    case 0:
	return UNDEFINED;
	break;
    case 8:
	return SIZE_BYTE;
	break;
    case 16:
	return SIZE_WORD;
	break;
    case 32:
	return SIZE_DWORD;
	break;
    case 64:
	return SIZE_QWORD;
	break;
    case 80:
	return SIZE_TWORD;
	break;
    case 128:
	return SIZE_128;
	break;
    default:
	assert(false);
	break;
    }
}


unsigned int encode_operand(const ARGTYPE &arg)
{
    int encoding;

    if((arg.ArgType&0xFFFF0000) != MEMORY_TYPE)
    {
	return 0;
    }

    //TODO: make this optimization optional
    //for the stack, absolute accesses are not useful. 
    if(arg.Memory.BaseRegister == 0 && arg.Memory.IndexRegister == 0 && arg.Memory.Displacement != 0)
	return 0;

    encoding = encode_size(arg.ArgSize);

    //TODO: warn if size is 0
/*
    if(encoding == 0)
    {
	cerr<<lib_name<<"+"<<addr<<":"<<disasm.CompleteInstr<<" no access size provided"<<endl;
    }
*/
    encoding = encoding <<4;
    encoding |= arg.AccessMode;
    encoding <<= 4;
    encoding |= encode_reg(arg.Memory.BaseRegister);
    encoding <<= 4;
    encoding |= encode_reg(arg.Memory.IndexRegister);
    encoding <<= 2;
    encoding |= encode_scale(arg.Memory.Scale);
    encoding <<= 2;
    if(arg.Memory.Displacement != 0)
	encoding |= 0x1; 
	    
    return encoding;
}

inline bool is_esp_dest(ARGTYPE &arg)
{
    return ((arg.ArgType&0xFFFF0000) == (REGISTER_TYPE+GENERAL_REG) && (LOWORD(arg.ArgType)==REG4)&& arg.AccessMode==WRITE);
}

/*

struct mem_ref_encoding
{
    unsigned int instr_code;
    unsigned int op1_code;
    unsigned int op2_code;
    long long displ;
};

bool encode_stack_alt(DISASM &disasm,stack_alt_encoding &out_encoding)
{


    ARGTYPE mod_src;

    if(is_esp_dest(disasm.Argument1))
	mod_src = disasm.Argument2;
    else if(is_esp_dest(disasm.Argument2))
	mod_src = disasm.Argument1;
    else if(disasm.Instruction.ImplicitModifiedRegs == REGISTER_TYPE+GENERAL_REG+REG4)
	{
	    //cout<<"Implicit esp: "<<disasm.CompleteInstr<<endl;
	}
	else
		return false;

    memset(&out_encoding,0,sizeof(stack_alt_encoding));

    if((mod_src.ArgType&0xFFFF0000) == (REGISTER_TYPE+GENERAL_REG))
    {
	unsigned int reg_code = encode_reg(LOWORD(mod_src.ArgType));

	//TODO: encode reg. 
    }
    else if(mod_src.ArgType == MEMORY_TYPE)
    {
	unsigned int base_code = encode_reg(mod_src.Memory.BaseRegister);
	unsigned int index_code = encode_reg(mod_src.Memory.IndexRegister);
	unsigned int scale_code = encode_scale(mod_src.Memory.Scale);
	out_encoding.constant = mod_src.Memory.Displacement;
    
	//TODO: encode it all.
    }
    else
    {
	out_encoding.constant = disasm.Instruction.Immediat;
    }

    string instr_mn = disasm.Instruction.Mnemonic;
    trim(instr_mn);

	//cout<<disasm.CompleteInstr<<endl;

    if(instr_mn.compare("and")==0)
    {
	//todo: encode and
    }
    else if(instr_mn.compare("add")==0)
    {
	//todo: encode add
    }
    else if(instr_mn.compare("sub")==0)
    {
	//todo: encode sub
    }
    else if(instr_mn.compare("leave")==0)
    {
	//todo: encode leave
    }
    else if(instr_mn.compare("mov")==0)
    {
	//todo: encode mov
    }
    else if(instr_mn.compare("lea")==0)
    {
	//todo: encode lea
    }
    else if(instr_mn.compare("inc")==0)
    {
	//todo: encode inc
    }
	else if(instr_mn.compare("dec")==0)
	{
		//todo: encode dec
	}
    else
    {
	//I want to know when this happens so I can add a new encoding
	//assert(false);
    }

    return true;
}
*/

static int counter = -16;

virtual_offset_t get_next_addr()
{
	counter += 16;
	return 0xf0000000 + counter;
}

void process_instructions(FileIR_t *fir_p)
{
	//using maps since I believe inserting instructions while iterating
	//will cause issues. 
	map<Instruction_t*,DISASM> post_esp_checks;
	map<Instruction_t*,DISASM> ret_esp_checks;
	map<Instruction_t*,DISASM> mem_refs;
	map<Instruction_t*,DISASM> func_entries;

    for(
	set<Instruction_t*>::const_iterator it=fir_p->GetInstructions().begin();
	it!=fir_p->GetInstructions().end();
	++it
	)
    {
		Instruction_t* instr = *it;
		assert(instr);

		DISASM disasm;
		instr->Disassemble(disasm); //calls memset for me, no worries
		string instr_mn = disasm.Instruction.Mnemonic;
		trim(instr_mn);

		//is esp a destination, or is esp implicitly modified?
		if(is_esp_dest(disasm.Argument1) || is_esp_dest(disasm.Argument2) ||
		   disasm.Instruction.ImplicitModifiedRegs == REGISTER_TYPE+GENERAL_REG+REG4)
		{
			assert(instr_mn.compare("ret") != 0);
			post_esp_checks[instr] = disasm;
		}

		if(instr_mn.compare("ret")==0)
			ret_esp_checks[instr] = disasm;

		if(((disasm.Argument1.ArgType&0xFFFF0000) == MEMORY_TYPE) ||
		   ((disasm.Argument2.ArgType&0xFFFF0000) == MEMORY_TYPE))
			mem_refs[instr] = disasm;

		//TODO: missing calls
	}

	for(
		set<Function_t*>::const_iterator it=fir_p->GetFunctions().begin();
		it!=fir_p->GetFunctions().end();
		++it
		)
	{
		Function_t *func = *it;

		/*
		ControlFlowGraph_t cfg(func);
		BasicBlock_t *block = cfg.GetEntry();
		*/

		//TODO: I am not sure if this is as reliable as using the control flow graph. 
		Instruction_t *first_instr = func->GetEntryPoint();

		DISASM disasm;
		first_instr->Disassemble(disasm); //calls memset for me, no worries

		func_entries[first_instr] = disasm;
	}






	for(
		map<Instruction_t*,DISASM>::const_iterator it=post_esp_checks.begin();
		it!=post_esp_checks.end();
		++it
		)
	{
/*
		//Execute the instruction then call the post_esp_check callback
		//The esp is checked after execution to avoid calculating esp
		//through shadow execution. Will not work for ret instructions. 
		Instruction_t *instr = it->first;
		Instruction_t *tmp;

		stringstream ss;
		ss.str("");
		unsigned int ra = get_next_addr();
		ss<<hex<<ra;

		tmp = insertAssemblyAfter(fir_p,instr,"pushad");
		tmp = insertAssemblyAfter(fir_p,tmp,"pushfd");
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		tmp = insertAssemblyAfter(fir_p,tmp,"nop");
		tmp->SetCallback("post_esp_check");
		tmp = insertAssemblyAfter(fir_p,tmp,"popfd");
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());
		tmp = insertAssemblyAfter(fir_p,tmp,"popad");

*/
		Instruction_t *instr = it->first;
		Instruction_t *orig_ft = instr;
		Instruction_t *tmp;

		unsigned int addr = instr->GetAddress()->GetVirtualOffset();
		stringstream ss;
		ss.str("");
		unsigned int ra = get_next_addr();
		

		tmp = insertAssemblyAfter(fir_p,instr,"pushad");

		tmp = insertAssemblyAfter(fir_p,tmp,"pushfd");
		ss<<hex<<addr;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");
		ss<<hex<<ra;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		tmp = insertAssemblyAfter(fir_p,tmp,"nop");
		tmp->SetCallback("post_esp_check");
		tmp = insertAssemblyAfter(fir_p,tmp,"pop eax");
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());
		tmp = insertAssemblyAfter(fir_p,tmp,"popfd");
		tmp = insertAssemblyAfter(fir_p,tmp,"popad");

	}

	for(
		map<Instruction_t*,DISASM>::const_iterator it=ret_esp_checks.begin();
		it!=ret_esp_checks.end();
		++it
		)
	{
		Instruction_t *instr = it->first;
		Instruction_t *orig_ft = instr;
		Instruction_t *tmp;

		unsigned int addr = instr->GetAddress()->GetVirtualOffset();
		stringstream ss;
		ss.str("");
		unsigned int ra = get_next_addr();
		

		insertAssemblyBefore(fir_p,instr,"pushad");

		tmp = insertAssemblyAfter(fir_p,instr,"pushfd");
		ss<<hex<<addr;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");
		ss<<hex<<ra;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		tmp = insertAssemblyAfter(fir_p,tmp,"nop");
		tmp->SetCallback("ret_esp_check");
		tmp = insertAssemblyAfter(fir_p,tmp,"pop eax");
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());
		tmp = insertAssemblyAfter(fir_p,tmp,"popfd");
		tmp = insertAssemblyAfter(fir_p,tmp,"popad");

/*
		//because rets redirect execution, a post_esp_check callback cannot be made
		//here a special ret callback is used as a solution, checked prior to execution
		//of the original instruction. No need for shadow execution as ret has a consistant behavior.


		


		//Note the use of insertAssemblyBefore, read in reverse order. 
		insertAssemblyBefore(fir_p,instr,"popad");
		insertAssemblyBefore(fir_p,instr,"popfd");
		tmp = insertAssemblyBefore(fir_p,instr,"nop");
		//TOOD: okay this is ugly, but I have to set data after it no longer
		//points to the original instruction.
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());
		tmp = insertAssemblyBefore(fir_p,instr,"push 0x"+ss.str());
		//TOOD: okay this is ugly, but I have to set the callback after
		//the instruction no longer points to the original.
		//tmp->SetCallback("ret_esp_check");
		tmp->SetCallback("post_esp_check");
		insertAssemblyBefore(fir_p,instr,"pushfd");
		insertAssemblyBefore(fir_p,instr,"pushad");
*/
	}

/*

	for(
		map<Instruction_t*,DISASM>::const_iterator it=func_entries.begin();
		it!=func_entries.end();
		++it
		)
	{

		Instruction_t *instr = it->first;
		Instruction_t *tmp;
		Instruction_t *ft = instr->GetFallthrough();

		stringstream ss;
		ss.str("");
		unsigned int ra = get_next_addr();
		unsigned int func_addr = instr->GetAddress()->GetVirtualOffset();//instr->GetFunction()->GetEntryPoint()->GetAddress()->GetVirtualOffset();

		insertAssemblyBefore(fir_p,instr,"pushad");

		tmp = insertAssemblyAfter(fir_p,instr,"pushfd");
		ss<<hex<<func_addr;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");
		ss<<hex<<ra;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");
		tmp = insertAssemblyAfter(fir_p,tmp,"nop");
		tmp->SetCallback("func_entry");
		tmp = insertAssemblyAfter(fir_p,tmp,"pop eax");
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());
		tmp = insertAssemblyAfter(fir_p,tmp,"popfd");
		tmp = insertAssemblyAfter(fir_p,tmp,"popad");

	}

*/
	for(
		map<Instruction_t*,DISASM>::const_iterator it=mem_refs.begin();
		it!=mem_refs.end();
		++it
		)
	{
		Instruction_t *instr = it->first;
		Instruction_t *tmp;
		DISASM disasm = it->second;


		PREFIXINFO prefix = disasm.Prefix;
		unsigned int addr = 0;
		unsigned int func_addr = 0;
		unsigned int op1_code=0,op2_code=0;
		unsigned int displ = 0;
		string instr_mn = disasm.Instruction.Mnemonic;
		trim(instr_mn);
		string lib_name = URLToFile(fir_p->GetFile()->GetURL());

		assert(instr->GetAddress());
		addr = instr->GetAddress()->GetVirtualOffset();

		//I am not sure if any of these situations exist, but I want to avoid yucky segfaults
		if(instr->GetFunction() && instr->GetFunction()->GetEntryPoint() &&
		   instr->GetFunction()->GetEntryPoint()->GetAddress())
		{
			//TODO: this might not work for dyn libs
			func_addr = instr->GetFunction()->GetEntryPoint()->GetAddress()->GetVirtualOffset();
		}

		op1_code = encode_operand(disasm.Argument1);
		op2_code = encode_operand(disasm.Argument2);

		assert(!(((op1_code&0x00000001)==0x1)&&((op2_code&0x00000001)==0x1)));
 
		//TODO: I am type casting to unsigned int, will this cause problems?
		if(disasm.Argument1.Memory.Displacement != 0)
			displ = (unsigned int)disasm.Argument1.Memory.Displacement;
		else if(disasm.Argument2.Memory.Displacement != 0)
			displ = (unsigned int)disasm.Argument2.Memory.Displacement;
		else
			displ = 0;

		if(instr_mn.compare("lea") == 0)
		{
			disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x44;
		}

		if(prefix.RepPrefix == InUsePrefix)
		{
			disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF3FFF)|(0x1<<14);
		}
		else if(prefix.RepnePrefix == InUsePrefix)
		{
			disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF3FFF)|(0x2<<14);
		}

		//TODO: if rep, should I have a post rep check?


		stringstream ss;
		ss.str("");
		//TODO: addr can equal 0 make sure its handled.
		
//Note the use of insertAssemblyBefore, read in reverse order. 
		insertAssemblyBefore(fir_p,instr,"pushad");

		tmp = insertAssemblyAfter(fir_p,instr,"pushfd");

		ss<<hex<<displ;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		ss<<hex<<op2_code;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		ss<<hex<<op1_code;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		ss<<hex<<disasm.Instruction.Category;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		ss<<hex<<func_addr;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		ss<<hex<<addr;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		unsigned int ra = get_next_addr();
		ss<<hex<<ra;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");

		tmp = insertAssemblyAfter(fir_p,tmp,"nop");
		tmp->SetCallback("mem_ref");

		tmp = insertAssemblyAfter(fir_p,tmp,"add esp, 0x18");
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());

		tmp = insertAssemblyAfter(fir_p,tmp,"popfd");
		tmp = insertAssemblyAfter(fir_p,tmp,"popad");

	}



	//TODO: there is no reason to loop through this again.

	//NOTE: this must be done after all other instrumentation (since it inserts before). 
	for(
		map<Instruction_t*,DISASM>::const_iterator it=func_entries.begin();
		it!=func_entries.end();
		++it
		)
	{

		Instruction_t *instr = it->first;
		Instruction_t *tmp;
		Instruction_t *ft = instr->GetFallthrough();

		stringstream ss;
		ss.str("");
		unsigned int ra = get_next_addr();
		unsigned int func_addr = instr->GetAddress()->GetVirtualOffset();//instr->GetFunction()->GetEntryPoint()->GetAddress()->GetVirtualOffset();

		insertAssemblyBefore(fir_p,instr,"pushad");

		tmp = insertAssemblyAfter(fir_p,instr,"pushfd");
		ss<<hex<<func_addr;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");
		ss<<hex<<ra;
		tmp = insertAssemblyAfter(fir_p,tmp,"push 0x"+ss.str());
		ss.str("");
		tmp = insertAssemblyAfter(fir_p,tmp,"nop");
		tmp->SetCallback("func_entry");
		tmp = insertAssemblyAfter(fir_p,tmp,"pop eax");
		tmp->GetAddress()->SetVirtualOffset(ra);
		tmp->SetIndirectBranchTargetAddress(tmp->GetAddress());
		tmp = insertAssemblyAfter(fir_p,tmp,"popfd");
		tmp = insertAssemblyAfter(fir_p,tmp,"popad");

	}


}


int main(int argc, char **argv)
{
    prog_name = argv[0];

    if(argc != ARG_CNT)
    {
	usage();
	exit(INPUT_ERR_NUM);
    }

    int vid;
    if(str2int(vid,argv[1]) != SUCCESS)
    {
	cerr<<"Variant ID ("<<argv[1]<<") could not be parsed as an integer."<<endl;
	exit(INPUT_ERR_NUM);
    }
    

    annot_ofstream.open(argv[2]);
    
    if(!annot_ofstream.is_open())
    {
	cerr<<"Could not open file "<<argv[2]<<" for writing"<<endl;
	exit(INPUT_ERR_NUM);
    }

 
    pqxxDB_t pqxx_interface;
    BaseObj_t::SetInterface(&pqxx_interface);

    VariantID_t *vid_p;

    try
    {
	cout<<"Getting VariantID...";
	vid_p = new VariantID_t(vid);
	assert(vid_p && vid_p->IsRegistered());
	cout<<"Done!"<<endl;

	cout<<vid_p->GetFiles().size()<<" Files to Analyze"<<endl;
	int file_cnt =1;
	//Get each file
	for(set<File_t*>::const_iterator it=vid_p->GetFiles().begin();
	    it != vid_p->GetFiles().end();
	    ++it,++file_cnt
	    )
	{
		stringstream ss;
		ss.str("");
	    File_t* this_file_p=*it;
	    assert(this_file_p);
	    cout<<"File "<<file_cnt<<endl;
	    
	    cout<<"Getting FileIR...";
	    FileIR_t *fir_p = new FileIR_t(*vid_p,this_file_p);
	    assert(fir_p);
	    cout<<"Done!"<<endl;

	    cout<<"Processing FileIR...";	    
	    process_instructions(fir_p);
	    cout<<"Done!"<<endl;
		ss<<file_cnt;
		//annot_ofstream.open(string("annot_test"+ss.str()).c_str());
		fir_p->GenerateSPRI(annot_ofstream);
	    //annot_ofstream.close();

		delete fir_p;
	}

	annot_ofstream.close();
	
	pqxx_interface.Commit();
    }
    catch(DatabaseError_t dberr)
    {
	cerr<<"Unexpected database error: "<<dberr<<endl;
	exit(DB_ERR_NUM);
    }

    

    return 0;
}
