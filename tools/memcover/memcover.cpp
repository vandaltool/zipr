#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <beaengine/BeaEngine.h>
#include <libIRDB-core.hpp>
#include "General_Utility.hpp"

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


void process(FileIR_t *fir_p)
{
    vector<Instruction_t*> no_addr_instrs;

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
	
	//TODO: There may be some cases I have to check for first. 

	//TODO: encode operands first, then check if both are 0, if so ignore instruction. 


/*
	if((disasm.Argument1.ArgType&0xFFFF0000) == (REGISTER_TYPE+GENERAL_REG) && (LOWORD(disasm.Argument1.ArgType)==REG4)&& disasm.Argument1.AccessMode==WRITE)||
	   ((disasm.Argument2.ArgType&0xFFFF0000) == (REGISTER_TYPE+GENERAL_REG)&& (LOWORD(disasm.Argument2.ArgType)==REG4) && disasm.Argument2.AccessMode==WRITE)
	   {
	       
	   }
*/
	if((((disasm.Argument1.ArgType&0xFFFF0000) != MEMORY_TYPE) && 
	   ((disasm.Argument2.ArgType&0xFFFF0000) != MEMORY_TYPE)) || 
	   instr_mn.compare("call")!=0 ||
	   instr_mn.compare("leave")!=0) 
	    continue;


	PREFIXINFO prefix = disasm.Prefix;
	unsigned int addr = 0;
	unsigned int func_addr = 0;
	unsigned int op1_code=0,op2_code=0;
	long long displ = 0;
	string lib_name = URLToFile(fir_p->GetFile()->GetURL());

	assert(instr->GetAddress());

	addr = instr->GetAddress()->GetVirtualOffset();

	if(addr == 0)
	{
	    cerr<<"no addr: "<<disasm.CompleteInstr<<endl;
	    //TODO: mark instruction for instrumentation and continue;
	    continue;
	}

	if(instr->GetFunction() && instr->GetFunction()->GetEntryPoint() &&
	   instr->GetFunction()->GetEntryPoint()->GetAddress())
	{
	    func_addr = instr->GetFunction()->GetEntryPoint()->GetAddress()->GetVirtualOffset();
	}

	if(instr_mn.compare("lea") == 0)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x44;
	}
	else if(instr_mn.find("push") != string::npos)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x45;
	}
	else if(instr_mn.find("pop") != string::npos)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x46;
	}
	else if(instr_mn.compare("ret") == 0)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x47;
	}
	else if(instr_mn.compare("call") == 0)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x48;
	}
	else if(instr_mn.compare("leave") == 0)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x49;
	}
	//TODO: calls.

	if(prefix.RepPrefix == InUsePrefix)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF3FFF)|(0x1<<14);
	}
	else if(prefix.RepnePrefix == InUsePrefix)
	{
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF3FFF)|(0x2<<14);
	}

	//TODO: record if pop or push, ret or call. 
	//TODO: record in instr category/encoding the numb of operands

	op1_code = encode_operand(disasm.Argument1);
	op2_code = encode_operand(disasm.Argument2);

	//nand, both args can't have displacements, if this happens I need to know why. 
	assert(!(((op1_code&0x00000001)==0x1)&&((op2_code&0x00000001)==0x1)));

	if(disasm.Argument1.Memory.Displacement != 0)
	    displ = disasm.Argument1.Memory.Displacement;
	else if(disasm.Argument2.Memory.Displacement != 0)
	    displ = disasm.Argument2.Memory.Displacement;
	else
	    displ = 0;

	annot_ofstream<<"MEM_ACCESS:"lib_name<<":"<<hex<<addr<<":"<<hex<<func_addr<<":"<<hex<<disasm.Instruction.Category<<":"<<hex<<op1_code<<":"<<hex<<op2_code<<":";
	if(displ < 0)
	{
	    displ = displ *-1;
	    annot_ofstream<<"-";
	}

	annot_ofstream<<hex<<displ<<endl;
	
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
	    File_t* this_file_p=*it;
	    assert(this_file_p);
	    cout<<"File "<<file_cnt<<endl;
	    
	    cout<<"Getting FileIR...";
	    FileIR_t *fir_p = new FileIR_t(*vid_p,this_file_p);
	    assert(fir_p);
	    cout<<"Done!"<<endl;

	    cout<<"Processing FileIR...";	    
	    process(fir_p);
	    cout<<"Done!"<<endl;
	    
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
