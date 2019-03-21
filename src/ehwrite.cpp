
#include <zipr_all.h>
#include <irdb-core>
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

#include <zipr_dwarf2.hpp>

#include "exeio.h"

using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace EXEIO;

template < typename T > std::string to_hex_string( const T& n )
{
        std::ostringstream stm ;
        stm << std::hex<< "0x"<< n ;
        return stm.str() ;
}

template<int ptrsize>
void EhWriterImpl_t<ptrsize>::GenerateNewEhInfo()
{
	auto page_round_up=[](const uintptr_t x) -> uintptr_t 
	{
		auto page_size=(uintptr_t)PAGE_SIZE;
		return  ( (((uintptr_t)(x)) + page_size-1)  & (~(page_size-1)) );
	};

	// find maximum used scoop address.
	const auto max_used_addr=std::max_element(
		zipr_obj.getFileIR()->getDataScoops().begin(),
		zipr_obj.getFileIR()->getDataScoops().end(),
		[&](const DataScoop_t* a, const DataScoop_t* b)
		{
			assert(a && b && a->getEnd() && b->getEnd()) ;
			return a->getEnd()->getVirtualOffset() < b->getEnd()->getVirtualOffset();
		}
		);

	// round it up and stringify it.
	eh_frame_hdr_addr=page_round_up((*max_used_addr)->getEnd()->getVirtualOffset());

	
	BuildFDEs();
	GenerateEhOutput();
	CompileEhOutput();
	ScoopifyEhOutput();

}

template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::CIErepresentation_t::canSupport(Instruction_t* insn) const
{
	// if the insn is missing info, we can support it.
	if(insn==nullptr)
		return true;
	if(insn->getEhProgram()==nullptr)
		return true;

	// all info here.


	// check that the program,CAF, DAF and RR match.  if not, can't support
	if(insn->getEhProgram()->getCIEProgram() != pgm || 
	   insn->getEhProgram()->getCodeAlignmentFactor() != code_alignment_factor || 
	   insn->getEhProgram()->getDataAlignmentFactor() != data_alignment_factor || 
	   insn->getEhProgram()->getReturnRegNumber() != (int64_t)return_reg 
	  )
		return false;

	auto personality_it=find_if(
		insn->getEhProgram()->getRelocations().begin(), 
		insn->getEhProgram()->getRelocations().end(),
		[](const Relocation_t* r) { return r->getType()=="personality"; });

	// lastly, check for a compatible personality reloc.
	// if incoming has no personality, we better have no personality to match.
	if(personality_it==insn->getEhProgram()->getRelocations().end())
	{
		return personality_reloc==nullptr;
	}

	// incomming has personality, but do we?
	if(personality_reloc==nullptr) return false;

	// compare personalities.
	if(personality_reloc->getWRT() != (*personality_it)->getWRT()) return false;
	if(personality_reloc->getAddend() != (*personality_it)->getAddend()) return false;

	return true;
}

