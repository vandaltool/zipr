
#include <zipr_all.h>
#include <string>
#include <algorithm>
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

	InstructionSet_t::iterator it;
        for( it=m_firp.GetInstructions().begin(); it!=m_firp.GetInstructions().end(); ++it)
        {
                Instruction_t* insn=*it;
                Relocation_t* reloc=FindSlowpathRelocation(insn);
                if(reloc)
                        break;
        }
	// exited loop normally, or hit the break statement?
	if(it==m_firp.GetInstructions().end())
	{
		cout<<"Found no slow paths to link, skipping slow-path code."<<endl;
		return;
	}
	

// optimization
// find out if slow path insns are even needed!

	Instruction_t* slow_path=NULL, *exit_node=NULL, *tmp=NULL;
	string reg="ecx";
	if(m_firp.GetArchitectureBitWidth()==64)
		reg="rcx";

	// call exit.
	exit_node=
	slow_path=tmp = addNewAssembly(&m_firp, NULL, "mov eax, 1");
	          tmp = insertAssemblyAfter(&m_firp,tmp,"int 0x80",NULL);

	for(it=slow_path_nonces.begin(); it!=slow_path_nonces.end(); ++it)
	{
		Instruction_t* insn=*it;
		Relocation_t* reloc=FindNonceRelocation(insn);
		assert(reloc);


		string assembly="cmp "+reg+", "+to_string(insn->GetIndirectBranchTargetAddress()->GetVirtualOffset());
		// insert before acts weird, and really does enough bookkeeping to insert-after in a way to mimic insert before.
        	Instruction_t* after = insertAssemblyBefore(&m_firp,slow_path,assembly);
		Instruction_t* jne   = insertAssemblyAfter(&m_firp, slow_path, "je 0", insn);
		exit_node=after;
	}

#ifdef CGC
	// CGC  needs to keep from faulting, so we have to check to make sure the IB is an intra-module IB.
	// if not, we terminate immediately.
	// so, we emit this sequence before we try the slow path:
	// 	pop rcx (get ret addr)
	//	cmp rcx, start_segement_addr
	//	jlt terminate
	//	cmp rcx, end_segment_addr
	//	jgt terminate
	// After we're sure it's in this segment, we can 
	// go ahead and check for a nonce that we layed down previously.
	// 	cmp byte [rcx-1], 0xf4
	// 	jeq slow_path

	Instruction_t* after = insertAssemblyBefore(&m_firp,slow_path,"pop "+reg);
	tmp = insertAssemblyAfter(&m_firp,slow_path,"cmp "+reg+", 0x12345678");
	min_addr_update.insert(tmp);
        tmp = insertAssemblyAfter(&m_firp,tmp,"jl 0",exit_node);        // terminate
	tmp = insertAssemblyAfter(&m_firp,tmp,"cmp "+reg+", 0x87654321");
	max_addr_update.insert(tmp);
        tmp = insertAssemblyAfter(&m_firp,tmp,"jg 0",exit_node);        // terminate
        tmp = insertAssemblyAfter(&m_firp,tmp,"cmp byte ["+reg+"-1], 0xf4");
        tmp = insertAssemblyAfter(&m_firp,tmp,"jne 0",after);       // finally, go to the slow path checks when a nonce didn't work.
        tmp = insertAssemblyAfter(&m_firp,tmp,"jmp "+reg);
#endif // CGC

	for( it=m_firp.GetInstructions().begin(); it!=m_firp.GetInstructions().end(); ++it)
	{
		Instruction_t* insn=*it;
		Relocation_t* reloc=FindSlowpathRelocation(insn);
		if(reloc)
			insn->SetTarget(slow_path);	
	}

	m_firp.SetBaseIDS();		// assign a unique ID to each insn.
	m_firp.AssembleRegistry();	// resolve all assembly into actual bits.
}

Relocation_t* NonceRelocs_t::FindRelocation(Instruction_t* insn, string type)
{
	Instruction_t* first_slow_path_insn=NULL;
	RelocationSet_t::iterator rit;
	for( rit=insn->GetRelocations().begin(); rit!=insn->GetRelocations().end(); ++rit)
	{
		Relocation_t& reloc=*(*rit);
		if(reloc.GetType()==type)
		{
			return &reloc;
		}
	}
	return NULL;
}

Relocation_t* NonceRelocs_t::FindSlowpathRelocation(Instruction_t* insn)
{
	return FindRelocation(insn,"slow_cfi_path");
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

			assert(insn.GetIndirectBranchTargetAddress());
			
		}
	}

	AddSlowPathInstructions();

	cout<<"#ATTRIBUTE nonce_references="<< std::dec<<handled<<endl;
	cout<<"#ATTRIBUTE instructions="<< std::dec<<insns<<endl;
	cout<<"#ATTRIBUTE slow_path_nonces="<< std::dec<<slow_path_nonces.size()<<endl;

}


void NonceRelocs_t::UpdateAddrRanges(std::map<libIRDB::Instruction_t*,RangeAddress_t> &final_insn_locations)
{
	RangeAddress_t  min_addr=m_memory_space.GetMinPlopped();
	RangeAddress_t  max_addr=m_memory_space.GetMaxPlopped();

	InstructionSet_t::iterator it;
	for(it=min_addr_update.begin(); it!=min_addr_update.end(); ++it)
	{
		Instruction_t& insn=*(*it);
		RangeAddress_t insn_addr=final_insn_locations[&insn];
		if(insn_addr)
		{
			cout<<"Updating min_addr at "<<hex<<insn_addr<<" to compare to "<<min_addr<<endl;
			m_memory_space.PlopBytes(insn_addr+2,(const char*)&min_addr,sizeof(RangeAddress_t));
		}
		else
		{
			cout<<"No addr for  min_addr at "<<hex<<insn_addr<<" to compare to "<<min_addr<<endl;
		}
	}

	for(it=max_addr_update.begin(); it!=max_addr_update.end(); ++it)
	{
		Instruction_t& insn=*(*it);
		RangeAddress_t insn_addr=final_insn_locations[&insn];
		assert(insn_addr);
		if(insn_addr)
		{
			cout<<"Updating max_addr at "<<hex<<insn_addr<<" to compare to "<<max_addr<<endl;
			m_memory_space.PlopBytes(insn_addr+2,(const char*)&max_addr,sizeof(RangeAddress_t));
		}
		else
		{
			cout<<"No addr for  max_addr at "<<hex<<insn_addr<<" to compare to "<<max_addr<<endl;
		}
	}


	
}
