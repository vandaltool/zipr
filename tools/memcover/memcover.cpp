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
	return 0x1;
	break;
    case REG1:
	return 0x2;
	break;
    case REG2:
	return 0x3;
	break;
    case REG3:
	return 0x4;
	break;
    case REG4:
	return 0x5;
	break;
    case REG5:
	return 0x6;
	break;
    case REG6:
	return 0x7;
	break;
    case REG7:
	return 0x8;
	break;
    case 0:
	return 0x0;
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
	return 0x1;
	break;
    case 2:
	return 0x2;
	break;
    case 4:
	return 0x3;
	break;
    case 8:
	return 0x4;
	break;
    case 0:
	return 0x0;
	break;
    default:
	assert(false);
	break;
    }

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
	unsigned int addr = 0;
	unsigned int func_addr = 0;
	unsigned int op1_code=0,op2_code=0;
	long long displ = 0;
	
	
	//TODO: There may be some cases I have to check for first. 

	if(((disasm.Argument1.ArgType&0xFFFF0000) != MEMORY_TYPE) && 
	   ((disasm.Argument2.ArgType&0xFFFF0000) != MEMORY_TYPE))
	    continue;

	if(instr->GetAddress())
	{
	    addr = instr->GetAddress()->GetVirtualOffset();
	}
	else
	{
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
	    //TODO: set instr category to reflect lea
	    disasm.Instruction.Category = (disasm.Instruction.Category&0xFFFF0000)|0x44;
	}

	//TODO: record if pop or push, ret or call. 
	//TODO: record in instr category/encoding the numb of operands

	if((disasm.Argument1.ArgType&0xFFFF0000) == MEMORY_TYPE)
	{
	    op1_code = disasm.Argument1.AccessMode;
	    op1_code = op1_code <<4;
	    op1_code = op1_code | encode_reg(disasm.Argument1.Memory.BaseRegister);
	    op1_code = op1_code <<4;
	    op1_code = op1_code | encode_reg(disasm.Argument1.Memory.IndexRegister);
	    op1_code = op1_code <<2;
	    op1_code = op1_code | encode_scale(disasm.Argument1.Memory.Scale);
	    op1_code = op1_code <<2;
	    if(disasm.Argument1.Memory.Displacement != 0)
	    {
		op1_code = op1_code | 0x1;
		displ = disasm.Argument1.Memory.Displacement;
	    }
	    
	}

	if((disasm.Argument2.ArgType&0xFFFF0000) == MEMORY_TYPE)
	{
	    op2_code = op1_code | disasm.Argument2.AccessMode;
	    op2_code = op2_code <<4;
	    op2_code = op2_code | encode_reg(disasm.Argument2.Memory.BaseRegister);
	    op2_code = op2_code <<4;
	    op2_code = op2_code | encode_reg(disasm.Argument2.Memory.IndexRegister);
	    op2_code = op2_code <<2;
	    op2_code = op2_code | encode_scale(disasm.Argument2.Memory.Scale);
	    op2_code = op2_code <<2;
	    if(disasm.Argument2.Memory.Displacement != 0)
	    {
		assert(displ == 0); //only one displ allowed
		op2_code = op2_code | 0x1;

		displ = disasm.Argument2.Memory.Displacement;
	    }
	}

	annot_ofstream<<URLToFile(fir_p->GetFile()->GetURL())<<":"<<hex<<addr<<":"<<hex<<func_addr<<":"<<hex<<disasm.Instruction.Category<<":"<<hex<<op1_code<<":"<<hex<<op2_code<<":";
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