template <int ptrsize>
EhWriterImpl_t<ptrsize>::CIErepresentation_t::CIErepresentation_t(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
	: has_been_output(false)
{
	assert(insn && ehw && insn->getEhProgram());

	pgm = insn->getEhProgram()->getCIEProgram();
	code_alignment_factor = insn->getEhProgram()->getCodeAlignmentFactor();
	data_alignment_factor = insn->getEhProgram()->getDataAlignmentFactor();
	return_reg = insn->getEhProgram()->getReturnRegNumber();

	auto personality_it=find_if(
		insn->getEhProgram()->getRelocations().begin(), 
		insn->getEhProgram()->getRelocations().end(),
		[](const Relocation_t* r) { return r->getType()=="personality";});

	personality_reloc = (personality_it==insn->getEhProgram()->getRelocations().end())
		? (Relocation_t*)nullptr
		: *personality_it;
}


template <int ptrsize>
void EhWriterImpl_t<ptrsize>::print_pers(Instruction_t* insn, EhWriterImpl_t<ptrsize>::CIErepresentation_t *cie)
{
	const auto pretty_print= [&](Relocation_t* pr)
		{
			if(pr==nullptr)
			{
				cout<<"Found no personality reloc"<<endl;
				return;
			}
			const auto personality_scoop=dynamic_cast<DataScoop_t*>(pr->getWRT());
			const auto personality_insn=dynamic_cast<Instruction_t*>(pr->getWRT());

			if(pr->getWRT()==nullptr)
				cout<<"\tFound null personality"<<endl;
			else if(personality_scoop)
				cout<<"\tFound personlity scoop "<<personality_scoop->getName()<<"+0x"<<hex<<pr->getAddend()<<endl;
			else if(personality_insn)
				cout<<"\tFound personlity instruction "<<hex<<personality_insn->getBaseID()<<dec<<":"<<hex<<personality_insn->getDisassembly()<<endl;
			else
				cout<<"\tFound reloc: unexpected type? "<<endl;
		};

	cout<<"  CIE-Personality addr= "<<hex<<cie->personality_reloc<<dec<<endl;
	pretty_print(cie->GetPersonalityReloc());
	const auto personality_it=find_if(
		insn->getEhProgram()->getRelocations().begin(), 
		insn->getEhProgram()->getRelocations().end(),
		[](const Relocation_t* r) { return r->getType()=="personality"; });

	const auto pr = (personality_it==insn->getEhProgram()->getRelocations().end())
		? (Relocation_t*)nullptr
		: *personality_it;
	cout<<"  insn personality addr= "<<hex<<pr<<dec<<endl;
	pretty_print(pr);


};


template <int ptrsize>
EhWriterImpl_t<ptrsize>::FDErepresentation_t::FDErepresentation_t(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
	: 
		lsda(insn),
		cie(nullptr)
{
	auto cie_it=find_if( ehw->all_cies.begin(), ehw->all_cies.end(), [&](const CIErepresentation_t* candidate)
			{
				return candidate->canSupport(insn);
			});

	if(cie_it==ehw->all_cies.end())
	{
		cie=new CIErepresentation_t(insn,ehw);
		ehw->all_cies.push_back(cie);

		if(getenv("EH_VERBOSE")!=nullptr)
			cout<<"Creating new CIE representation"<<endl;
	}
	else
	{
		cie=*cie_it;
		if(getenv("EH_VERBOSE")!=nullptr)
		{
			cout<<"Re-using CIE representation"<<endl;
			print_pers(insn, cie);
		}
	}

	start_addr=ehw->zipr_obj.GetLocationMap()->at(insn);
	last_advance_addr=start_addr;
	end_addr=start_addr+insn->getDataBits().size();
	pgm=EhProgramListingManip_t(insn->getEhProgram()->getFDEProgram());
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

// see https://en.wikipedia.org/wiki/LEB128
template <int ptrsize>
static bool read_uleb128
        ( uint64_t &result,
        uint32_t& position,
        const uint8_t* const data,
        const uint32_t max)
{
        result = 0;
        auto shift = 0;
        while( position < max )
        {
                auto byte = data[position];
                position++;
                result |= ( ( byte & 0x7f ) << shift);
                if ( ( byte & 0x80) == 0)
                        break;
                shift += 7;
        }
        return ( position > max );

}

// see https://en.wikipedia.org/wiki/LEB128
template <int ptrsize>
static bool read_sleb128 (
        int64_t &result,
        uint32_t & position,
        const uint8_t* const data,
        const uint32_t max)
{
        result = 0;
        auto shift = 0;
        auto size = 64;  // number of bits in signed integer;
        auto byte=uint8_t(0);
        do
        {
                byte = data [position];
                position++;
                result |= ((byte & 0x7f)<< shift);
                shift += 7;
        } while( (byte & 0x80) != 0);

        /* sign bit of byte is second high order bit (0x40) */
        if ((shift < size) && ( (byte & 0x40) !=0 /* sign bit of byte is set */))
                /* sign extend */
                result |= - (1 << shift);
        return ( position > max );

}




template <int ptrsize>
static void print_uleb_operand(
	stringstream &sout,
        uint32_t pos,
        const uint8_t* const data,
        const uint32_t max)
{
        auto uleb=uint64_t(0xdeadbeef);
        read_uleb128<ptrsize>(uleb, pos, data, max);
        sout<<" "<<dec<<uleb;
}

template <int ptrsize>
static void print_sleb_operand(
	stringstream &sout,
        uint32_t pos,
        const uint8_t* const data,
        const uint32_t max)
{
        auto leb=int64_t(0xdeadbeef);
        read_sleb128<ptrsize>(leb, pos, data, max);
        sout<<" "<<dec<<leb;
}



template <int ptrsize>
string EhWriterImpl_t<ptrsize>::EhProgramListingManip_t::getPrintableString(const string &s) const
{

	stringstream sout;
	// make sure uint8_t is an unsigned char.	
	static_assert(std::is_same<unsigned char, uint8_t>::value, "uint8_t is not unsigned char");

	auto data=s;
	auto opcode=(uint8_t)data[0];
	auto opcode_upper2=(uint8_t)(opcode >> 6);
	auto opcode_lower6=(uint8_t)(opcode & (0x3f));
	auto pos=uint32_t(1);
	auto max=data.size();

	switch(opcode_upper2)
	{
		case 1:
		{
			// case DW_CFA_advance_loc:
			sout<<"				cfa_advance_loc "<<dec<<+opcode_lower6<<" * caf";
			break;
		}
		case 2:
		{
			uint64_t uleb=0;
			sout<<"				cfa_offset "; 
			if(read_uleb128<ptrsize>(uleb, pos, (const uint8_t* const)data.data(), max))
				break;
			// case DW_CFA_offset:
			sout <<dec<<uleb;
			break;
		}
		case 3:
		{
			// case DW_CFA_restore (register #):
			sout<<"				cfa_restore";
			break;
		}
		case 0:
		{
			switch(opcode_lower6)
			{
			
				case DW_CFA_nop:
					sout<<"				nop" ;
					break;
				case DW_CFA_remember_state:
					sout<<"				remember_state" ;
					break;
				case DW_CFA_restore_state:
					sout<<"				restore_state" ;
					break;

				// takes single uleb128
				case DW_CFA_undefined:
					sout<<"				undefined" ;
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max); 
					break;
		
				case DW_CFA_same_value:
					sout<<"				same_value ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max); 
					break;
				case DW_CFA_restore_extended:
					sout<<"				restore_extended ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max); 
					break;
				case DW_CFA_def_cfa_register:
					sout<<"				def_cfa_register ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max); 
					break;
				case DW_CFA_GNU_args_size:
					sout<<"				GNU_arg_size ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max); 
					break;
				case DW_CFA_def_cfa_offset:
					sout<<"				def_cfa_offset "; 
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max); 
					break;

				case DW_CFA_set_loc:
				{
					auto arg=uintptr_t(0xDEADBEEF);
					switch(ptrsize)
					{
						case 4:
							arg=*(uint32_t*)&data.data()[pos]; break;
						case 8:
							arg=*(uint64_t*)&data.data()[pos]; break;
						default: 
							assert(0);
					}
					sout<<"				set_loc "<<hex<<arg;
					break;
				}
				case DW_CFA_advance_loc1:
				{
					auto loc=*(uint8_t*)(&data.data()[pos]);
					sout<<"				advance_loc1 "<<+loc<<" * caf " ;
					break;
				}

				case DW_CFA_advance_loc2:
				{
					auto loc=*(uint16_t*)(&data.data()[pos]);
					sout<<"				advance_loc2 "<<+loc<<" * caf " ;
					break;
				}

				case DW_CFA_advance_loc4:
				{
					auto loc=*(uint32_t*)(&data.data()[pos]);
					sout<<"				advance_loc4 "<<+loc<<" * caf " ;
					break;
				}
				case DW_CFA_offset_extended:
					sout<<"				offset_extended ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					break;
				case DW_CFA_register:
					sout<<"				register ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					break;
				case DW_CFA_def_cfa:
					sout<<"				def_cfa ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					break;
				case DW_CFA_def_cfa_sf:
					sout<<"				def_cfa_sf ";
					print_uleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					print_sleb_operand<ptrsize>(sout,pos,(const uint8_t* const)data.data(),max);
					break;

				case DW_CFA_def_cfa_expression:
				{
					auto uleb=uint64_t(0);
					sout<<"				def_cfa_expression "; 
					if(read_uleb128<ptrsize>(uleb, pos, (const uint8_t* const)data.data(), max))
						break;
					sout <<dec<<uleb;
					pos+=uleb;		// doing this old school for now, as we aren't printing the expression.
					sout <<" (not printing expression)";
					break;
				}
				case DW_CFA_expression:
				{
					auto uleb1=uint64_t(0);
					auto uleb2=uint64_t(0);
					sout<<"                              expression "; 
					if(read_uleb128<ptrsize>(uleb1, pos, (const uint8_t* const)data.data(), max))
						break;
					if(read_uleb128<ptrsize>(uleb2, pos, (const uint8_t* const)data.data(), max))
						break;
					sout<<dec<<uleb1<<" "<<uleb2;
					pos+=uleb2;
					break;
				}
				case DW_CFA_val_expression:
				{
					auto uleb1=uint64_t(0);
					auto uleb2=uint64_t(0);
					sout<<"                              val_expression "; 
					if(read_uleb128<ptrsize>(uleb1, pos, (const uint8_t* const)data.data(), max))
						break;
					if(read_uleb128<ptrsize>(uleb2, pos, (const uint8_t* const)data.data(), max))
						break;
					sout <<dec<<uleb1<<" "<<uleb2;
					pos+=uleb2;
					break;
				}
				case DW_CFA_def_cfa_offset_sf:
				{
					auto leb=int64_t(0);
					sout<<"					def_cfa_offset_sf "; 
					if(read_sleb128<ptrsize>(leb, pos, (const uint8_t* const)data.data(), max))
						break;
					sout <<dec<<leb;
					break;
				}
				case DW_CFA_offset_extended_sf:
				{
					auto uleb1=uint64_t(0);
					auto sleb2=int64_t(0);
					sout<<"                              offset_extended_sf "; 
					if(read_uleb128<ptrsize>(uleb1, pos, (const uint8_t* const)data.data(), max))
						break;
					if(read_sleb128<ptrsize>(sleb2, pos, (const uint8_t* const)data.data(), max))
						break;
					sout <<dec<<uleb1<<" "<<sleb2;
					break;
				}


				/* SGI/MIPS specific */
				case DW_CFA_MIPS_advance_loc8:

				/* GNU extensions */
				case DW_CFA_GNU_window_save:
				case DW_CFA_GNU_negative_offset_extended:
				default:
					sout<<"Unhandled opcode cannot print. opcode="<<opcode;
			}
			break;
		}
		default:
			assert(0);
	}

	return sout.str();
}

template <int ptrsize>
EhWriterImpl_t<ptrsize>::FDErepresentation_t::LSDArepresentation_t::LSDArepresentation_t(Instruction_t* insn)
	// if there are call sites, use the call site encoding.  if not, set to omit for initializer.
	//  extend/canExtend should be able to extend an omit to a non-omit.
	: tt_encoding( insn->getEhCallSite() ? insn->getEhCallSite()->getTTEncoding() : 0xff)
{
		
	extend(insn);
}



static const auto RelocsEqual=[](const Relocation_t* a, const Relocation_t* b) -> bool
{
	if(a==nullptr && b==nullptr)
		return true;
	if(a==nullptr || b==nullptr)
		return false;
	return 
		forward_as_tuple(a->getType(), a->getOffset(), a->getWRT(), a->getAddend()) == 
		forward_as_tuple(b->getType(), b->getOffset(), b->getWRT(), b->getAddend());
};

template <int ptrsize>
bool EhWriterImpl_t<ptrsize>::FDErepresentation_t::LSDArepresentation_t::canExtend(Instruction_t* insn) const
{
	if(insn->getEhCallSite() == nullptr)
		return true;

	const auto insn_tt_encoding = insn->getEhCallSite()->getTTEncoding();

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
	       ((tt_encoding&0xf)==0x0 && ptrsize==4) || 		// or is exactly DW_EH_PE_absptr on 32-bit
 	       (tt_encoding&0xf)==0xb ); 	// or encoding contains DW_EH_PE_sdata4
	const auto tt_entry_size=4;

	const auto mismatch_tt_entry = find_if(
		insn->getEhCallSite()->getRelocations().begin(),
		insn->getEhCallSite()->getRelocations().end(),
		[&](const Relocation_t* candidate_reloc)
			{
				const auto tt_index=candidate_reloc->getOffset()/tt_entry_size;
				if(tt_index>=(int64_t)type_table.size())
					return false;
				const auto &tt_entry=type_table.at(tt_index);
	
				if(tt_entry==nullptr) // entry is empty, so no conflict
					return false;
				return !RelocsEqual(candidate_reloc, tt_entry);
			}
		);

	// return true if we found no mismatches
	return (mismatch_tt_entry == insn->getEhCallSite()->getRelocations().end());
}

