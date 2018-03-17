
#include <zipr_all.h>
#include <libIRDB-core.hpp>
#include <Rewrite_Utility.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <fstream>
#include <elf.h>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "targ-config.h"
//#include "beaengine/BeaEngine.h"

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;

template < typename T > std::string to_hex_string( const T& n )
{
        std::ostringstream stm ;
        stm << std::hex<< "0x"<< n ;
        return stm.str() ;
}

template<int ptrsize>
void EhWriterImpl_t<ptrsize>::GenerateNewEhInfo()
{
	
	BuildFDEs();
	GenerateEhOutput();
	CompileEhOutput();
	ScoopifyEhOutput();

}

template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::CIErepresentation_t::canSupport(Instruction_t* insn) const
{
	// if the insn is missing info, we can support it.
	if(insn==NULL)
		return true;
	if(insn->GetEhProgram()==NULL)
		return true;

	// all info here.


	// check that the program,CAF, DAF and RR match.  if not, can't support
	if(insn->GetEhProgram()->GetCIEProgram() != pgm || 
	   insn->GetEhProgram()->GetCodeAlignmentFactor() != code_alignment_factor || 
	   insn->GetEhProgram()->GetDataAlignmentFactor() != data_alignment_factor || 
	   insn->GetEhProgram()->GetReturnRegNumber() != (int64_t)return_reg 
	  )
		return false;

	auto personality_it=find_if(
		insn->GetEhProgram()->GetRelocations().begin(), 
		insn->GetEhProgram()->GetRelocations().end(),
		[](const Relocation_t* r) { return r->GetType()=="personality"; });

	// lastly, check for a compatible personality reloc.
	// if incoming has no personality, we better have no personality to match.
	if(personality_it==insn->GetEhProgram()->GetRelocations().end())
	{
		return personality_reloc==NULL;
	}

	// incomming has personality, but do we?
	if(personality_reloc==NULL) return false;

	// compare personalities.
	if(personality_reloc->GetWRT() != (*personality_it)->GetWRT()) return false;
	if(personality_reloc->GetAddend() != (*personality_it)->GetAddend()) return false;

	return true;
}

