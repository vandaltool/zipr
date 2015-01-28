
#include <zipr_all.h>
#include <string>

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;


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

		// for each relocation on this instruction
		RelocationSet_t::iterator rit;
		for( rit=insn.GetRelocations().begin(); rit!=insn.GetRelocations().end(); ++rit)
		{
			relocs++;
			Relocation_t& reloc=*(*rit);
			if(IsNonceRelocation(reloc))
			{
				HandleNonceRelocation(insn,reloc);
				handled++;
			}
		}
	}
	cout<<"#ATTRIBUTE nonce_references="<< std::dec<<handled<<endl;
	cout<<"#ATTRIBUTE relocations="<< std::dec<<handled<<endl;
	cout<<"#ATTRIBUTE instructions="<< std::dec<<handled<<endl;
	cout<<"#ATTRIBUTE slow_path_nonces="<< std::dec<<slow_path_nonces.size()<<endl;

}

