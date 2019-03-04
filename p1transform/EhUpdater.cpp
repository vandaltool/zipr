
#include "EhUpdater.hpp"
#include <string>
#include <map>

using namespace std;
using namespace IRDB_SDK;

extern map<Function_t*, set<Instruction_t*> > inserted_instr; //used to undo inserted instructions


// see https://en.wikipedia.org/wiki/LEB128
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
	return ( position >= max );

}

static string to_uleb128 (uint64_t value)
{
	auto output_str=string("");
	do 
	{
		auto byte = value&0x7f; // low order 7 bits of value;
		value >>= 7;
		if (value != 0) // more bytes to come 
			byte |= 0x80; // set high order bit of byte;

		output_str.push_back(byte);	

	} while (value != 0);

	return output_str;
}

/* transform any eh handling info for the FDE program*/
bool EhUpdater_t::update_program(EhProgram_t* ehpgm)
{
	assert(ehpgm);
	const auto daf=ehpgm->getDataAlignmentFactor();
	const auto saved_reg_size=m_layout->GetSavedRegsSize();
	const auto orig_frame_size=m_layout->GetOriginalAllocSize();
	const auto altered_frame_size=m_layout->GetAlteredAllocSize();

	/* change the offset, as needed, in a dwarf instruction.  the offset is at location 'pos' */
	const auto change_offset=[&](string &dwarf_insn, const uint32_t offset_pos, const bool factored) -> void
	{
		/* handle */
		/* format: [(char)(opcode+reg#)]    [(uleb128) offset/data_alignment_factor] */
		/* we need to adjust factored offset if it's greater than the saved reg size */
		auto factored_offset=uint64_t(0);
		auto pos_to_read=(uint32_t)1;
		const auto data=reinterpret_cast<const uint8_t*>(dwarf_insn.data());
		const auto res=read_uleb128(factored_offset,pos_to_read,data, dwarf_insn.size());
		assert(res);
		auto offset=factored_offset;
		if(factored)
			offset*=daf;
		if(offset>saved_reg_size)
		{
			const auto new_offset=offset+(altered_frame_size-orig_frame_size);
			auto factored_new_offset=new_offset;
			if(factored)
				factored_new_offset/=daf;

			const auto  encoded_factored_new_offset=to_uleb128(factored_new_offset);
			auto new_dwarf_insn=string("");
			for(auto i=0U;i<offset_pos;i++)
				new_dwarf_insn.push_back(dwarf_insn[i]);
			new_dwarf_insn+=encoded_factored_new_offset;
			dwarf_insn=new_dwarf_insn;
		}
	};
	auto tmppgm = ehpgm->getFDEProgram();
	for(auto &dwarf_insn :  tmppgm)
	{
		auto opcode=dwarf_insn[0];
		auto opcode_upper2=(uint8_t)(opcode >> 6);
		auto opcode_lower6=(uint8_t)(opcode & (0x3f));
		switch(opcode_upper2)
		{
			/* case DW_CFA_offset: */
			/* reg should be restored from CFA+(offset*daf) */
			case 0x2: /* DW_CFA_offset: */
			{
				change_offset(dwarf_insn,1, true);
				break;
			};
			case 0:
			{
				switch(opcode_lower6)
				{
					/* sanitize */
					case 0xd: /* DW_CFA_def_cfa_register: */
					{
						/* [ (char)opcode ] [ (uleb)register ] */
						/* assert if register != sp (if bp not used) or bp (if bp is used) */
						/* never update this insn */
						assert(0);
					}
					/* handle */
					case 0xe: /* DW_CFA_def_cfa_offset: */ 
					{
						/* [(char)opcode] [(uleb)offset] */
						/* if offset > saved reg size, new_offset=offset+(new_frame_size-old_frame_size) */
						change_offset(dwarf_insn,1,false);
						break;
					}
					case 0x11: /*DW_CFA_def_cfa_offset_sf: */
					{
						/* [(char)opcode] [(sleb)offset/data_alignment_factor] */
						/* if offset > saved reg size, new_offset=offset+(new_frame_size-old_frame_size) */
						assert(0);
					}
					
					case 0x5: /*DW_CFA_offset_extended: */
					{
						/* [ (char)opcode ] [ (uleb)reg # ]  [ (uleb) offset]  */
						/* we need to adjust factored offset if it's greater than the saved reg size */
						change_offset(dwarf_insn,2,true);
						break;
					}

					default: 
						break;
				}
			}
			default: 	
				break;
		}
	}
	ehpgm->setFDEProgram(tmppgm);
	return true;
}

/* transform any eh handling info for the instruction */
bool EhUpdater_t::update_instructions(Instruction_t* insn)
{
	const auto ehpgm=insn->getEhProgram();

	/* no program == no update */
	if(ehpgm==NULL)
		return true;

	const auto new_eh_pgm=m_firp->copyEhProgram(*ehpgm);
	// const auto new_eh_pgm=new EhProgram_t(*ehpgm);
	// m_firp->GetAllEhPrograms().insert(new_eh_pgm);
	insn->setEhProgram(new_eh_pgm);

	return update_program(new_eh_pgm);
}

/* transform any eh handling info for each instruction */
bool EhUpdater_t::update_instructions()
{
	// for all instructions in the fuunction.
	for(const auto& i: m_func->getInstructions())
		update_instructions(i);
	// plus any instructions we allocated for the function
// inserted instrucctions are now part of m_func->getInstructions() -- were we updating this frame twice?
//	for(const auto& i: inserted_instr[m_func])
//		update_instructions(i);

	return true;
}


/* transform any eh handling info for the function */
bool EhUpdater_t::execute()
{
	if(m_func->getUseFramePointer()) /* no updates needed if a frame pointer is used */
		return true;

	/* can only update for p1 functions */
	if( ! m_layout->IsP1() )
		return false;

	return update_instructions();
}