template <int ptrsize>
EhWriterImpl_t<ptrsize>::CIErepresentation_t::CIErepresentation_t(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
	: has_been_output(false)
{
	assert(insn && ehw && insn->GetEhProgram());

	pgm = insn->GetEhProgram()->GetCIEProgram();
	code_alignment_factor = insn->GetEhProgram()->GetCodeAlignmentFactor();
	data_alignment_factor = insn->GetEhProgram()->GetDataAlignmentFactor();
	return_reg = insn->GetEhProgram()->GetReturnRegNumber();

	auto personality_it=find_if(
		insn->GetEhProgram()->GetRelocations().begin(), 
		insn->GetEhProgram()->GetRelocations().end(),
		[](const Relocation_t* r) { return r->GetType()=="personality";});

	personality_reloc = (personality_it==insn->GetEhProgram()->GetRelocations().end())
		? (Relocation_t*)NULL
		: *personality_it;
}


template <int ptrsize>
void EhWriterImpl_t<ptrsize>::print_pers(Instruction_t* insn, EhWriterImpl_t<ptrsize>::CIErepresentation_t *cie)
{
	const auto pretty_print= [&](Relocation_t* pr)
		{
			if(pr==NULL)
			{
				cout<<"Found no personality reloc"<<endl;
				return;
			}
			const auto personality_scoop=dynamic_cast<DataScoop_t*>(pr->GetWRT());
			const auto personality_insn=dynamic_cast<Instruction_t*>(pr->GetWRT());

			if(pr->GetWRT()==NULL)
				cout<<"\tFound null personality"<<endl;
			else if(personality_scoop)
				cout<<"\tFound personlity scoop "<<personality_scoop->GetName()<<"+0x"<<hex<<pr->GetAddend()<<endl;
			else if(personality_insn)
				cout<<"\tFound personlity instruction "<<hex<<personality_insn->GetBaseID()<<dec<<":"<<hex<<personality_insn->getDisassembly()<<endl;
			else
				cout<<"\tFound reloc: unexpected type? "<<endl;
		};

	cout<<"  CIE-Personality addr= "<<hex<<cie->personality_reloc<<dec<<endl;
	pretty_print(cie->GetPersonalityReloc());
	const auto personality_it=find_if(
		insn->GetEhProgram()->GetRelocations().begin(), 
		insn->GetEhProgram()->GetRelocations().end(),
		[](const Relocation_t* r) { return r->GetType()=="personality"; });

	const auto pr = (personality_it==insn->GetEhProgram()->GetRelocations().end())
		? (Relocation_t*)NULL
		: *personality_it;
	cout<<"  insn personality addr= "<<hex<<pr<<dec<<endl;
	pretty_print(pr);


};


template <int ptrsize>
EhWriterImpl_t<ptrsize>::FDErepresentation_t::FDErepresentation_t(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
	: 
		lsda(insn),
		cie(NULL)
{
	auto cie_it=find_if( ehw->all_cies.begin(), ehw->all_cies.end(), [&](const CIErepresentation_t* candidate)
			{
				return candidate->canSupport(insn);
			});

	if(cie_it==ehw->all_cies.end())
	{
		cie=new CIErepresentation_t(insn,ehw);
		ehw->all_cies.push_back(cie);

		if(getenv("EH_VERBOSE")!=NULL)
			cout<<"Creating new CIE representation"<<endl;
	}
	else
	{
		cie=*cie_it;
		if(getenv("EH_VERBOSE")!=NULL)
		{
			cout<<"Re-using CIE representation"<<endl;
			print_pers(insn, cie);
		}
	}

	start_addr=ehw->zipr_obj.GetLocationMap()->at(insn);
	last_advance_addr=start_addr;
	end_addr=start_addr+insn->GetDataBits().size();
	pgm=EhProgramListingManip_t(insn->GetEhProgram()->GetFDEProgram());
}


template <int ptrsize>
int EhWriterImpl_t<ptrsize>::EhProgramListingManip_t::getMergeIndex(const EhProgramListingManip_t &other)
{
	auto other_index=(size_t)0;
	for(const auto &this_ele : *this)
	{
		if(isAdvanceDirective(this_ele))
			continue;

		if(other_index >= other.size())
			return -1;

		if( this_ele!=other.at(other_index))
			return -1;
		other_index++;
	}

	return other_index;
}
template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::EhProgramListingManip_t::canExtend(const EhProgramListingManip_t &other)
{
	return getMergeIndex(other) != -1;
}

template <int ptrsize>
void EhWriterImpl_t<ptrsize>::EhProgramListingManip_t::extend(const uint64_t inc_amt, const EhProgramListingManip_t &other)
{
	if(size() > 0 && isAdvanceDirective(at(size()-1)))
	{
		auto &last_insn=at(size()-1);
		auto old_inc_amt=(int)0;
		auto old_size=(int)0;
		switch(last_insn[0])
		{
			case DW_CFA_advance_loc1: old_size=1; break;
			case DW_CFA_advance_loc2: old_size=2; break;
			case DW_CFA_advance_loc4: old_size=4; break;
		}
		for(auto i=0;i<old_size;i++)
			((char*)(&old_inc_amt))[i]=last_insn[i+1];

		auto new_inc_amt=old_inc_amt+inc_amt;
		auto opcode=(char)0;
		auto new_size=(int)0;
		if(new_inc_amt<=255)
		{
			new_size=1;
			opcode=DW_CFA_advance_loc1;
		}
		else if(new_inc_amt<=65535)
		{
			new_size=2;
			opcode=DW_CFA_advance_loc2;
		}
		else
		{
			new_size=4;
			opcode=DW_CFA_advance_loc4;
		}
		last_insn.resize(1+new_size);
		last_insn[0]=opcode;
		for(auto i=0;i<new_size;i++)
			last_insn[i+1]=((const char*)(&new_inc_amt))[i];
	}
	else
	{	
		auto new_size=(int)0;
		auto opcode=(char)0;
		if(inc_amt<=255)
		{
			new_size=1;
			opcode=DW_CFA_advance_loc1;
		}
		else if(inc_amt<=65535)
		{
			new_size=2;
			opcode=DW_CFA_advance_loc2;
		}
		else
		{
			new_size=4;
			opcode=DW_CFA_advance_loc4;
		}
		// make an advance directive
		string ehinsn;
		ehinsn.resize(1+new_size);
		ehinsn[0]=opcode;
		for(auto i=0;i<new_size;i++)
			ehinsn[i+1]=((const char*)(&inc_amt))[i];

		// add it with this->push_back()
		this->push_back(ehinsn);
	}

	// add elements from other[merge_index]-other[other.size()]
	auto merge_index=getMergeIndex(other);
	assert(merge_index>=0);
	for_each(other.begin()+merge_index,other.end(), [&](const string& s)
	{
		this->push_back(s);
	});

	
}

template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::EhProgramListingManip_t::isAdvanceDirective(const string &s) const
{
	// make sure uint8_t is an unsigned char.       
	static_assert(std::is_same<unsigned char, uint8_t>::value, "uint8_t is not unsigned char");

	auto data=s.data();
	auto opcode=data[0];
	auto opcode_upper2=(uint8_t)(opcode >> 6);
	auto opcode_lower6=(uint8_t)(opcode & (0x3f));

	switch(opcode_upper2)
	{
		case 1:
		{
			return true;
		}
		case 0:
		{
			switch(opcode_lower6)
			{
				case DW_CFA_advance_loc1:
				case DW_CFA_advance_loc2:
				case DW_CFA_advance_loc4:
					return true;

			}
		}
	}
	return false;

}

template <int ptrsize>
EhWriterImpl_t<ptrsize>::FDErepresentation_t::LSDArepresentation_t::LSDArepresentation_t(Instruction_t* insn)
	// if there are call sites, use the call site encoding.  if not, set to omit for initializer.
	//  extend/canExtend should be able to extend an omit to a non-omit.
	: tt_encoding( insn->GetEhCallSite() ? insn->GetEhCallSite()->GetTTEncoding() : 0xff)
{
		
	extend(insn);
}



static const auto RelocsEqual=[](const Relocation_t* a, const Relocation_t* b) -> bool
{
	if(a==NULL && b==NULL)
		return true;
	if(a==NULL || b==NULL)
		return false;
	return 
		forward_as_tuple(a->GetType(), a->GetOffset(), a->GetWRT(), a->GetAddend()) == 
		forward_as_tuple(b->GetType(), b->GetOffset(), b->GetWRT(), b->GetAddend());
};

template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::FDErepresentation_t::LSDArepresentation_t::canExtend(Instruction_t* insn) const
{
	if(insn->GetEhCallSite() == NULL)
		return true;

	const auto insn_tt_encoding = insn->GetEhCallSite()->GetTTEncoding();

	// no type table
	if(tt_encoding==0xff || insn_tt_encoding==0xff) // DW_EH_PE_omit (0xff)
	{
		// no encoding issues.
	}
	// check if tt encodings match.
	else if(insn_tt_encoding!=tt_encoding)
		return true;

	assert((tt_encoding&0xf)==0x3 || 	// encoding contains DW_EH_PE_udata4 
	       (tt_encoding)==0xff || 		// or is exactly DW_EH_PE_omit
 	       (tt_encoding&0xf)==0xb ); 	// or encoding contains DW_EH_PE_sdata4
	const auto tt_entry_size=4;

	const auto mismatch_tt_entry = find_if(
		insn->GetEhCallSite()->GetRelocations().begin(),
		insn->GetEhCallSite()->GetRelocations().end(),
		[&](const Relocation_t* candidate_reloc)
			{
				const auto tt_index=candidate_reloc->GetOffset()/tt_entry_size;
				if(tt_index>=(int64_t)type_table.size())
					return false;
				const auto &tt_entry=type_table.at(tt_index);
	
				if(tt_entry==NULL) // entry is empty, so no conflict
					return false;
				return !RelocsEqual(candidate_reloc, tt_entry);
			}
		);

	// return true if we found no mismatches
	return (mismatch_tt_entry == insn->GetEhCallSite()->GetRelocations().end());
}

template <int ptrsize>
void EhWriterImpl_t<ptrsize>::FDErepresentation_t::LSDArepresentation_t::extend(Instruction_t* insn)
{
	// if there's no call site info, the LSDA doesn't need an extension.
	if(insn->GetEhCallSite() == NULL)
		return;

	const auto insn_tt_encoding = insn->GetEhCallSite()->GetTTEncoding();

	// FIXME: optimization possibilty:  see if the last call site in the table
	// has the same set of catch-types + landing_pad and is "close enough" to this insn.
	// if so, combine.  

	cout<<"Creating call sites in LSDA for "<<hex<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;

	// just create a new entry in the CS table.. 
	auto cs=(call_site_t){0}; 

	cs.cs_insn_start=insn;
	cs.cs_insn_end=insn;
	cs.landing_pad=insn->GetEhCallSite()->GetLandingPad();

	if(tt_encoding == 0xff /* omit */)
	{
		/* existing is omit, use new encoding */
		tt_encoding=insn_tt_encoding;
	}
	else if(insn_tt_encoding == 0xff /* omit */)
	{
		/* new encoding is omit, use current tt encoding */
	}
	else if(tt_encoding == insn_tt_encoding)
	{
		/* encodings match, -- do nothing */
	}
	else
		assert(0); // canExtend failure?
	
		


	// go through the relocations on the eh call site and 
	// resolve each one by putting it into a set.
	// the set will be (non-duplicating) inserted as a "entry" (really multiple entries) 
	// into the action table,
	// Each reloc will also get a non-duplicating insert into the type table.
	for(const auto &reloc : insn->GetEhCallSite()->GetRelocations())
	{
		const auto wrt_scoop=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
		if(reloc->GetWRT()==NULL)
			cout<<"\tFound reloc: NULL (catch all)"<<endl;
		else if(wrt_scoop)
			cout<<"\tFound reloc: scoop "<<wrt_scoop->GetName()<<"+0x"<<hex<<reloc->GetAddend()<<endl;
		else
			cout<<"\tFound reloc: unexpected type? "<<endl;

		// for now, this is the only supported reloc type on a EhCallSite 
		assert(reloc->GetType()=="type_table_entry");
		auto tt_it=find_if(type_table.begin(),type_table.end(), 
			[reloc](const Relocation_t* candidate) { return candidate!=NULL && RelocsEqual(candidate,reloc); });
		if(tt_it==type_table.end())
		{
			const auto tt_encoding = insn->GetEhCallSite()->GetTTEncoding();
			assert((tt_encoding&0xf)==0x3 ||  // encoding contains DW_EH_PE_udata4
			       (tt_encoding&0xf)==0xb); // encoding contains DW_EH_PE_sdata4
			const auto tt_entry_size=4;
			const auto tt_index= reloc->GetOffset()/tt_entry_size;
			if(tt_index>=(int64_t)type_table.size())
				type_table.resize(tt_index+1);
			assert(type_table.at(tt_index)==NULL || RelocsEqual(type_table.at(tt_index),reloc));	
			type_table[tt_index]=reloc;	
		}
	}

	cs.actions=insn->GetEhCallSite()->GetTTOrderVector();

	auto at_it=find(action_table.begin(),action_table.end(), cs.actions);
	if(at_it==action_table.end())
	{
		// actions will be at the end of the action table.
		cs.action_table_index=action_table.size();

		// add to the action table
		action_table.push_back(cs.actions);
	}
	else
	{
		// calc which entry in the action table for the call site.
		cs.action_table_index=at_it-action_table.begin();
	}

	// the call site will index the action table at the end of the action table.
	callsite_table.push_back(cs);
}

template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::FDErepresentation_t::canExtend(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
{
	const auto insn_addr=ehw->zipr_obj.GetLocationMap()->at(insn);
	const auto new_fde_thresh=100;

	if(insn_addr<start_addr)
		return false;

	if(end_addr + new_fde_thresh < insn_addr)
		return false;

	assert(cie);
	if(!cie->canSupport(insn))
		return false; // can't change the CIE.

	return pgm.canExtend(insn->GetEhProgram()->GetFDEProgram()) && 
		lsda.canExtend(insn);


}

template <int ptrsize>
void EhWriterImpl_t<ptrsize>::FDErepresentation_t::extend(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
{
	const auto insn_addr=ehw->zipr_obj.GetLocationMap()->at(insn);
	const auto new_end_addr=insn_addr+insn->GetDataBits().size();
	const auto incr_amnt=insn_addr-last_advance_addr;
	last_advance_addr=insn_addr;

	// add appropriate instructions to the pgm.
	pgm.extend((incr_amnt)/cie->code_alignment_factor, insn->GetEhProgram()->GetFDEProgram());

	lsda.extend(insn);

	// extend the end 
	end_addr=new_end_addr;
	
}

template <int ptrsize>
void EhWriterImpl_t<ptrsize>::FDErepresentation_t::emitAssembly(ostream& out)
{
	out<<"Hello"<<endl;
}

template<int ptrsize>
void EhWriterImpl_t<ptrsize>::BuildFDEs()
{
	// build a map of the instructions in program order
	map<virtual_offset_t,Instruction_t*> insn_in_order;
	for(const auto& this_pair : *zipr_obj.GetLocationMap())
		insn_in_order[this_pair.second]=this_pair.first;


	// build the fdes (and cies/lsdas) for this insn, starting with a null fde in case none exist
	auto current_fde=(FDErepresentation_t*)NULL;
	auto insns_with_frame=0;

	// for_each instruction in program order
	for(const auto& this_pair : insn_in_order)
	{
		const auto &this_insn=this_pair.second;
		const auto &this_addr=this_pair.first;

		// no eh pgm or call site?  no worries, just ignore this insn
		if(this_insn->GetEhProgram()==NULL && this_insn->GetEhCallSite()==NULL)
			continue;

		insns_with_frame++;

		// if it has an unwinder and/or a call site, we will need an fde.

		// end this fde
		if(current_fde && !current_fde->canExtend(this_insn, this))
		{
			if(getenv("EH_VERBOSE")!=NULL)
				cout<<"Ending FDE because insn "<<hex<<this_insn->GetBaseID()<<":"<<this_insn->getDisassembly()<<" doesn't fit at " << this_addr<< endl;
			current_fde=NULL;
		}


		// if we need to start a new fde, create one.
		if(current_fde==NULL)
		{
			if(getenv("EH_VERBOSE")!=NULL)
				cout<<"Creating new FDE for "<<hex<<this_insn->GetBaseID()<<":"<<this_insn->getDisassembly()<< " at " << this_addr<<endl;
			current_fde=new FDErepresentation_t(this_insn,this);
			all_fdes.push_back(current_fde);
		}
		else
		{
			if(getenv("EH_VERBOSE")!=NULL)
			{
				cout<<"Extending new FDE for "<<hex<<this_insn->GetBaseID()<<":"<<this_insn->getDisassembly()<<" at " << this_addr <<endl;
				print_pers(this_insn,current_fde->cie);

			}
			current_fde->extend(this_insn,this);
		}
	}
	cout<<"#ATTRIBUTE fdes_calculated="<<dec<<all_fdes.size()<<endl;
	cout<<"#ATTRIBUTE cies_calculated="<<dec<<all_cies.size()<<endl;
	cout<<"#ATTRIBUTE insns_with_eh_info="<<dec<<insns_with_frame<<endl;
	cout<<"#ATTRIBUTE avg_insns_per_fde="<<dec<<insns_with_frame/(float)all_fdes.size()<<endl;
}

template<int ptrsize>
void EhWriterImpl_t<ptrsize>::GenerateEhOutput()
{

	auto output_program=[&](const EhProgramListingManip_t& p, ostream & out) 
	{
		auto flags=out.flags();//save flags
		for(auto i=0U; i < p.size() ; i ++ ) 
		{		
			const auto &s = p[i];
			if(i==p.size()-1 && p.isAdvanceDirective(s))	/* no need to output last insn if it's an advance */
				break;
			out<<"       .byte ";
			auto first=true;
			for(const auto &c : s)
			{
				if(!first)
				{
					out<<", ";
				}
				else
					first=false;
				out<<"0x"<<hex<<setfill('0')<<setw(2)<<((int)c&0xff);
			}
			out << endl;
		}
		out.flags(flags); // restore flags
	};
	const auto output_lsda=[&](const FDErepresentation_t* fde, ostream& out, const uint32_t lsda_num) -> void
	{
		const auto lsda=&fde->lsda;
		out<<"LSDA"<<dec<<lsda_num<<":"<<endl;
		if(!lsda->exists())
		{
			return;
		}

		// the encoding of the landing pad is done in two parts:
		// landing_pad=landing_pad_base+landing_pad_offset.
		// typically, the landing_pad_base is omitted and the fde->start_addr is used instead.  
		// However, sometimes the fde->start_addr is equal to the landing_pad,
		// which would make the landing_pad_offset 0.
		// but a landing_pad offset of 0 indicates no landing pad.
		// To avoid this situation, we first detect it,
		// then arbitrarily pick (and encode) a landing_pad_base that's
		// not equal to any landing paad in the call site list.
		const auto calc_landing_pad_base=[&]() -> uint64_t
		{
			// look for a landing pad that happens to match the fde->start_addr
			const auto lp_base_conflict_it=find_if(
				lsda->callsite_table.begin(),
				lsda->callsite_table.end(),
				[&](const typename FDErepresentation_t::LSDArepresentation_t::call_site_t& candidate)
				{
					if(candidate.landing_pad==NULL)
						return false;
					const auto lp_addr=zipr_obj.GetLocationMap()->at(candidate.landing_pad);
					return (lp_addr==fde->start_addr);
				});

			// if not found, there's no need to adjust the landing pad base address.
			if(lp_base_conflict_it==lsda->callsite_table.end())
				return fde->start_addr;

			// we pick an arbitrary landing_pad base by taking the min of all
			// landing pads (and the fde_start addr), and subtracting 1.
			const auto min_cs_entry_it=min_element(
				lsda->callsite_table.begin(),
				lsda->callsite_table.end(),
				[&](const typename FDErepresentation_t::LSDArepresentation_t::call_site_t& a , const typename FDErepresentation_t::LSDArepresentation_t::call_site_t &b)
				{
					const auto lp_a=a.landing_pad;
					const auto lp_b=b.landing_pad;
					if(lp_a && lp_b)
					{
						const auto lp_addr_a=zipr_obj.GetLocationMap()->at(lp_a);
						const auto lp_addr_b=zipr_obj.GetLocationMap()->at(lp_b);
						return lp_addr_a<lp_addr_b;
					}
					if(!lp_a && lp_b)
						return false;
					if(!lp_b && lp_a)
						return true;
					return false;
				});
			const auto &min_cs_entry=*min_cs_entry_it;
			const auto min_lp_addr=zipr_obj.GetLocationMap()->at(min_cs_entry.landing_pad);
			const auto min_addr=min(min_lp_addr,fde->start_addr);
			return min_addr-1;
	
		};

		const auto landing_pad_base=calc_landing_pad_base();

		// how to output actions
		const auto output_action=[&](const libIRDB::TTOrderVector_t &act, const uint32_t act_num) -> void
		{
			const auto &ttov=act;
			const auto biggest_ttov_index=ttov.size()-1;
			auto act_entry_num=biggest_ttov_index;

			for(int i=act_entry_num; i>=0; i--)
			{
				out<<"LSDA"<<dec<<lsda_num<<"_act"<<act_num<<"_start_entry"<<act_entry_num<<""<<":"<<endl;
				out<<"	.uleb128 "<<dec<<ttov.at(act_entry_num)<<endl;        
				if(act_entry_num==biggest_ttov_index)
					out<<"	.uleb128 0 "<<endl;
				else
					out<<"	.uleb128  LSDA"<<lsda_num<<"_act"<<act_num<<"_start_entry"<<act_entry_num+1<<" - . "<<endl;
				act_entry_num--;
			}
		};


		const auto output_callsite=[&](const typename FDErepresentation_t::LSDArepresentation_t::call_site_t &cs, const uint32_t cs_num) -> void
		{
			const auto cs_start_addr=zipr_obj.GetLocationMap()->at(cs.cs_insn_start);
			const auto cs_end_addr=zipr_obj.GetLocationMap()->at(cs.cs_insn_start)+cs.cs_insn_start->GetDataBits().size();
			const auto cs_len=cs_end_addr-cs_start_addr;
			out<<"LSDA"<<dec<<lsda_num<<"_cs_tab_entry"<<cs_num<<"_start:"<<endl;
        		out<<"	# 1) start of call site relative to FDE start addr"<<endl;
        		out<<"	.uleb128 0x"<<hex<<cs_start_addr<<" - 0x"<<hex<<fde->start_addr<<endl;
        		out<<"	# 2) length of call site"<<endl;
        		out<<"	.uleb128 "<<dec<<cs_len<<endl;
			if(cs.landing_pad)
			{
				const auto lp_addr=zipr_obj.GetLocationMap()->at(cs.landing_pad);
				out<<"	# 3) the landing pad, or 0 if none exists."<<endl;
				out<<"	.uleb128 0x"<<hex<<lp_addr<<" - 0x"<<hex<<landing_pad_base<<endl;
			}
			else
			{
				out<<"	# 3) the landing pad, or 0 if none exists."<<endl;
				out<<"	.uleb128 0"<<endl;
			}
			if(cs.actions.size() > 0 )
			{
				out<<"	# 4) index into action table + 1 -- 0 indicates unwind only"<<endl;
				out<<"	.uleb128 1 + LSDA"<<dec<<lsda_num<<"_act"
				   <<cs.action_table_index<<"_start_entry0 - LSDA"<<dec<<lsda_num<<"_action_tab_start"<<endl;
			}
			else
			{
				out<<"	# 4) index into action table + 1 -- 0 indicates unwind only"<<endl;
				out<<"	.uleb128 0 # no actions!" << endl;
			}
			out<<"LSDA"<<dec<<lsda_num<<"_cs_tab_entry"<<cs_num<<"_end:"<<endl;

		};

		const auto output_lsda_header=[&]()
		{
			if(landing_pad_base==fde->start_addr)
			{
				out<<"        # 1) encoding of next field "<<endl;
				out<<"        .byte 0xff # DW_EH_PE_omit (0xff)"<<endl;
				out<<""<<endl;
				out<<"        # 2) landing pad base, if omitted, use FDE start addr"<<endl;
				out<<"        # .<fdebasetype> <fdebase> -- omitted.  "<<endl;
			}
			else
			{
				out<<"        # 1) encoding of next field "<<endl;
				out<<"        .byte 0x1b # DW_EH_PE_pcrel (0x10) |sdata4 (0xb)"<<endl;
				out<<""<<endl;
				out<<"        # 2) landing pad base, if omitted, use FDE start addr"<<endl;
				out<<"        .int  0x"<<hex<<landing_pad_base<<"- . # as pcrel|sdata4 .  "<<endl;
			}
			out<<""<<endl;
			out<<"        # 3) encoding of type table entries"<<endl;
			out<<"        .byte 0x"<<hex<<lsda->tt_encoding<<"  # DW_EH_PE_udata4"<<endl;
			out<<""<<endl;
			out<<"        # 4) type table pointer -- always a uleb128"<<endl;
			if(lsda->tt_encoding==0xff) /* omit */
			{
				out<<"        # .uleb128 LSDAptr omitted"<< endl;
			}
			else
			{
				out<<"        .uleb128 LSDA"<<dec<<lsda_num<<"_type_table_end - LSDA"<<lsda_num<<"_tt_ptr_end"<<endl;
			}
			out<<"LSDA"<<dec<<lsda_num<<"_tt_ptr_end:"<<endl;
			out<<""<<endl;
			out<<"        # 5) call site table encoding"<<endl;
			out<<"        .byte 0x1 # DW_EH_PE_uleb128 "<<endl;
			out<<""<<endl;
			out<<"        # 6) the length of the call site table"<<endl;
			out<<"        .uleb128 LSDA"<<dec<<lsda_num<<"_cs_tab_end-LSDA"<<dec<<lsda_num<<"_cs_tab_start"<<endl;
			out<<"LSDA"<<dec<<lsda_num<<"_cs_tab_start:"<<endl;
		};

		const auto output_call_site_table=[&]()
		{
			// output call site table
			auto cs_num=0;
			for(const auto & cs : lsda->callsite_table)
			{
				output_callsite(cs,cs_num++);
			}
			out<<"LSDA"<<dec<<lsda_num<<"_cs_tab_end:"<<endl;
		};
		const auto output_action_table=[&]()
		{
			// output action table
			out<<"LSDA"<<dec<<lsda_num<<"_action_tab_start:"<<endl;
			auto act_num=0;
			for(const auto & act : lsda->action_table)
			{
				output_action(act,act_num++);
			}
			out<<"LSDA"<<dec<<lsda_num<<"_action_tab_end:"<<endl;
		};

		const auto output_type_table=[&]()
		{
			out<<"LSDA"<<dec<<lsda_num<<"_type_table_start:"<<endl;
			for_each( lsda->type_table.rbegin(), lsda->type_table.rend(),  [&](const Relocation_t* reloc)
			{
				if(reloc==NULL)
				{
					// indicates a catch all or empty type table entry
					out<<"	.int 0x0 # not used!"<<endl;
				}
				else if(reloc->GetWRT()==NULL)
				{
					// indicates a catch all or empty type table entry
					out<<"	.int 0x0 # catch all "<<endl;
				}
				else
				{
					// indicates a catch of a paritcular type
					const auto scoop=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
					assert(scoop);
					const auto final_addr=scoop->GetStart()->GetVirtualOffset() + reloc->GetAddend();
					if(((lsda->tt_encoding)&0x10) == 0x10) // if encoding contains pcrel (0x10).
						out<<"	.int 0x"<<hex<<final_addr<<" - . "<<endl;
					else
						out<<"	.int 0x"<<hex<<final_addr<<endl;
					
				}
			});
			out<<"LSDA"<<dec<<lsda_num<<"_type_table_end:"<<endl;
		};

		output_lsda_header();
		output_call_site_table();
		output_action_table();
		output_type_table();

	};
	const auto output_cie=[&](const CIErepresentation_t* cie, ostream& out) -> void
	{
		assert(cie);
		if(cie->has_been_output)
			return;

		const auto cie_pos_it=std::find(all_cies.begin(), all_cies.end(), cie);
		assert(cie_pos_it!=all_cies.end());

		const auto personality_scoop=cie->personality_reloc ? dynamic_cast<DataScoop_t*>  (cie->personality_reloc->GetWRT()) : (DataScoop_t*)NULL;
		const auto personality_insn =cie->personality_reloc ? dynamic_cast<Instruction_t*>(cie->personality_reloc->GetWRT()) : (Instruction_t*)NULL;
		const auto personality_addend=cie->personality_reloc ? cie->personality_reloc->GetAddend() : 0;

		const auto cie_pos=cie_pos_it-all_cies.begin();

		cie->has_been_output=true;
		out<<"# cie "<<dec<<cie_pos<<""<<endl;
		out<<"Lcie"<<cie_pos<<":"<<endl;
		out<<"	 .int Lcie"<<cie_pos<<"_end - Lcie"<<cie_pos<<" - 4 # length of this record. -4 because length doesn't include this field"<<endl;
		out<<"        .int 0                  # cie (not fde)"<<endl;
		out<<"        .byte 3                 # version"<<endl;
		out<<"        .asciz \"zPLR\"           # aug string."<<endl;
		out<<"        .uleb128 "<<dec<<cie->code_alignment_factor<<"              # code alignment factor"<<endl;
		out<<"        .sleb128 "<<dec<<cie->data_alignment_factor<<"             # data alignment factor"<<endl;
		out<<"        .uleb128 "<<dec<<cie->return_reg<<"             # return address reg."<<endl;
		out<<"        # encode the Z (length)"<<endl;
		out<<"        .sleb128 Lcie"<<cie_pos<<"_aug_data_end-Lcie"<<cie_pos<<"_aug_data_start # Z -- handle length field"<<endl;
		out<<"Lcie"<<cie_pos<<"_aug_data_start:"<<endl;
		out<<""<<endl;
		if(personality_scoop)
		{
			auto personality_value=personality_scoop->GetStart()->GetVirtualOffset()+personality_addend;
			out<<"        #encode the P (personality encoding + personality routine)"<<endl;
			out<<"        .byte 0x80 | 0x10 | 0x0B        #  personality pointer encoding DH_EH_PE_indirect (0x80) | pcrel | sdata4"<<endl;
			out<<"        .int "<<personality_value<<" - .               # actual personality routine, encoded as noted in prev line."<<endl;
		}
		else if(personality_insn)
		{
			const auto personality_insn_addr=zipr_obj.GetLocationMap()->at(personality_insn);
			const auto personality_value=personality_insn_addr+personality_addend;
			out<<"        #encode the P (personality encoding + personality routine)"<<endl;
			out<<"        .byte 0x10 | 0x0B        #  personality pointer encoding pcrel | sdata4"<<endl;
			out<<"        .int "<<personality_value<<" - .               # actual personality routine, encoded as noted in prev line."<<endl;
		}
		else
		{
			assert(cie->personality_reloc==NULL || cie->personality_reloc->GetWRT()==NULL);
			out<<"        #encode the P (personality encoding + personality routine)"<<endl;
			out<<"        .byte  0x0B        #  personality pointer encoding sdata4"<<endl;
			out<<"        .int 0               # actual personality routine, encoded as noted in prev line."<<endl;
		}
		out<<""<<endl;
		out<<"        # encode L (lsda encoding) "<<endl;
		out<<"        .byte  0x1b     # LSDA encoding (pcrel|sdata4)"<<endl;
		out<<""<<endl;
		out<<"        # encode R (FDE encoding) "<<endl;
		out<<"        .byte  0x10 | 0x0B      # FDE encoding (pcrel | sdata4)"<<endl;
		out<<"Lcie"<<cie_pos<<"_aug_data_end:"<<endl;
		out<<"       # CIE program"<<endl;
		output_program(cie->pgm,out);
		out<<""<<endl;
		out<<"       # pad with nops"<<endl;
		out<<"       .align 4, 0"<<endl;
		out<<"Lcie"<<cie_pos<<"_end:"<<endl;

	};
	auto output_fde=[&](const FDErepresentation_t* fde, ostream& out, const uint32_t fde_num) -> void
	{
		assert(fde && fde->cie);
		output_cie(fde->cie,out);


		auto cie_pos_it=std::find(all_cies.begin(), all_cies.end(), fde->cie);
		assert(cie_pos_it!=all_cies.end());
		auto cie_pos=cie_pos_it-all_cies.begin();

		out<<"#fde "<<dec<<fde_num<<""<<endl;
		out<<"Lfde"<<fde_num<<":"<<endl;
		out<<"        .int Lfde"<<fde_num<<"_end - Lfde"<<fde_num<<" - 4      # length of this record. -4 because "
		     "length doesn't include this field."<<endl;
		out<<"        .int . - Lcie"<<cie_pos<<"                  # this is an FDE (not a "
		     "cie), and it's cie is CIE"<<cie_pos<<".  byte offset from start of field."<<endl;
		out<<"        .int 0x"<<hex<<fde->start_addr<<dec<<" - .               # FDE start addr"<<endl;
		out<<"        .int "<<dec<<fde->end_addr-fde->start_addr<<"                     # fde range length (i.e., can calc the "
		     "fde_end_addr from this -- note that pcrel is ignored here!)"<<endl;
		out<<"        #encode Z (length)"<<endl;
		out<<"        .uleb128 Lfde"<<fde_num<<"_aug_data_end-Lfde"<<fde_num<<"_aug_data_start"<<endl;
		out<<"Lfde"<<fde_num<<"_aug_data_start:"<<endl;
		out<<"        #encode L (LSDA) "<<endl;
		if(fde->hasLsda())
			out<<"        .int LSDA"<<fde_num<<" - .    # LSDA hard coded here (as pcrel+sdata4)"<<endl;	 
		else
			out<<"        .int 0      # no LSDA "<<endl;	 
		out<<"Lfde"<<fde_num<<"_aug_data_end:"<<endl;
		out<<""<<endl;
		out<<"        # FDE"<<fde_num<<" program"<<endl;
		output_program(fde->pgm,out);
		out<<"        .align 4, 0"<<endl;
		out<<"        Lfde"<<fde_num<<"_end:"<<endl;

	};
	auto generate_eh_frame_hdr=[&](ostream& out) -> void	
	{
		out<<".section eh_frame_hdr, \"a\", @progbits"<<endl;
		out<<"eh_frame_hdr_start:"<<endl;
		out<<"        .byte 1                 # version"<<endl;
		out<<"        .byte 0x10 | 0x0B       # encoding for pointer to eh-frame -- DH_EH_PE_pcrel (0x10) | DH_EH_PE_sdata4 (0x0B)"<<endl;
		out<<"        .byte 0x03              # encoding for ; of entries in eh-frame-hdr  -- BDH_EH_PE_udata4 (0x03)"<<endl;
		out<<"        .byte 0x30 | 0x0B       # encoding for pointers (to fdes) held in the eh-frame-hdr header  "
		     "-- DH_EH_PE_datarel (0x30) | DH_EH_PE_sdata4 (0x0b) " <<endl;

		out<<"        .int Lfde_table - .     # pointer to fde_table, encoded as an sdata4, pcrel"<<endl;
		out<<"        .int (eh_frame_table_end-eh_frame_table)/8     # number of FDEs in the header."<<endl;
		out<<"        .align 4"<<endl;
		out<<"eh_frame_table:"<<endl;
		out<<"        # fde pointers"<<endl;

		for(auto fde_num=0U; fde_num < all_fdes.size(); fde_num++)
		{
			const auto& fde=all_fdes[fde_num];
			out<<"        .int 0x"<<hex<<fde->start_addr<<" - eh_frame_hdr_start"<<endl;
			out<<"        .int Lfde"<<dec<<fde_num<<" - eh_frame_hdr_start"<<endl;
		}

		out<<"eh_frame_table_end:"<<endl;

	};
	auto generate_eh_frame=[&](ostream& out) -> void	
	{
		out<<".section eh_frame, \"a\", @progbits"<<endl;
		out<<"Lfde_table: # needed for xref to eh_frame_hdr" <<endl;

		auto fde_num=0;
		for(const auto& fde: all_fdes)
		{
			output_fde(fde,out, fde_num++);
		}
	};
	auto generate_gcc_except_table=[&](ostream& out) -> void	
	{
		out<<".section gcc_except_table, \"a\", @progbits"<<endl;
		auto lsda_num=0;
		for(const auto& fde: all_fdes)
		{
			output_lsda(fde,out,lsda_num++);
		}
	};

	ofstream fout(ehframe_s_filename, std::ofstream::out);
	if(!fout)
	{
		cerr<<"Fatal: cannot open "<<ehframe_s_filename<<"."<<endl;
		exit(2);
	}

	generate_eh_frame_hdr(fout);
	generate_eh_frame(fout);
	generate_gcc_except_table(fout);
}

template<int ptrsize>
void EhWriterImpl_t<ptrsize>::CompileEhOutput()
{
	auto page_round_up=[](const uintptr_t x) -> uintptr_t 
	{
		auto page_size=(uintptr_t)PAGE_SIZE;
		return  ( (((uintptr_t)(x)) + page_size-1)  & (~(page_size-1)) );
	};

	// find maximum used scoop address.
	const auto max_used_addr=std::max_element(
		zipr_obj.GetFileIR()->GetDataScoops().begin(),
		zipr_obj.GetFileIR()->GetDataScoops().end(),
		[&](const DataScoop_t* a, const DataScoop_t* b)
		{
			assert(a && b && a->GetEnd() && b->GetEnd()) ;
			return a->GetEnd()->GetVirtualOffset() < b->GetEnd()->GetVirtualOffset();
		}
		);

	// round it up and stringify it.
	const auto eh_frame_hdr_addr=page_round_up((*max_used_addr)->GetEnd()->GetVirtualOffset());
	const auto eh_frame_hdr_addr_str=to_hex_string(eh_frame_hdr_addr);

	// create and execute the command to build the ehframe.
	auto cmd=(string)"$PEASOUP_HOME/tools/eh_frame_tools/eh_to_bin.sh "+ehframe_s_filename+" "+eh_frame_hdr_addr_str+" "+ehframe_exe_filename;
	auto res=system(cmd.c_str());

	// err check.
	if( res==-1 || WEXITSTATUS(res)!=0 )
	{
		perror("Cannot compile eh_frame.");
		cerr<<"Exit code="<<res<<endl;
		cerr<<" for command="<<cmd<<endl;
		exit(2);
	}
}

template<int ptrsize>
void EhWriterImpl_t<ptrsize>::ScoopifyEhOutput()
{
        ELFIO::elfio ehframe_exe_elfio;
	ehframe_exe_elfio.load(ehframe_exe_filename);

	auto to_scoop=[&](const string &secname)->void
	{
		const auto &sec=ehframe_exe_elfio.sections[secname];

		// if sec is missing, don't scoopify.

		if(sec==NULL) return;
		const auto data=string(sec->get_data(), sec->get_size());
		const auto start_vo=sec->get_address();
		const auto start_addr=new AddressID_t(BaseObj_t::NOT_IN_DATABASE, BaseObj_t::NOT_IN_DATABASE, start_vo);
		const auto end_vo=sec->get_address()+sec->get_size()-1;
		const auto end_addr=new AddressID_t(BaseObj_t::NOT_IN_DATABASE, BaseObj_t::NOT_IN_DATABASE, end_vo);
		const auto new_scoop=new DataScoop_t(zipr_obj.GetFileIR()->GetMaxBaseID()+1, secname, start_addr,end_addr,NULL,4,false,data);

		zipr_obj.GetFileIR()->GetAddresses().insert(start_addr);
		zipr_obj.GetFileIR()->GetAddresses().insert(end_addr);
		zipr_obj.GetFileIR()->GetDataScoops().insert(new_scoop);
	};

	to_scoop(".eh_frame_hdr");	
	to_scoop(".eh_frame");	
	to_scoop(".gcc_except_table");	


}

template<int ptrsize>
EhWriterImpl_t<ptrsize>::~EhWriterImpl_t()
{
	for(const auto &i : all_fdes) delete i;
	for(const auto &i : all_cies) delete i;
}


/* make sure these get compiled */
namespace zipr
{
template class EhWriterImpl_t<8>;
template class EhWriterImpl_t<4>;
};
