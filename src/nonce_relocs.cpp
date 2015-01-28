
#include <zipr_all.h>
#include <string>
#include "utils.hpp"
#include "Rewrite_Utility.hpp"


using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;

static Instruction_t* addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
        Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
        else
                newinstr = allocateNewInstruction(firp,BaseObj_t::NOT_IN_DATABASE, NULL);

        firp->RegisterAssembly(newinstr, p_asm);

        if (p_instr)
        {
                newinstr->SetFallthrough(p_instr->GetFallthrough());
                p_instr->SetFallthrough(newinstr);
        }

        return newinstr;
}



bool NonceRelocs_t::IsNonceRelocation(Relocation_t& reloc)
{
	if(strstr(reloc.GetType().c_str(),"cfi_nonce=")==NULL)
		return false;
	return true;
}


int NonceRelocs_t::GetNonceValue(Relocation_t& reloc)
{
	int value=0;
	size_t loc=reloc.GetType().find('=');

	// get the tail of the string starting a position "loc"
	string nonce_value=reloc.GetType().substr(loc+1,reloc.GetType().size()-loc-1);

//cout<<"Nonce string is "<<nonce_value<<endl;;

	return (int)strtol(nonce_value.c_str(),0,10);	
}

int NonceRelocs_t::GetNonceSize(Relocation_t& reloc)
{
	int size=-reloc.GetOffset();
	return size;
}

void NonceRelocs_t::HandleNonceRelocation(Instruction_t &insn, Relocation_t& reloc)
{
	int size=GetNonceSize(reloc);
	int value=GetNonceValue(reloc);

	assert(insn.GetIndirectBranchTargetAddress());
	RangeAddress_t addr=insn.GetIndirectBranchTargetAddress()->GetVirtualOffset()-size;

	
	if(!m_memory_space.AreBytesFree(addr,size))
	{
		cout<<"Cannot insert nonce at "<<std::hex<<addr<<" because memory is already busy"<<endl;
		slow_path_nonces.insert(&insn);
		return;
	}

	// for each byte of the nonce, plop down the values
	cout<<"Plopping nonce "<<std::hex<<value<<" at "<<std::hex<<addr<<endl;
	for(int i=0;i<size;i++)
	{ 
		cout<<"Plopping "<<std::hex<<(value&0xff)<<" at "<<std::hex<<(addr+i)<<endl;
		m_memory_space.PlopByte(addr+i,value&0xff);
		value=value>>8;
	}

	// plop bytes for nonce into exe

}

void NonceRelocs_t::AddSlowPathInstructions()
{

	Instruction_t* slow_path=NULL, *tmp=NULL;

	slow_path=tmp = addNewAssembly(&m_firp, NULL, "mov eax, 1");
	          tmp = insertAssemblyAfter(&m_firp,tmp,"int 0x80",NULL);

	for(InstructionSet_t::iterator it=slow_path_nonces.begin();
		it!=slow_path_nonces.end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		Relocation_t* reloc=FindNonceRelocation(insn);
		assert(reloc);

		string reg="ecx";
		if(m_firp.GetArchitectureBitWidth()==64)
			reg="rcx";

		string assembly="cmp "+reg+", "+to_string(insn->GetIndirectBranchTargetAddress()->GetVirtualOffset());
        	Instruction_t* after = insertAssemblyBefore(&m_firp,slow_path,assembly);
		Instruction_t* jne   = insertAssemblyAfter(&m_firp, slow_path, "je 0", insn);
	}

	for(InstructionSet_t::iterator it=m_firp.GetInstructions().begin();
		it!=m_firp.GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		Relocation_t* reloc=FindSlowpathRelocation(insn);
		if(reloc)
			insn->SetTarget(slow_path);	
	}

	m_firp.AssembleRegistry();	// resolve all assembly into actual bits.
}

Relocation_t* NonceRelocs_t::FindSlowpathRelocation(Instruction_t* insn)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit;
	for( rit=insn->GetRelocations().begin(); rit!=insn->GetRelocations().end(); ++rit)
	{
		Relocation_t& reloc=*(*rit);
		if(reloc.GetType()=="slow_cfi_path")
		{
			return &reloc;
		}
	}
	return NULL;
}

Relocation_t* NonceRelocs_t::FindNonceRelocation(Instruction_t* insn)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit;
	for( rit=insn->GetRelocations().begin(); rit!=insn->GetRelocations().end(); ++rit)
	{
		Relocation_t& reloc=*(*rit);
		if(IsNonceRelocation(reloc))
		{
			return &reloc;
		}
	}
	return NULL;
}

void NonceRelocs_t::HandleNonceRelocs()
{
	int handled=0;
	int insns=0;
	int relocs=0;
	// for each instruction 
	InstructionSet_t::iterator iit;
	for(iit=m_firp.GetInstructions().begin(); iit!=m_firp.GetInstructions().end(); ++iit)
	{
		Instruction_t& insn=*(*iit);
		insns++;

		Relocation_t* reloc=FindNonceRelocation(&insn);
		if(reloc)
		{
			HandleNonceRelocation(insn,*reloc);
			handled++;
		}
	}

	AddSlowPathInstructions();

	cout<<"#ATTRIBUTE nonce_references="<< std::dec<<handled<<endl;
	cout<<"#ATTRIBUTE instructions="<< std::dec<<insns<<endl;
	cout<<"#ATTRIBUTE slow_path_nonces="<< std::dec<<slow_path_nonces.size()<<endl;

}