template <int ptrsize>
void EhWriterImpl_t<ptrsize>::FDErepresentation_t::LSDArepresentation_t::extend(Instruction_t* insn)
{
	// if there's no call site info, the LSDA doesn't need an extension.
	if(insn->getEhCallSite() == nullptr)
		return;

	const auto insn_tt_encoding = insn->getEhCallSite()->getTTEncoding();

	// FIXME: optimization possibilty:  see if the last call site in the table
	// has the same set of catch-types + landing_pad and is "close enough" to this insn.
	// if so, combine.  

	cout<<"Creating call sites in LSDA for "<<hex<<insn->getBaseID()<<":"<<insn->getDisassembly()<<endl;

	// just create a new entry in the CS table.. 
	auto cs=(call_site_t){0}; 

	cs.cs_insn_start=insn;
	cs.cs_insn_end=insn;
	cs.landing_pad=insn->getEhCallSite()->getLandingPad();

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
	for(const auto &reloc : insn->getEhCallSite()->getRelocations())
	{
		const auto wrt_scoop=dynamic_cast<DataScoop_t*>(reloc->getWRT());
		if(reloc->getWRT()==nullptr)
			cout<<"\tFound reloc: nullptr (catch all)"<<endl;
		else if(wrt_scoop)
			cout<<"\tFound reloc: scoop "<<wrt_scoop->getName()<<"+0x"<<hex<<reloc->getAddend()<<endl;
		else
			cout<<"\tFound reloc: unexpected type? "<<endl;

		// for now, this is the only supported reloc type on a EhCallSite 
		assert(reloc->getType()=="type_table_entry");
		auto tt_it=find_if(type_table.begin(),type_table.end(), 
			[reloc](const Relocation_t* candidate) { return candidate!=nullptr && RelocsEqual(candidate,reloc); });
		if(tt_it==type_table.end())
		{
			const auto tt_encoding = insn->getEhCallSite()->getTTEncoding();
			assert(
			       (tt_encoding&0xf)==0x3 ||  		// encoding contains DW_EH_PE_udata4
			       (tt_encoding&0xf)==0xb ||  		// encoding contains DW_EH_PE_sdata4
			       ((tt_encoding&0xf)==0x0 && ptrsize==4)  	// encoding contains DW_EH_PE_absptr && ptrsize==4
				);
			const auto tt_entry_size=4;
			const auto tt_index= reloc->getOffset()/tt_entry_size;
			if(tt_index>=(int64_t)type_table.size())
				type_table.resize(tt_index+1);
			assert(type_table.at(tt_index)==nullptr || RelocsEqual(type_table.at(tt_index),reloc));	
			type_table[tt_index]=reloc;	
		}
	}

	cs.actions=insn->getEhCallSite()->getTTOrderVector();

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

	return pgm.canExtend(insn->getEhProgram()->getFDEProgram()) && 
		lsda.canExtend(insn);


}

template <int ptrsize>
void EhWriterImpl_t<ptrsize>::FDErepresentation_t::extend(Instruction_t* insn, EhWriterImpl_t<ptrsize>* ehw)
{
	const auto insn_addr=ehw->zipr_obj.GetLocationMap()->at(insn);
	const auto new_end_addr=insn_addr+insn->getDataBits().size();
	const auto incr_amnt=insn_addr-last_advance_addr;
	last_advance_addr=insn_addr;

	// add appropriate instructions to the pgm.
	pgm.extend((incr_amnt)/cie->code_alignment_factor, insn->getEhProgram()->getFDEProgram());

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
	map<VirtualOffset_t,Instruction_t*> insn_in_order;
	for(const auto& this_pair : *zipr_obj.GetLocationMap())
		insn_in_order[this_pair.second]=this_pair.first;


	// build the fdes (and cies/lsdas) for this insn, starting with a null fde in case none exist
	auto current_fde=(FDErepresentation_t*)nullptr;
	auto insns_with_frame=0;

	// for_each instruction in program order
	for(const auto& this_pair : insn_in_order)
	{
		const auto &this_insn=this_pair.second;
		const auto &this_addr=this_pair.first;

		// no eh pgm or call site?  no worries, just ignore this insn
		if(this_insn->getEhProgram()==nullptr && this_insn->getEhCallSite()==nullptr)
			continue;

		insns_with_frame++;

		// if it has an unwinder and/or a call site, we will need an fde.

		// end this fde
		if(current_fde && !current_fde->canExtend(this_insn, this))
		{
			if(getenv("EH_VERBOSE")!=nullptr)
				cout<<"Ending FDE because insn "<<hex<<this_insn->getBaseID()<<":"<<this_insn->getDisassembly()<<" doesn't fit at " << this_addr<< endl;
			current_fde=nullptr;
		}


		// if we need to start a new fde, create one.
		if(current_fde==nullptr)
		{
			if(getenv("EH_VERBOSE")!=nullptr)
				cout<<"Creating new FDE for "<<hex<<this_insn->getBaseID()<<":"<<this_insn->getDisassembly()<< " at " << this_addr<<endl;
			current_fde=new FDErepresentation_t(this_insn,this);
			all_fdes.push_back(current_fde);
		}
		else
		{
			if(getenv("EH_VERBOSE")!=nullptr)
			{
				cout<<"Extending new FDE for "<<hex<<this_insn->getBaseID()<<":"<<this_insn->getDisassembly()<<" at " << this_addr <<endl;
				print_pers(this_insn,current_fde->cie);

			}
			current_fde->extend(this_insn,this);
		}
	}

	const auto avg_insn_per_fde = insns_with_frame/(float)all_fdes.size();


        assert(getenv("SELF_VALIDATE")==nullptr || all_fdes.size() > 10 ) ;
        assert(getenv("SELF_VALIDATE")==nullptr || all_cies.size() > 0 ) ;
        assert(getenv("SELF_VALIDATE")==nullptr || insns_with_frame > 10 );
        assert(getenv("SELF_VALIDATE")==nullptr ||  avg_insn_per_fde > 1 ) ;

	cout<<"# ATTRIBUTE ExceptionHandlerWrite::fdes_calculated="<<dec<<all_fdes.size()<<endl;
	cout<<"# ATTRIBUTE ExceptionHandlerWrite::cies_calculated="<<dec<<all_cies.size()<<endl;
	cout<<"# ATTRIBUTE ExceptionHandlerWrite::insns_with_eh_info="<<dec<<insns_with_frame<<endl;
	cout<<"# ATTRIBUTE ExceptionHandlerWrite::avg_insns_per_fde="<<dec<<avg_insn_per_fde<<endl;
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
			out << asm_comment << p.getPrintableString(s)<<endl;
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
					if(candidate.landing_pad==nullptr)
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
		const auto output_action=[&](const IRDB_SDK::TTOrderVector_t &act, const uint32_t act_num) -> void
		{
			const auto &ttov=act;
			const auto biggest_ttov_index=ttov.size()-1;
			auto act_entry_num=biggest_ttov_index;

			for(int i=act_entry_num; i>=0; i--)
			{
				out<<"LSDA"<<dec<<lsda_num<<"_act"<<act_num<<"_start_entry"<<act_entry_num<<""<<":"<<endl;
				out<<"	.sleb128 "<<dec<<ttov.at(act_entry_num)<<endl;        
				if(act_entry_num==biggest_ttov_index)
					out<<"	.sleb128 0 "<<endl;
				else
					out<<"	.sleb128  LSDA"<<lsda_num<<"_act"<<act_num<<"_start_entry"<<act_entry_num+1<<" - . "<<endl;
				act_entry_num--;
			}
		};


		const auto output_callsite=[&](const typename FDErepresentation_t::LSDArepresentation_t::call_site_t &cs, const uint32_t cs_num) -> void
		{
			const auto cs_start_addr=zipr_obj.GetLocationMap()->at(cs.cs_insn_start);
			const auto cs_end_addr=zipr_obj.GetLocationMap()->at(cs.cs_insn_start)+cs.cs_insn_start->getDataBits().size();
			const auto cs_len=cs_end_addr-cs_start_addr;
			out<<"LSDA"<<dec<<lsda_num<<"_cs_tab_entry"<<cs_num<<"_start:"<<endl;
        		out<< asm_comment << "	1) start of call site relative to FDE start addr (call site encoding)"<<endl;
        		out<<"	.sleb128 0x"<<hex<<cs_start_addr<<" - 0x"<<hex<<fde->start_addr<<endl;
        		out<<asm_comment << "   2) length of call site (call site encoding)"<<endl;
        		out<<"	.sleb128 "<<dec<<cs_len<<endl;
			if(cs.landing_pad)
			{
				const auto lp_addr=zipr_obj.GetLocationMap()->at(cs.landing_pad);
				out<<asm_comment<<"   3) the landing pad, or 0 if none exists. (call site encoding)"<<endl;
				out<<"	.sleb128 0x"<<hex<<lp_addr<<" - 0x"<<hex<<landing_pad_base<<endl;
			}
			else
			{
				out<<asm_comment<<"   3) the landing pad, or 0 if none exists. (call site encoding)"<<endl;
				out<<"	.sleb128 0"<<endl;
			}
			if(cs.actions.size() > 0 )
			{
				out<<asm_comment<<"   4) index into action table + 1 -- 0 indicates unwind only (call site encoding)"<<endl;
				out<<"	.sleb128 1 + LSDA"<<dec<<lsda_num<<"_act"
				   <<cs.action_table_index<<"_start_entry0 - LSDA"<<dec<<lsda_num<<"_action_tab_start"<<endl;
			}
			else
			{
				out<<asm_comment<<"   4) index into action table + 1 -- 0 indicates unwind only (always uleb)"<<endl;
				out<<"	.uleb128 0 // no actions!" << endl;
			}
			out<<"LSDA"<<dec<<lsda_num<<"_cs_tab_entry"<<cs_num<<"_end:"<<endl;

		};

		const auto output_lsda_header=[&]()
		{
			if(landing_pad_base==fde->start_addr)
			{
				out<<asm_comment<<"   1) encoding of next field "<<endl;
				out<<"        .byte 0xff "<<asm_comment<<" DW_EH_PE_omit (0xff)"<<endl;
				out<<""<<endl;
				out<<asm_comment<<"   2) landing pad base, if omitted, use FDE start addr"<<endl;
				out<<asm_comment<<"   .<fdebasetype> <fdebase> -- omitted.  "<<endl;
			}
			else
			{
				out<<asm_comment<<"   1) encoding of next field "<<endl;
				out<<"        .byte 0x1b "<<asm_comment<<" DW_EH_PE_pcrel (0x10) |sdata4 (0xb)"<<endl;
				out<<""<<endl;
				out<<"        "<<asm_comment<<" 2) landing pad base, if omitted, use FDE start addr"<<endl;
				out<<"        .int  0x"<<hex<<landing_pad_base<<" + eh_frame_hdr_start - . - "<<dec<<eh_frame_hdr_addr<<" "<<asm_comment<<" as pcrel|sdata4 .  "<<endl;
			}
			out<<""<<endl;
			out<<asm_comment<<"   3) encoding of type table entries"<<endl;
			out<<"        .byte 0x"<<hex<<lsda->tt_encoding<<"  "<<asm_comment<<" DW_EH_PE_udata4"<<endl;
			out<<""<<endl;
			out<<asm_comment<<"   4) type table pointer -- always a uleb128"<<endl;
			if(lsda->tt_encoding==0xff) /* omit */
			{
				out<<asm_comment<<"   .uleb128 LSDAptr omitted"<< endl;
			}
			else
			{
				out<<"        .uleb128 LSDA"<<dec<<lsda_num<<"_type_table_end - LSDA"<<lsda_num<<"_tt_ptr_end"<<endl;
			}
			out<<"LSDA"<<dec<<lsda_num<<"_tt_ptr_end:"<<endl;
			out<<""<<endl;
			out<<asm_comment<<"   5) call site table encoding"<<endl;
			out<<"        .byte 0x9 "<<asm_comment<<" DW_EH_PE_sleb128 "<<endl;
			out<<""<<endl;
			out<<asm_comment<<"   6) the length of the call site table"<<endl;
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
				if(reloc==nullptr)
				{
					// indicates a catch all or empty type table entry
					out<<"	.int 0x0 "<<asm_comment<<" not used!"<<endl;
				}
				else if(reloc->getWRT()==nullptr)
				{
					// indicates a catch all or empty type table entry
					out<<"	.int 0x0 "<<asm_comment<<" catch all "<<endl;
				}
				else
				{
					// indicates a catch of a paritcular type
					const auto scoop=dynamic_cast<DataScoop_t*>(reloc->getWRT());
					assert(scoop);
					const auto final_addr=scoop->getStart()->getVirtualOffset() + reloc->getAddend();
					if(((lsda->tt_encoding)&0x10) == 0x10) // if encoding contains pcrel (0x10).
						out<<"	.int 0x"<<hex<<final_addr<<" + eh_frame_hdr_start - . -  "<<dec<<eh_frame_hdr_addr<<endl;
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

		const auto personality_scoop=cie->personality_reloc ? dynamic_cast<DataScoop_t*>  (cie->personality_reloc->getWRT()) : (DataScoop_t*)nullptr;
		const auto personality_insn =cie->personality_reloc ? dynamic_cast<Instruction_t*>(cie->personality_reloc->getWRT()) : (Instruction_t*)nullptr;
		const auto personality_addend=cie->personality_reloc ? cie->personality_reloc->getAddend() : 0;

		const auto cie_pos=cie_pos_it-all_cies.begin();

		cie->has_been_output=true;
		out<<asm_comment<<" cie "<<dec<<cie_pos<<""<<endl;
		out<<"Lcie"<<cie_pos<<":"<<endl;
		out<<"	 .int Lcie"<<cie_pos<<"_end - Lcie"<<cie_pos<<" - 4 "<<asm_comment<<" length of this record. -4 because length doesn't include this field"<<endl;
		out<<"        .int 0                  "<<asm_comment<<" cie (not fde)"<<endl;
		out<<"        .byte 3                 "<<asm_comment<<" version"<<endl;
		out<<"        .asciz \"zPLR\"           "<<asm_comment<<" aug string."<<endl;
		out<<"        .uleb128 "<<dec<<cie->code_alignment_factor<<"              "<<asm_comment<<" code alignment factor"<<endl;
		out<<"        .sleb128 "<<dec<<cie->data_alignment_factor<<"             "<<asm_comment<<" data alignment factor"<<endl;
		out<<"        .uleb128 "<<dec<<cie->return_reg<<"             "<<asm_comment<<" return address reg."<<endl;
		out<<"        "<<asm_comment<<" encode the Z (length)"<<endl;
		out<<"        .sleb128 Lcie"<<cie_pos<<"_aug_data_end-Lcie"<<cie_pos<<"_aug_data_start "<<asm_comment<<" Z -- handle length field"<<endl;
		out<<"Lcie"<<cie_pos<<"_aug_data_start:"<<endl;
		out<<""<<endl;
		if(personality_scoop)
		{
			auto personality_value=personality_scoop->getStart()->getVirtualOffset()+personality_addend;
			out<<"        "<<asm_comment<<"encode the P (personality encoding + personality routine)"<<endl;
			out<<"        .byte 0x80 | 0x10 | 0x0B        "<<asm_comment<<"  personality pointer encoding DH_EH_PE_indirect (0x80) | pcrel | sdata4"<<endl;
			out<<"        .int "<<personality_value<<" + eh_frame_hdr_start - . - "<<dec<<eh_frame_hdr_addr<<" "<<asm_comment<<" actual personality routine, encoded as noted in prev line."<<endl;
		}
		else if(personality_insn)
		{
			const auto personality_insn_addr=zipr_obj.GetLocationMap()->at(personality_insn);
			const auto personality_value=personality_insn_addr+personality_addend;
			out<<"        "<<asm_comment<<"encode the P (personality encoding + personality routine)"<<endl;
			out<<"        .byte 0x10 | 0x0B        "<<asm_comment<<"  personality pointer encoding pcrel | sdata4"<<endl;
			out<<"        .int "<<personality_value<<" + eh_frame_hdr_start - . - "<<dec<<eh_frame_hdr_addr<<" "<<asm_comment<<" actual personality routine, encoded as noted in prev line."<<endl;
		}
		else
		{
			assert(cie->personality_reloc==nullptr || cie->personality_reloc->getWRT()==nullptr);
			out<<"        "<<asm_comment<<"encode the P (personality encoding + personality routine)"<<endl;
			out<<"        .byte  0x0B        "<<asm_comment<<"  personality pointer encoding sdata4"<<endl;
			out<<"        .int 0               "<<asm_comment<<" actual personality routine, encoded as noted in prev line."<<endl;
		}
		out<<""<<endl;
		out<<"        "<<asm_comment<<" encode L (lsda encoding) "<<endl;
		out<<"        .byte  0x1b     "<<asm_comment<<" LSDA encoding (pcrel|sdata4)"<<endl;
		out<<""<<endl;
		out<<"        "<<asm_comment<<" encode R (FDE encoding) "<<endl;
		out<<"        .byte  0x10 | 0x0B      "<<asm_comment<<" FDE encoding (pcrel | sdata4)"<<endl;
		out<<"Lcie"<<cie_pos<<"_aug_data_end:"<<endl;
		out<<"       "<<asm_comment<<" CIE program"<<endl;
		output_program(cie->pgm,out);
		out<<""<<endl;
		out<<"       "<<asm_comment<<" pad with nops"<<endl;
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

		out<<""<<asm_comment<<"fde "<<dec<<fde_num<<""<<endl;
		out<<"Lfde"<<fde_num<<":"<<endl;
		out<<"        .int Lfde"<<fde_num<<"_end - Lfde"<<fde_num<<" - 4      "<<asm_comment<<" length of this record. -4 because "
		     "length doesn't include this field."<<endl;
		out<<"        .int . - Lcie"<<cie_pos<<"                  "<<asm_comment<<" this is an FDE (not a "
		     "cie), and it's cie is CIE"<<cie_pos<<".  byte offset from start of field."<<endl;
		out<<"        .int 0x"<<hex<<fde->start_addr<<dec<<" + eh_frame_hdr_start - . - "<<dec<<eh_frame_hdr_addr<<" "<<asm_comment<<" FDE start addr"<<endl;
		out<<"        .int "<<dec<<fde->end_addr-fde->start_addr<<"                     "<<asm_comment<<" fde range length (i.e., can calc the "
		     "fde_end_addr from this -- note that pcrel is ignored here!)"<<endl;
		out<<"        "<<asm_comment<<"encode Z (length)"<<endl;
		out<<"        .uleb128 Lfde"<<fde_num<<"_aug_data_end-Lfde"<<fde_num<<"_aug_data_start"<<endl;
		out<<"Lfde"<<fde_num<<"_aug_data_start:"<<endl;
		out<<"        "<<asm_comment<<"encode L (LSDA) "<<endl;
		if(fde->hasLsda())
			out<<"        .int LSDA"<<fde_num<<" - .    "<<asm_comment<<" LSDA hard coded here (as pcrel+sdata4)"<<endl;	 
		else
			out<<"        .int 0 + eh_frame_hdr_start - . - "<<dec<<eh_frame_hdr_addr<<"      "<<asm_comment<<" no LSDA, encoded with pcrel "<<endl;	 
		out<<"Lfde"<<fde_num<<"_aug_data_end:"<<endl;
		out<<""<<endl;
		out<<"        "<<asm_comment<<" FDE"<<fde_num<<" program"<<endl;
		output_program(fde->pgm,out);
		out<<"        .align 4, 0"<<endl;
		out<<"        Lfde"<<fde_num<<"_end:"<<endl;

	};
	auto generate_eh_frame_hdr=[&](ostream& out) -> void	
	{
		out<<".section eh_frame_hdr, \"a\", @progbits"<<endl;
		out<<"eh_frame_hdr_start:"<<endl;
		out<<"        .byte 1                 "<<asm_comment<<" version"<<endl;
		out<<"        .byte 0x10 | 0x0B       "<<asm_comment<<" encoding for pointer to eh-frame -- DH_EH_PE_pcrel (0x10) | DH_EH_PE_sdata4 (0x0B)"<<endl;
		out<<"        .byte 0x03              "<<asm_comment<<" encoding for ; of entries in eh-frame-hdr  -- BDH_EH_PE_udata4 (0x03)"<<endl;
		out<<"        .byte 0x30 | 0x0B       "<<asm_comment<<" encoding for pointers (to fdes) held in the eh-frame-hdr header  "
		     "-- DH_EH_PE_datarel (0x30) | DH_EH_PE_sdata4 (0x0b) " <<endl;

		out<<"        .int Lfde_table - .     "<<asm_comment<<" pointer to fde_table, encoded as an sdata4, pcrel"<<endl;
		out<<"        .int (eh_frame_table_end-eh_frame_table)/8     "<<asm_comment<<" number of FDEs in the header."<<endl;

// on some archs, this line causes an additional alignment, as the assembler feels free to align as much as you want.
// but the eh-parser is expecting the table _right here_, so we do the alignment manually by making sure the above part
// has a multiple-of-4 bytes.
//		out<<"        .align 4"<<endl;
		out<<"eh_frame_table:"<<endl;
		out<<"        "<<asm_comment<<" fde pointers"<<endl;

		for(auto fde_num=0U; fde_num < all_fdes.size(); fde_num++)
		{
			const auto& fde=all_fdes[fde_num];
			out<<"        .int 0x"<<hex<<fde->start_addr<<" - "<<dec<<eh_frame_hdr_addr<<endl;
			out<<"        .int Lfde"<<dec<<fde_num<<" - eh_frame_hdr_start"<<endl;
		}

		out<<"eh_frame_table_end:"<<endl;

	};
	auto generate_eh_frame=[&](ostream& out) -> void	
	{
		out<<".section eh_frame, \"a\", @progbits"<<endl;
		out<<"Lfde_table: "<<asm_comment<<" needed for xref to eh_frame_hdr" <<endl;

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

	const auto eh_frame_hdr_addr_str=to_hex_string(eh_frame_hdr_addr);

	// create and execute the command to build the ehframe.
	auto cmd=(string)"$PEASOUP_HOME/tools/eh_frame_tools/eh_to_bin.sh "+ehframe_s_filename+" "+eh_frame_hdr_addr_str+" "+ehframe_exe_filename;
	cout<<"Running: "<<cmd<<endl;
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
        EXEIO::exeio ehframe_exe_rep;
	ehframe_exe_rep.load(ehframe_exe_filename);

	auto to_scoop=[&](const string &secname)->void
	{
		const auto &sec=ehframe_exe_rep.sections[secname];

		// if sec is missing, don't scoopify.

		if(sec==nullptr) return;
		const auto data      = string(sec->get_data(), sec->get_size());
		const auto start_vo  = sec->get_address();
		const auto start_addr= zipr_obj.getFileIR()->addNewAddress(BaseObj_t::NOT_IN_DATABASE, start_vo);
		const auto end_vo    = sec->get_address()+sec->get_size()-1;
		const auto end_addr  = zipr_obj.getFileIR()->addNewAddress(BaseObj_t::NOT_IN_DATABASE, end_vo);
		const auto new_scoop = zipr_obj.getFileIR()->addNewDataScoop(secname, start_addr,end_addr,nullptr,4,false,data);
		(void)new_scoop;

		//zipr_obj.getFileIR()->getAddresses().insert(start_addr);
		//zipr_obj.getFileIR()->getAddresses().insert(end_addr);
		//zipr_obj.getFileIR()->getDataScoops().insert(new_scoop);
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
