
/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <irdb-core>
#include <irdb-util>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <algorithm>
#include <numeric>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <elf.h>
#include <cctype>

#include <exeio.h>
#include "check_thunks.hpp"
#include "fill_in_indtargs.hpp"
#include "libMEDSAnnotation.h"
#include "back_search.hpp"

using namespace IRDB_SDK;
using namespace std;
using namespace EXEIO;
using namespace MEDS_Annotation;

/* 
 * Constants
 */
const auto BINARY_NAME=string("a.ncexe");
const auto SHARED_OBJECTS_DIR=string("shared_objects");


/*
 * defines 
 */
#define ALLOF(a) begin(a),end(a)


//
// Compute power of two greater than or equal to `n`
//
static inline uint64_t findNextPowerOf2(uint64_t n)
{
    // decrement `n` (to handle the case when `n` itself is a power of 2)
    n--;

    // set all bits after the last set bit
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    // increment `n` and return
    return ++n;
}


/*
 * Return the x86-64, 64-bit register corresponding to the regno passed in
 */
static inline string regNoToX8664Reg(int regno) 
{
	switch(regno)
	{
		case 0/*REG0*/: return "rax"; 
		case 1/*REG1*/: return "rcx"; 
		case 2/*REG2*/: return "rdx"; 
		case 3/*REG3*/: return "rbx"; 
		case 4/*REG4*/: return "rsp"; 
		case 5/*REG5*/: return "rbp"; 
		case 6/*REG6*/: return "rsi"; 
		case 7/*REG7*/: return "rdi"; 
		case 8/*REG8*/: return "r8"; 
		case 9/*REG9*/: return "r9"; 
		case 10/*REG10*/: return "r10"; 
		case 11/*REG11*/: return "r11"; 
		case 12/*REG12*/: return "r12"; 
		case 13/*REG13*/: return "r13"; 
		case 14/*REG14*/: return "r14"; 
		case 15/*REG15*/: return "r15"; 
		default: 
			// no base register;
			return "";
	}
}

/*
 * Return the x86-32/64, 32-bit register corresponding to the regno passed in
 */
static inline string regNoToX8632Reg(int regno) 
{
	switch(regno)
	{
		case 0/*REG0*/: return "eax"; 
		case 1/*REG1*/: return "ecx"; 
		case 2/*REG2*/: return "edx"; 
		case 3/*REG3*/: return "ebx"; 
		case 4/*REG4*/: return "esp"; 
		case 5/*REG5*/: return "ebp"; 
		case 6/*REG6*/: return "esi"; 
		case 7/*REG7*/: return "edi"; 
		case 8/*REG8*/: return "r8w"; 
		case 9/*REG9*/: return "r9w"; 
		case 10/*REG10*/: return "r10w"; 
		case 11/*REG11*/: return "r11w"; 
		case 12/*REG12*/: return "r12w"; 
		case 13/*REG13*/: return "r13w"; 
		case 14/*REG14*/: return "r14w"; 
		case 15/*REG15*/: return "r15w"; 
		default: 
			// no base register;
			return "";
	}
}

extern void read_ehframe(FileIR_t* firp, EXEIO::exeio* );

template<typename T>
static inline T cptrtoh(FileIR_t* firp, const uint8_t* cptr)
{
	const auto ptrsize = firp->getArchitectureBitWidth() / 8 ;
	const auto mt      = firp->getArchitecture()->getMachineType();

	switch(ptrsize)
	{
		case 4:
		{
			const auto raw_const = *reinterpret_cast<const uint64_t*>(cptr);
			switch(mt)
			{
				case admtArm32:
				case admtAarch64:
				case admtI386:
				case admtX86_64:
					return le32toh(raw_const);
				case admtMips32:
					return be32toh(raw_const);

				default:
					throw invalid_argument("Cannot detect machine type");
			}
			assert(0);
		}
		case 8:
		{
			const auto raw_const = *reinterpret_cast<const uint32_t*>(cptr);
			switch(mt)
			{
				case admtArm32:
				case admtAarch64:
				case admtI386:
				case admtX86_64:
					return le64toh(raw_const);
				case admtMips64:
					return be64toh(raw_const);

				default:
					throw invalid_argument("Cannot detect machine type");
			}
			assert(0);
			break;
		}
		default:
		{
			throw invalid_argument("Cannot detect pointer size");
		}
	}


}

/* 
 * Endian convert a integer type to the machine's type.
 */
template<typename T>
static inline T targetToHost(FileIR_t* firp, const T cptr)
{
	const auto intSize = sizeof(T);
	const auto mt      = firp->getArchitecture()->getMachineType();

	switch(intSize)
	{
		case 1:
		{
			return cptr;
		}
		case 2:
		{
			switch(mt)
			{
				case admtArm32:
				case admtAarch64:
				case admtI386:
				case admtX86_64:
					return le16toh(cptr);
				case admtMips32:
					return be16toh(cptr);

				default:
					throw invalid_argument("Cannot detect machine type");
			}
			assert(0);
		}
		case 4:
		{
			switch(mt)
			{
				case admtArm32:
				case admtAarch64:
				case admtI386:
				case admtX86_64:
					return le32toh(cptr);
				case admtMips32:
					return be32toh(cptr);

				default:
					throw invalid_argument("Cannot detect machine type");
			}
			assert(0);
		}
		case 8:
		{
			switch(mt)
			{
				case admtArm32:
				case admtAarch64:
				case admtI386:
				case admtX86_64:
					return le64toh(cptr);
				case admtMips64:
					return be64toh(cptr);

				default:
					throw invalid_argument("Cannot detect machine type");
			}
			assert(0);
			break;
		}
		default:
		{
			throw invalid_argument("Cannot detect size of integer to endian convert");
		}
	}
}



class PopulateIndTargs_t : public TransformStep_t
{

	// record all full addresses and page-addresses found per function (or null for no function
	using PerFuncAddrSet_t = set<VirtualOffset_t>;
	map<Function_t *, PerFuncAddrSet_t> all_adrp_results;
	map<Function_t *, PerFuncAddrSet_t> all_add_adrp_results;

	// record all full addresses and page-addresses found that are spilled to the stack
	using SpillPoint_t = pair<Function_t *, VirtualOffset_t>;
	map<SpillPoint_t, PerFuncAddrSet_t> spilled_add_adrp_results;
	map<SpillPoint_t, PerFuncAddrSet_t> spilled_adrps;

	// record all full addresses found that are spilled to to a floating-point register (e.g., D10)
	using DregSpillPoint_t = pair<Function_t *, string>;
	map<DregSpillPoint_t, PerFuncAddrSet_t> spilled_to_dreg;

	map<string, PerFuncAddrSet_t> per_reg_add_adrp_results;

	set<VirtualOffset_t> direct_addresses;

	FileIR_t* m_firp = nullptr;

public:
	/*
	 * class variables
	 */

	// the bounds of the executable sections in the pgm.
	set<pair<VirtualOffset_t, VirtualOffset_t>> bounds;

	// the set of (possible) targets we've found.
	map<VirtualOffset_t, ibt_provenance_t> targets;

	// the set of ranges represented by the eh_frame section, could be empty for non-elf files.
	set<pair<VirtualOffset_t, VirtualOffset_t>> ranges;

	// keep track of jmp tables
	using JmpTable_t = map<Instruction_t *, fii_icfs> ;
	JmpTable_t jmptables;


	// a map of virtual offset -> instruction for quick access.
	map<VirtualOffset_t, Instruction_t *> lookupInstructionMap;

	// the set of things that are partially unpinned already.
	set<Instruction_t *> already_unpinned;

	long total_unpins = 0;

	// stats for unpinning arm32 switch table entries.
	int type3_unpins = 0;
	int type3_pins = 0;

	// stats for unpinning x86-64 pic switch table entries.
	int type4_unpins = 0;
	int type4_pins = 0;
	map<string, int> type4_missed_unpin_reasons;

	/*
	 * find all pc-rel operands and record the address they directly access.
	 */
	void init_direct_addresses() 
	{
		const auto isVerbose = (getenv("IB_VERBOSE") != nullptr);
		for(auto insn: m_firp->getInstructions())
		{
			const auto dis = DecodedInstruction_t::factory(insn);
			for(auto operand : dis->getOperands())
			{
					if(operand->isMemory() && operand->isPcrel())
					{
						const auto displ = operand->getMemoryDisplacement();
						const auto generated_address =
							displ + 
							insn->getAddress()->getVirtualOffset() +
							insn->getDataBits().length();

						if(llabs(displ) < 16)
						{
							if(isVerbose) 
							{
							cout << "Skipping direct address (" << hex << generated_address 
						 	     << ") because small constant in " << displ
							     << ":" << insn->getDisassembly()
							     << '\n';
							}
							continue;

						}
						if(isVerbose) 
						{
							cout << "Found direct address " << hex << generated_address 
						 	     << " in " << insn->getAddress()->getVirtualOffset() 
							     << ":" << insn->getDisassembly()
							     << '\n';
						}

						direct_addresses.insert(generated_address);
					}
			}

		}
	
	}

	/*
	 * Convert a reg id to a lower-case string
	 */
	string registerToSearchString(const RegisterID_t &reg)
	{
		auto str = registerToString(reg);
		transform(ALLOF(str), begin(str), ::tolower);
		return str;
	}
	void range(VirtualOffset_t start, VirtualOffset_t end)
	{
		pair<VirtualOffset_t, VirtualOffset_t> foo(start, end);
		ranges.insert(foo);
	}

	/*
	 * is_in_range - determine if an address is referenced by the eh_frame section
	 */
	bool is_in_range(VirtualOffset_t p)
	{
		for (auto bound : ranges)
		{
			auto start = bound.first;
			auto end = bound.second;
			if (start <= p && p <= end)
				return true;
		}
		return false;
	}

	/*
	 * process_range -  do nothing now -- fix calls deals with this.
	 */
	void process_ranges(FileIR_t *firp)
	{
	}

	bool possible_target(VirtualOffset_t p, VirtualOffset_t from_addr, ibt_provenance_t prov)
	{
		if (is_possible_target(p, from_addr))
		{
			if (getenv("IB_VERBOSE") != nullptr)
			{
				if (from_addr != 0)
					cout << "Found IB target address 0x" << std::hex << p << " at 0x" << from_addr << std::dec << ", prov=" << prov << endl;
				else
					cout << "Found IB target address 0x" << std::hex << p << " from unknown location, prov=" << prov << endl;
			}
			targets[p].add(prov);
			return true;
		}
		return false;
	}

	bool is_possible_target(VirtualOffset_t p, VirtualOffset_t addr)
	{
		for (auto bound : bounds)
		{
			auto start = bound.first;
			auto end = bound.second;
			if (start <= p && p <= end)
			{
				return true;
			}
		}
		return false;
	}

	EXEIO::section *find_section(VirtualOffset_t addr, EXEIO::exeio *exeiop)
	{
		for (int i = 0; i < exeiop->sections.size(); ++i)
		{
			EXEIO::section *pSec = exeiop->sections[i];
			assert(pSec);
			if (pSec->get_address() > addr)
				continue;
			if (addr >= pSec->get_address() + pSec->get_size())
				continue;

			return pSec;
		}
		return nullptr;
	}

	void handle_argument(
		const DecodedInstruction_t &decoded_insn,
		const DecodedOperand_t &arg,
		Instruction_t *insn,
		ibt_provenance_t::provtype_t pt = ibt_provenance_t::ibtp_text)
	{
		if (arg.isMemory() && decoded_insn.getMnemonic() == "lea")
		{
			if (arg.isPcrel())
			{
				assert(insn);
				assert(insn->getAddress());
				possible_target(arg.getMemoryDisplacement() + insn->getAddress()->getVirtualOffset() +
									insn->getDataBits().length(),
								insn->getAddress()->getVirtualOffset(), pt);
			}
			else
			{
				possible_target(arg.getMemoryDisplacement(), insn->getAddress()->getVirtualOffset(), pt);
			}
		}
	}

	void lookupInstruction_init(FileIR_t *firp)
	{
		lookupInstructionMap.clear();
		for (auto insn : firp->getInstructions())
		{
			const auto addr = insn->getAddress()->getVirtualOffset();
			lookupInstructionMap[addr] = insn;
		}
	}

	Instruction_t *lookupInstruction(VirtualOffset_t virtual_offset)
	{
		if (lookupInstructionMap.find(virtual_offset) != lookupInstructionMap.end())
			return lookupInstructionMap[virtual_offset];
		return nullptr;
	}

	void mark_targets(FileIR_t *firp)
	{
		for (auto insn : firp->getInstructions())
		{
			const auto addr = insn->getAddress()->getVirtualOffset();

			/* lookup in the list of targets */
			if (targets.find(addr) != targets.end())
			{
				const auto isret = targets[addr].areOnlyTheseSet(ibt_provenance_t::ibtp_ret);
				const auto isprintf = targets[addr].areOnlyTheseSet(ibt_provenance_t::ibtp_stars_data | ibt_provenance_t::ibtp_texttoprintf) &&
									  targets[addr].isFullySet(ibt_provenance_t::ibtp_stars_data | ibt_provenance_t::ibtp_texttoprintf);
				if (isret)
				{
					if (getenv("IB_VERBOSE") != nullptr)
						cout << "Skipping pin for ret at " << hex << addr << endl;
				}
				else if (isprintf)
				{
					if (getenv("IB_VERBOSE") != nullptr)
						cout << "Skipping pin for text to printf at " << hex << addr << endl;
				}
				else if (firp->findScoop(addr))
				{
					if (getenv("IB_VERBOSE") != nullptr)
						cout << "Skipping pin data_in_text " << hex << addr << endl;
				}
				else
				{
					if (getenv("IB_VERBOSE") != nullptr)
						cout << "Setting pin at " << hex << addr << endl;
					auto newaddr = firp->addNewAddress(insn->getAddress()->getFileID(), insn->getAddress()->getVirtualOffset());
					insn->setIndirectBranchTargetAddress(newaddr);
				}
			}
		}
	}

	bool CallToPrintfFollows(FileIR_t *firp, Instruction_t *insn, const string &arg_str)
	{
		for (auto ptr = insn->getFallthrough(); ptr != nullptr; ptr = ptr->getFallthrough())
		{
			auto d = DecodedInstruction_t ::factory(ptr);
			if (d->getMnemonic() == string("call"))
			{
				// check we have a target
				if (ptr->getTarget() == nullptr)
					return false;

				// check the target has a function
				if (ptr->getTarget()->getFunction() == nullptr)
					return false;

				// check if we're calling printf.
				if (ptr->getTarget()->getFunction()->getName().find("printf") == string::npos)
					return false;

				// found it
				return true;
			}

			// found reference to argstring, assume it's a write and exit
			if (d->getDisassembly().find(arg_str) != string::npos)
				return false;
		}

		return false;
	}

	bool texttoprintf(FileIR_t *firp, Instruction_t *insn)
	{
		string dst = "";
		// note that dst is an output parameter of IsParameterWrite and an input parameter to CallFollows
		if (isParameterWrite(firp, insn, dst) && CallToPrintfFollows(firp, insn, dst))
		{
			return true;
		}
		return false;
	}

	void get_instruction_targets(FileIR_t *firp, EXEIO::exeio *exeiop, const set<VirtualOffset_t> &thunk_bases)
	{

		const auto origInsns = firp->getInstructions();
		for (auto insn : origInsns)
		{
			auto disasm = DecodedInstruction_t::factory(insn);
			VirtualOffset_t instr_len = disasm->length(); // Disassemble(insn,disasm);

			assert(instr_len == insn->getDataBits().size());

			const auto mt = firp->getArchitecture()->getMachineType();

			if (mt == admtX86_64 || mt == admtI386)
			{
				// work for both 32- and 64-bit.
				check_for_PIC_switch_table32_type2(firp, insn, *disasm, exeiop, thunk_bases);
				check_for_PIC_switch_table32_type3(firp, insn, *disasm, exeiop, thunk_bases);

				if (firp->getArchitectureBitWidth() == 32)
					check_for_PIC_switch_table32(firp, insn, *disasm, exeiop, thunk_bases);
				else if (firp->getArchitectureBitWidth() == 64)
					check_for_PIC_switch_table64(firp, insn, *disasm, exeiop);
				else
					assert(0);

				check_for_nonPIC_switch_table(firp, insn, *disasm, exeiop);
				check_for_nonPIC_switch_table_pattern2(firp, insn, *disasm, exeiop);
			}
			else if (mt == admtAarch64)
			{
				check_for_arm64_switch_type1(firp, insn, *disasm, exeiop);
			}
			else if (mt == admtArm32)
			{
				check_for_arm32_switch_type1(firp, insn, *disasm, exeiop);
				check_for_arm32_switch_type2(firp, insn, *disasm, exeiop);
				check_for_arm32_switch_type3(firp, insn, *disasm, exeiop);
			}
			else if (mt == admtMips32)
			{
				/* no reason to look for pc-rel constants in mips */
				if (firp->getArchitecture()->getFileType() == adftELFSO)
					continue;
				;
			}
			else
				throw invalid_argument("Cannot determine machine type");

			/* other branches can't indicate an indirect branch target */
			if (disasm->isBranch()) // disasm.Instruction.BranchType)
				continue;

			ibt_provenance_t::provtype_t prov = 0;
			if (!texttoprintf(firp, insn))
			{
				prov = ibt_provenance_t::ibtp_text;
			}
			else
			{
				cout << "TextToPrintf analysis of '" << disasm->getDisassembly() << "' successful at " << hex << insn->getAddress()->getVirtualOffset() << endl;
				prov = ibt_provenance_t::ibtp_texttoprintf;
			}
			/* otherwise, any immediate is a possible branch target */
			for (const auto &op : disasm->getOperands())
			{
				if (op->isConstant())
					possible_target(op->getConstant(), 0, prov);
			}

			for (auto i = 0; i < 4; i++)
			{
				if (disasm->hasOperand(i))
				{
					const auto op = disasm->getOperand(i);
					handle_argument(*disasm, *op, insn, prov);
				}
			}
		}
	}

	void get_executable_bounds(FileIR_t *firp, const section *shdr)
	{

		/* not a loaded section */
		if (!shdr->isLoadable())
			return;

		/* loaded, and contains instruction, record the bounds */
		if (!shdr->isExecutable())
			return;

		VirtualOffset_t first = shdr->get_address();
		VirtualOffset_t second = shdr->get_address() + shdr->get_size();

		bounds.insert(pair<VirtualOffset_t, VirtualOffset_t>(first, second));
	}

	/*
	 * Return true iff a reloc exists for the address in question
	 */
	template <class RelocType>
	bool priv_has_reloc(FileIR_t *firp, EXEIO::exeio *exeiop, uint64_t address)
	{
		// grab the relocations, and return if they don't exist.
		const auto reloc_sec = exeiop->sections[".rela.dyn"];
		if (!reloc_sec)
			return false;

		const auto reloc_data = reloc_sec->get_data();

		// for each entry in the reloc table.
		for (auto i = 0u; i + sizeof(RelocType) <= (size_t)reloc_sec->get_size(); i += sizeof(RelocType))
		{
			// extract the address
			const auto reloc = *reinterpret_cast<const RelocType *>(&reloc_data[i]);
			const auto r_offset = targetToHost(firp, reloc.r_offset);

			// if it matches, we are done.
			if (r_offset == address)
			{
				return true;
			}
		}
		return false;
	}

	/*
	 * Given an IR, determine if there is a reloc corresponding to the address given.
	 */
	bool has_reloc(FileIR_t *firp, EXEIO::exeio *exeiop, uint64_t address)
	{
		return arch_ptr_bytes() == 4 ? priv_has_reloc<Elf64_Rela>(firp, exeiop, address) : arch_ptr_bytes() == 8 ? priv_has_reloc<Elf32_Rela>(firp, exeiop, address)
																												 : throw invalid_argument("Cannot map architecture size to bit width");
	}

	void infer_targets(FileIR_t *firp, section *shdr, EXEIO::exeio *exeiop)
	{
		/* check for a not loaded section */
		if (!shdr->isLoadable())
			return;

		/* check for a loaded, but contains instruction section.
		 * we'll look through the VariantIR for this section. */
		if (shdr->isExecutable())
			return;

		/* if the type is NOBITS, then there's no actual data to look through */
		if (shdr->isBSS())
			return;

		// skip .dynsym section -- process-dynsym does this.
		// skip version sections -- no code pointers here.
		if (
			shdr->get_name() == ".gnu.version" ||
			shdr->get_name() == ".gnu.version_r" ||
			shdr->get_name() == ".dynsym")
		{
			return;
		}

		cout << "Checking section " << shdr->get_name() << endl;

		const char *data = shdr->get_data();

		assert(arch_ptr_bytes() == 4 || arch_ptr_bytes() == 8);
		// assume pointers need to be at least 4-byte aligned.
		for (auto i = 0u; i + arch_ptr_bytes() <= (size_t)shdr->get_size(); i += 4)
		{
			// Even on 64-bit, pointers might be stored as 32-bit, as a
			// elf object has the 32-bit limitations.  E.g., reloc tables, PLTs, GOTs, etc.
			// are still stored in 32-bit format.
			// Thus, we don't bother looking for 64-bit pointers
			// FIXME: Prior comment does not match the code.  Clarify which model is needed,
			// and make comment match code.
			const auto ptr_val = uint64_t(
				// cptrtoh<uint32_t>(firp, reinterpret_cast<const uint8_t*>(&data[i]))
				(arch_ptr_bytes() == 4) ? cptrtoh<uint32_t>(firp, reinterpret_cast<const uint8_t *>(&data[i])) : (arch_ptr_bytes() == 8) ? cptrtoh<uint64_t>(firp, reinterpret_cast<const uint8_t *>(&data[i]))
																																		 : throw invalid_argument("Cannot map architecture size to bit width"));

			const auto ptr_addr = i + shdr->get_address();

			const auto ptr_prov =
				(shdr->get_name() == ".init_array") ? ibt_provenance_t::ibtp_initarray : (shdr->get_name() == ".fini_array") ? ibt_provenance_t::ibtp_finiarray
																					 : (shdr->get_name() == ".got.plt")		 ? ibt_provenance_t::ibtp_gotplt
																					 : (shdr->get_name() == ".got")			 ? ibt_provenance_t::ibtp_got
																					 : (shdr->get_name() == ".symtab")		 ? ibt_provenance_t::ibtp_symtab
																					 : (shdr->isWriteable())				 ? ibt_provenance_t::ibtp_data
																															 : ibt_provenance_t::ibtp_rodata;

			// if this is a DLL, constant addresses in rodata need a relocation of some
			// form to be considered a target.
			// we add the is_possible_check because it's _much_ quicker than has_reloc
			if (
				exeiop->isDLL() &&
				shdr->get_name() == ".rodata" &&
				is_possible_target(ptr_val, ptr_addr) &&
				!has_reloc(firp, exeiop, ptr_addr))
				continue;

			possible_target(ptr_val, ptr_addr, ptr_prov);
		}
	}

	void handle_scoop_scanning(FileIR_t *firp)
	{
		// check for addresses in scoops in the text section.
		for (auto scoop : firp->getDataScoops())
		{
			if (scoop->getName() == ".ctor" || scoop->getName() == ".dtor")
			{
				const auto &scoop_contents = scoop->getContents();
				const auto ptrsize = firp->getArchitectureBitWidth() / 8;
				for (auto i = 0u; i + ptrsize < scoop_contents.size(); i += ptrsize)
				{
					const auto ptr = cptrtoh<uint64_t>(firp, reinterpret_cast<const uint8_t *>(scoop_contents.c_str() + i));
					possible_target(ptr, scoop->getStart()->getVirtualOffset() + i, ibt_provenance_t::ibtp_data);
				}
			}

			// test if scoop was added by fill_in_cfg -- make this test better.
			if (scoop->getName().find("data_in_text_") == string::npos)
				continue;

			// at the moment, FIC only creates 4-, 8-, and 16- bytes scoops
			// change this code if FIC chagnes.
			if (scoop->getSize() == 4)
			{
				// may be a 4-byter, which can't hold an address.
				continue;
			}
			if (scoop->getSize() == 8)
			{
				// check to see if the scoop has an IBTA
				const auto addr = *(uint64_t *)(scoop->getContents().c_str());
				possible_target(addr, scoop->getStart()->getVirtualOffset(), ibt_provenance_t::ibtp_unknown);
			}
			else
			{
				// we may see 16 indicating that a ldr q-word happened.
				// this isn't likely an IBT, so we skip scanning it.
				assert(scoop->getSize() == 16);
			}
		}
	}

	void print_targets()
	{
		int j = 1;
		for (auto p : targets)
		{
			const auto target = p.first;

			cout << hex << target;
			if (j % 10 == 0)
				cout << endl;
			else
				cout << ", ";
			j++;
		}

		cout << endl;
	}

	set<Instruction_t *> find_in_function(string needle, Function_t *haystack)
	{
		regex_t preg;
		set<Instruction_t *> found_instructions;

		assert(0 == regcomp(&preg, needle.c_str(), REG_EXTENDED));

		for (auto candidate : haystack->getInstructions())
		{
			auto disasm = DecodedInstruction_t::factory(candidate);

			// check it's the requested type
			if (regexec(&preg, disasm->getDisassembly().c_str(), 0, nullptr, 0) == 0)
			{
				found_instructions.insert(candidate);
			}
		}
		regfree(&preg);
		return found_instructions;
	}

	void check_for_arm32_switch_type1(
		FileIR_t *firp,
		Instruction_t *insn,
		const DecodedInstruction_t &d10,
		EXEIO::exeio *exeiop)
	{
		const auto prov = ibt_provenance_t::ibtp_switchtable_type1;

		/*
		 * Check for hand-written assembly for divsi and udivsi that has this dispatch insn: addeq pc, pc, <reg> lsl #2
		 */
		const auto d = DecodedInstruction_t::factory(insn);
		const auto is_addne = d->getMnemonic() == "addne";
		if (is_addne)
		{
			const auto is_op0_pc = d->getOperand(0)->getString() == "pc";
			const auto is_op1_pc = d->getOperand(1)->getString() == "pc";
			if (is_op0_pc && is_op1_pc)
			{
				cout << "Found gcc addne pc,pc idiom" << endl;
				for (auto i = 1u; i < 32u; i++)
				{
					const auto ibta = insn->getAddress()->getVirtualOffset() + 8 + i * 12;
					possible_target(ibta, 0, prov);
				}
			}
		}
		return;
	}

	void check_for_arm32_switch_type2(
		FileIR_t *firp,
		Instruction_t *i10,
		const DecodedInstruction_t &d10,
		EXEIO::exeio *exeiop)
	{
#if 0

Looking for this pattern:

I9:	cmp	r2, #4
I10:	ldrls	pc, [pc, r2, lsl #2]

or this:

I8:	ldr	r3, [pc, #k]
I9:	cmp	r2, r3
I10:	ldrls	pc, [pc, r2, lsl #2]

#endif

		const auto prov = ibt_provenance_t::ibtp_switchtable_type2;

		// check that i10 is what we need
		const auto i10_dis = d10.getDisassembly();
		const auto is_i10_ldrls = i10_dis.find("ldrls pc, [pc") == 0;
		if (!is_i10_ldrls)
			return;

		// this is sufficient to determine we have a switch dispatch.
		// now try to figure out the table size.
		auto jt_size = numeric_limits<uint32_t>::max();

		// and we do that by looking  for a cmp on the dispatch register.
		// the dispatch register is the index register in the ldrls instruction.
		// which we find via string extraction.
		const auto i10_dis_15_3 = i10_dis.substr(15, 3); // reg or reg,
		const auto i10_index_reg = i10_dis_15_3[2] == ',' ? i10_dis_15_3.substr(0, 2) : i10_dis_15_3;

		// look for i9
		auto i9 = (Instruction_t *)nullptr;
		if (!backup_until(string() + "cmp " + i10_index_reg + ",",  /* look for this pattern. */
						  i9,                       /* find i9 */
						  i10,                      /* before I10 */
						  "^" + i10_index_reg + "$" /* stop if i10_reg set */
						  ))
		{
			return;
		}

		if (i9 != nullptr)
		{
			// decode i9
			const auto d9 = DecodedInstruction_t::factory(i9);
			const auto d9_op1 = d9->getOperand(1);

			// look for a constant in the 2nd operand.
			if (d9_op1->isConstant())
				jt_size = d9_op1->getConstant();
			else
			{
				// check if it's a register
				// and look backwards for a load of the register from the .text seg
				// TBD
			}
		}

		// extract the jump table -- this is simple as the addresing mode in i10 says it's at is "pc+8".
		const auto jt_addr = i10->getAddress()->getVirtualOffset() + 8u;
		const auto jt_section = find_section(jt_addr, exeiop);
		assert(jt_section);
		const auto jt_secdata = jt_section->get_data();
		const auto jt_secaddr = jt_section->get_address();
		const auto jt_secendaddr = jt_secaddr + jt_section->get_size();

		auto jt_entry_no = 0u;
		while (true)
		{
			// calculate some stuff about the jump table entry we're looking at
			const auto jte_size = 4u;
			const auto jte_offset = jt_entry_no * jte_size;
			const auto jte_addr = jt_addr + jte_offset;

			// stop if we've exceeded the section size
			if (jte_addr + jte_size > jt_secendaddr)
				break;

			// extract the table entry
			const auto jte = *reinterpret_cast<const uint32_t *>(&jt_secdata[jte_addr - jt_secaddr]);

			// mark the instruction at jte as an ibt
			possible_target(jte, jte_addr, prov);

			// check to see if the entry is valid.  if not, exit.
			const auto ibtarget = lookupInstruction(jte);
			if (ibtarget == nullptr)
				break;

			cout << "Found ARM32 switch (ldrls -- type2)@0x" << hex << i10->getAddress()->getVirtualOffset()
				 << " table_entry[" << dec << jt_entry_no << "]=" << hex << jte << "@0x " << jte_addr
				 << " to " << ibtarget->getBaseID() << ":" << ibtarget->getDisassembly()
				 << "@" << ibtarget->getAddress()->getVirtualOffset() << endl;

			// add to i10
			jmptables[i10].insert(ibtarget);

			// stop if we've exceeded the number of table entries we found.
			if (jt_entry_no + 1 > jt_size)
				break;

			jt_entry_no++;
		}

		// add a data scoop for the switch table.
		cout << "Detected " << dec << jt_entry_no << "entries in this table.  adding data scoop for table" << endl;
		addSwitchTableScoop(firp, jt_entry_no + 1, 4, jt_addr, exeiop, nullptr, 0, false);

		// mark that we figured out all possible targets for this ib.
		jmptables[i10].setAnalysisStatus(iasAnalysisComplete);
	}

	void check_for_arm32_switch_type3(
		FileIR_t *firp,
		Instruction_t *i10,
		const DecodedInstruction_t &d10,
		EXEIO::exeio *exeiop)
	{
#if 0

Looking for this pattern:

I9:	cmp	r2, #4
I10:	addls	pc, [pc, r2, lsl #2]

or this:

I8:	ldr	r3, [pc, #k]
I9:	cmp	r2, r3
I10:	addls	pc, [pc, r2, lsl #2]

#endif

		const auto prov = ibt_provenance_t::ibtp_switchtable_type3;

		// check that i10 is what we need
		const auto i10_dis = d10.getDisassembly();
		const auto is_i10_ldrls = i10_dis.find("addls pc, pc") == 0;
		if (!is_i10_ldrls)
			return;

		// this is sufficient to determine we have a switch dispatch.
		// now try to figure out the table size.
		auto jt_size = numeric_limits<uint32_t>::max();

		// and we do that by looking  for a cmp on the dispatch register.
		// the dispatch register is the index register in the ldrls instruction.
		// which we find via string extraction.
		const auto i10_index_reg = d10.getOperand(2)->getString();

		// look for i9
		auto i9 = (Instruction_t *)nullptr;
		if (!backup_until(string() + "cmp " + i10_index_reg + ",",  /* look for this pattern. */
						  i9,                       /* find i9 */
						  i10,                      /* before I10 */
						  "^" + i10_index_reg + "$" /* stop if i10_reg set */
						  ))
		{
			return;
		}

		if (i9 != nullptr)
		{
			// decode i9
			const auto d9 = DecodedInstruction_t::factory(i9);
			const auto d9_op1 = d9->getOperand(1);

			// look for a constant in the 2nd operand.
			if (d9_op1->isConstant())
				jt_size = d9_op1->getConstant();
			else
			{
				// check if it's a register
				// and look backwards for a load of the register from the .text seg
				// TBD
			}
		}

		// extract the jump table -- this is simple as the addresing mode in i10 says it's at is "pc+8".
		const auto jt_addr = i10->getAddress()->getVirtualOffset() + 8u;
		const auto jt_entry_size = 4u;

		auto jt_entry_no = 0u;
		while (true)
		{
			// check to see if the entry is valid.  if not, exit.
			const auto jte = jt_addr + jt_entry_no * jt_entry_size;
			const auto ibtarget = lookupInstruction(jte);
			if (ibtarget == nullptr)
				break;

			// check if it's an uncond branch
			const auto ibt_dis = DecodedInstruction_t::factory(ibtarget);
			if (ibt_dis->getMnemonic() != "b")
				break;

			// mark the instruction at jte as an ibt
			possible_target(jte, 0, prov);

			cout << "Found ARM32 switch (addls -- type2)@0x" << hex << i10->getAddress()->getVirtualOffset()
				 << " to " << ibtarget->getBaseID() << ":" << ibtarget->getDisassembly()
				 << "@" << ibtarget->getAddress()->getVirtualOffset() << endl;

			// add to i10
			jmptables[i10].insert(ibtarget);

			// stop if we've exceeded the number of table entries we found.
			if (jt_entry_no + 1 > jt_size)
				break;

			jt_entry_no++;
		}

		// add a data scoop for the switch table.
		cout << "Detected " << dec << jt_entry_no << "entries in this table.  adding data scoop for table" << endl;

		// mark that we figured out all possible targets for this ib.
		jmptables[i10].setAnalysisStatus(iasAnalysisComplete);
	}
	void check_for_arm64_switch_type1(
		FileIR_t *firp,
		Instruction_t *i10,
		const DecodedInstruction_t &d10,
		EXEIO::exeio *exeiop)
	{
		const auto prov = ibt_provenance_t::ibtp_switchtable_type1;

#if 0
Sample code for this branch type:

       ; x2 gets the value of x0
       ; this probably is not normal or required, but we are not checking 
       ; it anyhow.  This is just to understand the example.

i1:    0x4039c4:    cmp     table_index_reg, #0x3
i2:    0x4039c8:    b.hi    0x4039d4	 ; may also be a b.ls

       // generate switch table base address
       // this code may be hard to find if the compiler optimizes it
       // outside the block with the rest of the dispatch code, and/or
       // spills a register.
       // thus, we allow for it not to be found, and instead us any "unk"
       // we return true if we've found the entry to avoid looking at unks
       // if we don't need to.
i5:    0x40449c:    adrp    table_page_reg, 0x415000          // table page 
i6:    0x4044a0:    add     table_base_reg, table_page_reg, #0x2b0 // table offset 
       // table=table_page+table_offset
       //
       // load from switch table
i7:    0x4044a4:    ldrh    table_entry_reg, [table_base_reg,table_index_reg,uxtw #1]
or
i7:    0x4044a4:    ldrb    table_entry_reg, [table_base_reg,table_index_reg,uxtw ]

       // calculate branch_addr+4+table[i]*4
i8:    0x4044a8:    adr     branch_reg, 0x4044b4 // jump base addr
i9:    0x4044ac:    add     i10_reg, branch_reg, table_entry_reg, sxth #2
       // actually take the branch
i10:   0x4044b0:    br      i10_reg
i11:   0x4044b4:    


notes:

1) jump table entries are 2-bytes
2) jump table entries specify an offset from the byte after dispatch branch
3) jump table entries dont store the lower 2 bits of the offset, as they 
   have to be 0 due to instruction alignment

#endif
		// sanity check the jump
		if (d10.getMnemonic() != "br")
			return;

		// grab the reg
		const auto i10_reg = d10.getOperand(0)->getString();

		// try to find I9
		auto i9 = (Instruction_t *)nullptr;
		/* search for externder=sxth or sxtb */
		if (!backup_until(string() + "(add " + i10_reg + ",.* sxth #2)|(add " + i10_reg + ",.* sxtb #2)", /* look for this pattern. */
						  i9,																			  /* find i9 */
						  i10,																			  /* before I10 */
						  "^" + i10_reg + "$"															  /* stop if i10_reg set */
						  ))
		{
			return;
		}

		// Extract the I9 fields.
		assert(i9);
		const auto d9p = DecodedInstruction_t::factory(i9);
		const auto &d9 = *d9p;
		const auto offset_reg = d9.getOperand(1)->getString();
		const auto table_entry_reg = d9.getOperand(2)->getString();

		// try to find I8
		auto i8 = (Instruction_t *)nullptr;
		if (!backup_until(string() + "adr " + offset_reg + ",",  /* look for this pattern. */
						  i8,                    /* find i8 */
						  i9,                    /* before I9 */
						  "^" + offset_reg + "$" /* stop if offste_reg set */
						  ))
			return;

		// extract the I8 fields
		assert(i8);
		const auto d8p = DecodedInstruction_t::factory(i8);
		const auto &d8 = *d8p;
		const auto jump_base_addr = d8.getOperand(1)->getConstant();

		// try to find I7
		auto i7 = (Instruction_t *)nullptr;
		if (!backup_until(string() + "(ldrh " + table_entry_reg + ",)|(ldrb " + table_entry_reg + ",)", /* look for this pattern. */
						  i7,                         /* find i7 */
						  i9,                         /* before I9 */
						  "^" + table_entry_reg + "$" /* stop if index_reg set */
						  ))
			return;

		// extract the I7 fields
		assert(i7);
		const auto d7p = DecodedInstruction_t::factory(i7);
		const auto &d7 = *d7p;
		const auto memory_op_string = d7.getOperand(1)->getString();
		const auto plus_pos = memory_op_string.find(" +");
		const auto table_base_reg = memory_op_string.substr(0, plus_pos);
		const auto table_index_reg = memory_op_string.substr(plus_pos + 3);
		const auto table_entry_size =
			d7.getMnemonic() == "ldrb" ? 1 : d7.getMnemonic() == "ldrh" ? 2
																		: throw invalid_argument("Unable to detected switch table entry size for ARM64");

		// now we try to find the table base in I5 and I6
		// but we may fail due to compiler opts.  Prepare for such failures
		// by creating a set of possible table bases.
		// If we find i5/i6 or a reload of a spilled table address,
		// we will refine our guess.
		auto all_table_bases = per_reg_add_adrp_results[table_base_reg];

		// try to find I6
		auto i6 = (Instruction_t *)nullptr;
		if (backup_until(string() + "add " + table_base_reg + ",",   /* look for this pattern. */
						 i6,                         /* find i6 */
						 i7,                         /* before I7 */
						 "^" + table_base_reg + "$", /* stop if table_base_reg set */
						 "", 			     /* no opcodes */
						 true,                       /* look hard -- recursely examine up to 10k instructions and 500 blocks */
						 10000,
						 500))
		{

			// extract the I6 fields
			assert(i6);
			const auto d6p = DecodedInstruction_t::factory(i6);
			const auto &d6 = *d6p;
			const auto table_page_reg = d6.getOperand(1)->getString();
			const auto table_page_offset = d6.getOperand(2)->getConstant();

			// try to find I5
			auto i5 = (Instruction_t *)nullptr;
			if (backup_until(string() + "adrp " + table_page_reg + ",", /* look for this pattern. */
							 i5,                        /* find i5 */
							 i6,                        /* before I6 */
							 "^" + table_page_reg + "$",/* stop if table_page set */
						 	"", 			    /* no opcodes */
							 true,                      /* look hard -- recursely examine up to 10k instructions and 500 blocks */
							 10000,
							 500))
			{
				// extract i5 fields
				assert(i5);
				const auto d5p = DecodedInstruction_t::factory(i5);
				const auto &d5 = *d5p;
				const auto table_page = d5.getOperand(1)->getConstant();
				const auto table_addr = table_page + table_page_offset;
				all_table_bases = PerFuncAddrSet_t({static_cast<VirtualOffset_t>(table_addr)});
			}
			else
			{
				for (auto adrp_value : all_adrp_results[i10->getFunction()])
				{
					all_table_bases.insert(adrp_value + table_page_offset);
				}
			}
		}
		// could not find i5/i6, it's possible (likely) that the table was just spilled and is being
		// reloaded from the stack.  check for that.
		else if (backup_until(string() + "ldr " + table_base_reg + ",",       /* look for this pattern. */
							  i6,                         /* find i6 -- the reload of the table */
							  i7,                         /* before I7 */
							  "^" + table_base_reg + "$", /* stop if table_base_reg set */
						 	  "", 			      /* no opcodes */
							  true,                       /* look hard -- recursely examine up to 10k instructions and 500 blocks */
							  10000,
							  500))
		{
			assert(i6);
			const auto d6p = DecodedInstruction_t::factory(i6);
			const auto &d6 = *d6p;
			// found reload of table address from spill location!
			// reloads write to the stack with a constant offset.
			const auto reload_op1 = d6.getOperand(1);
			assert(reload_op1->isMemory());
			const auto reload_op1_string = reload_op1->getString();

			// needs to have a base reg, which is either sp or x29
			const auto hasbr = (reload_op1->hasBaseRegister());
			const auto okbr = (reload_op1_string.substr(0, 2) == "sp" || reload_op1_string.substr(0, 3) == "x29");
			const auto hasdisp = (reload_op1->hasMemoryDisplacement());
			const auto hasir = (reload_op1->hasIndexRegister());
			const auto ok = hasbr && okbr && hasdisp && !hasir;

			if (ok)
			{
				// extract fields
				const auto reload_disp = reload_op1->getMemoryDisplacement();
				const auto reload_loc = SpillPoint_t({i10->getFunction(), reload_disp});
				const auto &spills = spilled_add_adrp_results[reload_loc];
				if (spills.size() > 0)
				{
					all_table_bases = spills;
					cout << "Using spilled table bases from stack!" << endl;
				}
			}
		}
		// also possible we couldn't find it spilled to the stack, and it's instead spilled to an FP register.
		else if (backup_until(string() + "fmov " + table_base_reg + ",",      /* look for this pattern. */
							  i6,                         /* find i6 -- the reload of the table from an FP reg*/
							  i7,                         /* before I7 */
							  "^" + table_base_reg + "$", /* stop if table_base_reg set */
						 	  "", 			      /* no opcodes */
							  true,                       /* look hard -- recursely examine up to 10k instructions and 500 blocks */
							  10000,
							  500))
		{
			assert(i6);
			const auto d6p = DecodedInstruction_t::factory(i6);
			const auto &d6 = *d6p;
			const auto reload_op1 = d6.getOperand(1);
			const auto reload_op1_str = reload_op1->getString();
			const auto is_dreg = reload_op1_str[0] == 'd';

			if (is_dreg)
			{

				const auto reload_loc = DregSpillPoint_t({i6->getFunction(), reload_op1_str});
				const auto spills = spilled_to_dreg[reload_loc];
				if (spills.size() > 0)
				{
					all_table_bases = spills;
					cout << "Using spilled table bases from d-reg!" << endl;
				}
			}
		}
		// end trying to find the table base.

		// try to find i1+i2 so we can assign a resonable bound on the switch table size.
		// assume it's 1024 if we can't otherwise find it.
		const auto max_bound = 1024u;
		auto my_bound = max_bound;

		// start by finding i2.
		auto i2 = (Instruction_t *)nullptr;
		if (backup_until(string() + "(b.hi)|(b.ls)", /* look for this pattern. */
						 i2,/* find i2 */
						 i7,/* before I7 */
						 "",/* don't stop for reg sets, just look for control flow */
						 "",/* no opcode */
						 true/* recurse into other blocks */
						 ))
		{
			/* find i1 */
			auto i1 = (Instruction_t *)nullptr;
			if (backup_until(string() + "cmp ",                           /* look for this pattern. */
							 i1,                          /* find i1 */
							 i2,                          /* before I2 */
							 "(cmp)|(adds)|(subs)|(cmn)", /* stop for CC-setting insns -- fixme, probably not the right syntax for stop-if */
							 "",                          /* no opcode stopping */
							 true                         /* recurse into other blocks */
							 ))
			{
				// try to verify that there's data flow from the ldr[bh] to the cmp
				auto next_reg = table_index_reg;
				auto prev_insn = i7;
				while (true)
				{
					auto new_i1 = (Instruction_t *)nullptr;
					if (backup_until(string() + "cmp " + next_reg + ",",   /* look for this pattern. */
									 new_i1,               /* find i1 */
									 prev_insn,            /* before prev_insn */
									 "^" + next_reg + "$", /* stop if next_reg is set */
									 "",                   /* no opcode stopping */
									 true                  /* recurse into other blocks */
									 ))
					{
						if (i1 != new_i1) /* sanity check that we got to the same place */
							break;
						const auto d1 = DecodedInstruction_t::factory(i1);
						const auto d1op1 = d1->getOperand(1);
						if (d1op1->isConstant())
						{
							my_bound = d1op1->getConstant();
						}
						break;
					}
					else if (backup_until(string() + "mov " + next_reg + ",",      /* look for this pattern. */
										  new_i1,              /* find i1 */
										  prev_insn,           /* before I2 */
										  "^" + next_reg + "$",/* stop if next_reg is set */
									 	  "",                  /* no opcode stopping */
										  true                 /* recurse into other blocks */
										  ))
					{
						// track backwards on reg 2 if we find a mov <reg1>, <reg2>
						const auto d1 = DecodedInstruction_t::factory(new_i1);
						const auto new_d1op1 = d1->getOperand(1);
						if (new_d1op1->isRegister())
						{
							next_reg = new_d1op1->getString();
							continue;
						}
						else
						{
							// movd constant into table reg?  wtf
							break;
						}
					}
					else
					{
						// no bound found
						break;
					}
					assert(0);
				}
			}
		}

		const auto do_verbose = getenv("IB_VERBOSE");

		// given:
		//   1) a set of possible places for a jump table (all_table_bases)
		//   2) address from which the the tables is relative (jump_base_addr)
		//   3) the size of the table (my_bound)
		//
		//   calculate any possible targets for this switch.

		// consider each jump table
		auto my_targets = set<VirtualOffset_t>();
		auto valid_table_count = 0u;
		for (auto cur_table_addr : all_table_bases)
		{

			// try to find switch table jumps
			const auto i10_func = i10->getFunction();

			// find the section with the jump table
			const auto table_section = find_section(cur_table_addr, exeiop);
			if (!table_section)
				continue;

			// if the section has no data, abort if not found
			const auto table_section_data_ptr = table_section->get_data();
			if (table_section_data_ptr == nullptr)
				continue;

			// get the section's adddress, abort if not loaded
			const auto table_section_address = table_section->get_address();
			if (table_section_address == 0)
				continue;

			// ARM swith tables are in ROdata, thus not exectuable and not writeable.
			if (table_section->isExecutable())
				continue;
			if (table_section->isWriteable())
				continue;

			// calculate how far into the section the table is.
			const auto table_offset_into_section = (cur_table_addr - table_section_address);

			// calculate the actual data for the table
			const auto table_data_ptr = table_section_data_ptr + table_offset_into_section;

			// calculate a stop point so we don't fall out of the section.
			const auto table_section_end_ptr = table_section_data_ptr + table_section->get_size();

			// define how to map an index in the table into an address to deref.
			const auto getEntryPtr = [&](const uint64_t index)
			{ return table_data_ptr + index * table_entry_size; };
			const auto getEntryAddr = [&](const uint64_t index)
			{ return cur_table_addr + index * table_entry_size; };

			if (do_verbose)
			{
				// log that we've found a table, etc.
				cout << "Found ARM type1 at 0x" << hex << i10->getAddress()->getVirtualOffset()
					 << " with my_bound = 0x" << hex << my_bound
					 << ", table_entry_size = " << dec << table_entry_size
					 << ", jump_base_addr = 0x" << hex << jump_base_addr
					 << ", table_base_reg='" << table_base_reg << "'"
					 << ", table_index_reg='" << table_index_reg << "'"
					 << ", table_addr=" << cur_table_addr
					 << endl;
			}

			const auto do_verbose = getenv("IB_VERBOSE") != nullptr;

			// look at each table entry
			auto target_count = 0U;
			using OffOffProvTuple_t = tuple<VirtualOffset_t, VirtualOffset_t, ibt_provenance_t>;
			auto this_table_targets = vector<OffOffProvTuple_t>();
			for (; getEntryPtr(target_count) + table_entry_size < table_section_end_ptr; target_count++)
			{
				const auto entry_ptr = getEntryPtr(target_count);
				const auto entry_address = getEntryAddr(target_count);
				const auto table_entry =
					table_entry_size == 1 ? *(int8_t *)entry_ptr : table_entry_size == 2 ? *(int16_t *)entry_ptr
																						 : throw invalid_argument("Cannot determine how to load table entry from size");
				const auto candidate_ibta = jump_base_addr + table_entry * 4; // 4 being instruction alignment factor for ARM64
				const auto ibtarget = lookupInstruction(candidate_ibta);

				if (do_verbose)
					cout << "\tEntry #" << dec << target_count << "= ent-addr=" << hex << entry_address
						 << " ent=" << hex << +table_entry // print as int, not char.
						 << " ibta=" << candidate_ibta;

				// stop if we failed to find an instruction,
				// or find an instruction outside the function
				if (ibtarget == nullptr)
				{
					if (do_verbose)
						cout << " -- no target insn!" << endl;
					break;
				}
				const auto ibtarget_func = ibtarget->getFunction();
				if (i10_func == nullptr)
				{
					// finding switch in non-function is OK.
				}
				else if (ibtarget_func == nullptr)
				{
					// finding target in non-function is OK
				}
				else if (i10_func != ibtarget_func)
				{
					// finding switch in function to different function, not ok.
					if (do_verbose)
						cout << " -- switch to diff func? No." << endl;
					break;
				}

				// record that we found something that looks valid-enough to try to pin
				// stop if we couldn't pin.
				if (!is_possible_target(candidate_ibta, entry_address))
				{
					if (do_verbose)
						cout << " -- not valid target!" << endl;
					break;
				}
				if (firp->findScoop(candidate_ibta) != nullptr)
				{
					if (do_verbose)
						cout << " -- not valid target due to data-in-text detection!" << endl;
					break;
				}

				if (do_verbose)
					cout << " -- valid target!" << endl;

				// record this entry of the table
				// this works on newer compilers and is easier to read, but older compilers....
				// this_table_targets.push_back({candidate_ibta, entry_address,prov});
				this_table_targets.push_back(make_tuple(candidate_ibta, entry_address, prov));

				if (target_count > my_bound)
					break;
			}
			// allow this table if the bound was a max-bound situation.
			// also allow if all the table entries were valid.
			// (allow week off-by-one comparison due to inaccurancies in detecting exact bound)
			if (my_bound == max_bound || target_count + 1 >= my_bound)
			{
				// since this table is valid, add all entries to the list of IBT's for this IR.
				for (auto &t : this_table_targets)
				{
					const auto candidate_ibta = get<0>(t);
					possible_target(candidate_ibta, get<1>(t), get<2>(t));
					my_targets.insert(candidate_ibta);
				}
				valid_table_count++;
			}
		}
		cout << "Across " << dec << all_table_bases.size() << " tables, " << valid_table_count << " are valid, bound=" << my_bound
			 << " unique target count=" << my_targets.size() << " entry_size=" << table_entry_size << endl;
		return;
	}

	/*
	 * check_for_PIC_switch_table32 - look for switch tables in PIC code for 32-bit code.
	 */
	void check_for_PIC_switch_table32(FileIR_t *firp, Instruction_t *insn, const DecodedInstruction_t &disasm, EXEIO::exeio *exeiop, const set<VirtualOffset_t> &thunk_bases)
	{

		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type1;
#if 0

/* here's typical code */

I1: 080a9037 <gedit_floating_slider_get_type+0x607> call   0806938e <_gedit_app_ready+0x8e>  	// ebx=080a903c
I2: 080a903c <gedit_floating_slider_get_type+0x60c> add    $0x45fb8,%ebx			// ebx=<module_start>
...
I3: 080a90f8 <gedit_floating_slider_get_type+0x6c8> mov    -0x1ac14(%ebx,%esi,4),%eax		// table_start=<module_start-0x1ac14
												// table_offset=eax=table_start[esi]
I4: 080a90ff <gedit_floating_slider_get_type+0x6cf> add    %ebx,%eax				// switch_case=eax=<module_start>+table_offset
I5: 080a9101 <gedit_floating_slider_get_type+0x6d1> jmp    *%eax				// jump switch_case
...
I6: 0806938e <_gedit_app_ready+0x8e> mov    (%esp),%ebx
I7: 08069391 <_gedit_app_ready+0x91> ret


/* However, since the thunk and the switch table can be (and sometimes are) very control-flow distinct, 
 * we just collect all the places where a module can start by examining all the thunk/add pairs.
 * After that, we look for all jumps that match the I3-I5 pattern, and consider the offset against each
 * module start.  If we find that there are possible targets at <module_start>+table_offset, we record them.
 */

#endif

		Instruction_t *I5 = insn;
		Instruction_t *Icmp = nullptr;
		Instruction_t *I4 = nullptr;
		Instruction_t *I3 = nullptr;
		// check if I5 is a jump
		if (disasm.getMnemonic() != "jmp")
			return;

		// return if it's a jump to a constant address, these are common
		if (disasm.getOperand(0)->isConstant())
			return;

		// return if it's a jump to a memory address
		if (disasm.getOperand(0)->isMemory())
			return;

		assert(disasm.getOperand(0)->isRegister());
		const auto I5_reg = disasm.getOperand(0)->getString();
		auto jmp_reg = string();
		auto add_reg = string();

		// has to be a jump to a register now

		// backup and find the instruction that's an add before I8
		if (!backup_until(string() + "add " + I5_reg + "|lea " + I5_reg, I4, I5, I5_reg))
		{
			auto mov_insn = static_cast<Instruction_t *>(nullptr);
			if (!backup_until(string() + "mov " + I5_reg, mov_insn, I5, I5_reg))
				return;
			const auto p_mov_insn_disasm = DecodedInstruction_t::factory(mov_insn);
			const auto &mov_insn_disasm = *p_mov_insn_disasm;
			if (!mov_insn_disasm.getOperand(1)->isRegister())
				return;
			const auto mov_reg = mov_insn_disasm.getOperand(1)->getString();
			if (!backup_until(string() + "add " + mov_reg, I4, mov_insn, mov_reg))
				return;
			jmp_reg = mov_reg;
		}
		else
		{
			const auto p_d4 = DecodedInstruction_t::factory(I4);
			const auto &d4 = *p_d4;
			if (d4.getMnemonic() == "lea")
			{
				const auto base_reg = d4.getOperand(1)->getBaseRegister();
				switch (base_reg)
				{
				case 0 /*REG0*/:
					jmp_reg = "eax";
					break;
				case 1 /*REG1*/:
					jmp_reg = "ecx";
					break;
				case 2 /*REG2*/:
					jmp_reg = "edx";
					break;
				case 3 /*REG3*/:
					jmp_reg = "ebx";
					break;
				case 4 /*REG4*/:
					jmp_reg = "esp";
					break;
				case 5 /*REG5*/:
					jmp_reg = "ebp";
					break;
				case 6 /*REG6*/:
					jmp_reg = "esi";
					break;
				case 7 /*REG7*/:
					jmp_reg = "edi";
					break;
				default:
					// no base register;
					return;
				}
				const auto index_reg = d4.getOperand(1)->getBaseRegister();
				switch (index_reg)
				{
				case 0 /*REG0*/:
					add_reg = "eax";
					break;
				case 1 /*REG1*/:
					add_reg = "ecx";
					break;
				case 2 /*REG2*/:
					add_reg = "edx";
					break;
				case 3 /*REG3*/:
					add_reg = "ebx";
					break;
				case 4 /*REG4*/:
					add_reg = "esp";
					break;
				case 5 /*REG5*/:
					add_reg = "ebp";
					break;
				case 6 /*REG6*/:
					add_reg = "esi";
					break;
				case 7 /*REG7*/:
					add_reg = "edi";
					break;
				default:
					// no base register;
					return;
				}
			}
			else
			{
				jmp_reg = I5_reg;
				if (!d4.getOperand(1)->isRegister())
					return;
				add_reg = d4.getOperand(1)->getString();
			}
		}

		assert(jmp_reg != "" && add_reg != "" && I4 != nullptr);

		// backup and find the instruction that's an movsxd before I7
		if (!backup_until(string() + "(mov " + jmp_reg + "|mov " + add_reg + ")", I3, I4))
			return;

		auto table_size = 0U;
		if (!backup_until("cmp", Icmp, I3))
		{
			cerr << "pic32: could not find size of switch table" << endl;
			table_size = std::numeric_limits<int>::max();
			// set table_size to be very large, so we can still do pinning appropriately
		}
		else
		{
			auto dcmp = DecodedInstruction_t::factory(Icmp);
			table_size = dcmp->getImmediate();
			if (table_size <= 0)
				table_size = std::numeric_limits<int>::max();
		}

		// grab the offset out of the lea.
		const auto p_d2 = DecodedInstruction_t::factory(I3);
		const auto &d2 = *p_d2;

		// get the offset from the thunk
		auto table_offset = d2.getAddress();
		if (table_offset == 0)
			return;

		cout << hex << "Found switch dispatch at " << I3->getAddress()->getVirtualOffset()
			 << " with table_offset=" << table_offset << " and table_size=" << table_size << endl;

		/* iterate over all thunk_bases/module_starts */
		for (auto thunk_base : thunk_bases)
		{
			VirtualOffset_t table_base = thunk_base + table_offset;

			// find the section with the data table
			EXEIO::section *pSec = find_section(table_base, exeiop);
			if (!pSec)
				continue;

			// if the section has no data, abort
			const char *secdata = pSec->get_data();
			if (!secdata)
				continue;

			// get the base offset into the section
			VirtualOffset_t offset = table_base - pSec->get_address();
			int i;
			for (i = 0; i < 3; i++)
			{
				if ((int)(offset + i * 4 + sizeof(int)) > (int)pSec->get_size())
					break;

				const int *table_entry_ptr = (const int *)&(secdata[offset + i * 4]);
				int table_entry = *table_entry_ptr;

				if (!is_possible_target(thunk_base + table_entry, table_base + i * 4))
					break;
			}
			/* did we finish the loop or break out? */
			if (i == 3)
			{
				set<Instruction_t *> ibtargets;

				if (getenv("IB_VERBOSE") != 0)
					cout << "Found switch table (thunk-relative) at " << hex << table_base + table_offset << endl;
				// finished the loop.
				for (i = 0; true; i++)
				{
					if ((int)(offset + i * 4 + sizeof(int)) > (int)pSec->get_size())
						break;

					const int32_t *table_entry_ptr = (const int32_t *)&(secdata[offset + i * 4]);
					VirtualOffset_t table_entry = *table_entry_ptr;

					if (getenv("IB_VERBOSE") != 0)
						cout << "Found switch table (thunk-relative) entry[" << dec << i << "], " << hex << thunk_base + table_entry << endl;

					if (!possible_target(thunk_base + table_entry, table_base + i * 4, prov))
						break;

					auto ibtarget = lookupInstruction(thunk_base + table_entry);
					if (ibtarget && ibtargets.size() <= table_size)
					{
						ibtargets.insert(ibtarget);
					}
				}

				// valid switch table? may or may not have default: in the switch
				// table size = 8, #entries: 9 b/c of default
				cout << "pic32 (base pattern): table size: " << table_size << " ibtargets.size: " << ibtargets.size() << endl;
				jmptables[I5].addTargets(ibtargets);
				if (table_size == ibtargets.size() || table_size == (ibtargets.size() - 1))
				{
					cout << "pic32 (base pattern): valid switch table detected ibtp_switchtable_type1" << endl;
					jmptables[I5].setAnalysisStatus(iasAnalysisComplete);
				}
			}
			else
			{
				if (getenv("IB_VERBOSE") != 0)
					cout << "Found that  " << hex << table_base + table_offset << endl;
			}

			// now, try next thunk base
		}
	}

	void check_for_PIC_switch_table32_type2(FileIR_t *firp, Instruction_t *insn, const DecodedInstruction_t &disasm, EXEIO::exeio *exeiop, const set<VirtualOffset_t> &thunk_bases)
	{
		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type2;
		auto ibtargets = InstructionSet_t();
#if 0

/* here's typical code */
I1:   0x8098ffc <text_handler+33>: cmp    eax,0x8
I2:   0x8098fff <text_handler+36>: ja     0x8099067 <text_handler+140>
I3:   0x8099001 <text_handler+38>: lea    ecx,[ebx-0x21620]
I4:   0x8099007 <text_handler+44>: add    ecx,DWORD PTR [ebx+eax*4-0x21620]
I5:   0x809900e <text_handler+51>: jmp    ecx
#endif

		Instruction_t *I5 = insn;
		Instruction_t *I4 = nullptr;
		//        Instruction_t* I3=nullptr;
		// check if I5 is a jump
		if (strstr(disasm.getMnemonic().c_str() /*disasm.Instruction.Mnemonic*/, "jmp") == nullptr)
			return;

		// return if it's a jump to a constant address, these are common
		if (disasm.getOperand(0)->isConstant() /*disasm.Argument1.ArgType&CONSTANT_TYPE*/)
			return;

		// return if it's a jump to a memory address
		// if(disasm.Argument1.ArgType&MEMORY_TYPE)
		if (disasm.getOperand(0)->isMemory())
			return;

		// has to be a jump to a register now

		// backup and find the instruction that's an add before I8
		if (!backup_until("add", I4, I5))
			return;

		const auto d4 = DecodedInstruction_t::factory(I4);
		if (!d4->hasOperand(1) || !d4->getOperand(1)->isMemory())
			return;

		// found that sometimes I3 is set a different way,
		// and that it's perfectly reasonable to just use I4's offsets.
		// backup and find the instruction that's an movsxd before I7
		//	if(!backup_until("lea", I3, I4))
		//		return;

		// get the offset from the thunk
		VirtualOffset_t table_offset = d4->getOperand(1)->getMemoryDisplacement();
		if (table_offset == 0)
			return;

		cout << hex << "Found (type2) switch dispatch at " << I5->getAddress()->getVirtualOffset() << " with table_offset=" << table_offset << endl;

		/* iterate over all thunk_bases/module_starts */
		for (auto thunk_base : thunk_bases)
		{
			auto table_base = thunk_base + table_offset;

			// find the section with the data table
			auto pSec = find_section(table_base, exeiop);
			if (!pSec)
				continue;

			// if the section has no data, abort
			const auto secdata = pSec->get_data();
			if (!secdata)
				continue;

			// get the base offset into the section
			VirtualOffset_t offset = table_base - pSec->get_address();
			int i;
			for (i = 0; i < 3; i++)
			{
				if ((int)(offset + i * 4 + sizeof(int)) > (int)pSec->get_size())
					break;

				const int32_t *table_entry_ptr = (const int32_t *)&(secdata[offset + i * 4]);
				VirtualOffset_t table_entry = *table_entry_ptr;

				// cout<<"Checking target base:" << std::hex << table_base+table_entry << ", " << table_base+i*4<<endl;
				if (!is_possible_target(table_base + table_entry, table_base + i * 4) && !is_possible_target(thunk_base + table_entry, table_base + i * 4))
					break;
			}
			/* did we finish the loop or break out? */
			if (i == 3)
			{
				if (getenv("IB_VERBOSE") != 0)
					cout << "Found switch table (pic3, type2) (thunk-relative) at " << hex << table_base + table_offset << endl;
				// finished the loop.
				for (i = 0; true; i++)
				{
					if ((int)(offset + i * 4 + sizeof(int)) > (int)pSec->get_size())
						break;

					const int32_t *table_entry_ptr = (const int32_t *)&(secdata[offset + i * 4]);
					VirtualOffset_t table_entry = *table_entry_ptr;

					if (getenv("IB_VERBOSE") != 0)
						cout << "Found switch table (thunk-relative) entry[" << dec << i << "], " << hex << table_base + table_entry << " or " << thunk_base + table_entry << endl;
					auto t1 = possible_target(table_base + table_entry, table_base + i * 4, prov);
					auto t2 = possible_target(thunk_base + table_entry, table_base + i * 4, prov);
					if (!t1 && !t2)
						break;

					auto ibtarget1 = lookupInstruction(table_base + table_entry);
					if (ibtarget1)
						ibtargets.insert(ibtarget1);
					auto ibtarget2 = lookupInstruction(thunk_base + table_entry);
					if (ibtarget2)
						ibtargets.insert(ibtarget2);
				}
			}
			else
			{
				if (getenv("IB_VERBOSE") != 0)
					cout << "Found that  " << hex << table_base + table_offset << endl;
			}

			// now, try next thunk base
		}
		jmptables[I5].addTargets(ibtargets);
	}

	/*
	 * Detects this type of switch:
	 *
	 * 	  0x809900e <text_handler+51>: jmp    [eax*4 + 0x8088208]
	 *
	 * nb: also works for 64-bit.
	 */
	void check_for_PIC_switch_table32_type3(FileIR_t *firp, Instruction_t *insn, const DecodedInstruction_t &disasm, EXEIO::exeio *exeiop, const set<VirtualOffset_t> &thunk_bases)
	{
		uint32_t ptrsize = firp->getArchitectureBitWidth() / 8;
		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type3;

		Instruction_t *I5 = insn;
		// check if I5 is a jump
		if (strstr(disasm.getMnemonic().c_str(), "jmp") == nullptr)
			return;

		// return if it's not a jump to a memory address
		if (!(disasm.getOperand(0)->isMemory()))
			return;

		/* return if there's no displacement */
		if (disasm.getOperand(0)->getMemoryDisplacement() == 0)
			return;

		if (!disasm.getOperand(0)->hasIndexRegister() || disasm.getOperand(0)->getScaleValue() != ptrsize)
			return;

		// grab the table base out of the jmp.
		VirtualOffset_t table_base = disasm.getOperand(0)->getMemoryDisplacement();
		if (disasm.getOperand(0)->isPcrel())
			table_base += insn->getDataBits().size() + insn->getAddress()->getVirtualOffset();

		if (table_base == 0)
			return;

		// find the section with the data table
		EXEIO::section *pSec = find_section(table_base, exeiop);
		if (!pSec)
			return;

		auto table_max = numeric_limits<uint32_t>::max();
		auto cmp_insn = (Instruction_t *)nullptr;
		if (backup_until("cmp ", cmp_insn, insn))
		{
			assert(cmp_insn);
			const auto cmp_decode = DecodedInstruction_t::factory(cmp_insn);
			table_max = cmp_decode->getImmediate();
		}

		// if the section has no data, abort
		const char *secdata = pSec->get_data();
		if (!secdata)
			return;

		// get the base offset into the section
		VirtualOffset_t offset = table_base - pSec->get_address();

		// check to see if stars already marked this complete.
		// if so, just figure out the table size
		if (jmptables[insn].getAnalysisStatus() == iasAnalysisComplete)
		{
			// we already know it's a type3, we can just record the type and table base.
			jmptables[insn].AddSwitchType(prov);
			jmptables[insn].SetTableStart(table_base);

			// try to determine the table size using the complete set of targets.
			int i = 0;
			for (i = 0; true; i++)
			{

				if ((int)(offset + i * ptrsize + ptrsize) > (int)pSec->get_size())
					return;

				const void *table_entry_ptr = (const int *)&(secdata[offset + i * ptrsize]);

				VirtualOffset_t table_entry = 0;
				switch (ptrsize)
				{
				case 4:
					table_entry = (VirtualOffset_t) * (int *)table_entry_ptr;
					break;
				case 8:
					table_entry = (VirtualOffset_t) * (int **)table_entry_ptr;
					break;
				default:
					assert(0);
				}

				Instruction_t *ibt = lookupInstruction(table_entry);
				// if we didn't find an instruction or the insn isn't in our set, stop looking, we've found the table size
				if (ibt == nullptr || jmptables[insn].find(ibt) == jmptables[insn].end())
					break;
			}
			jmptables[insn].SetTableSize(i);
			if (getenv("IB_VERBOSE"))
			{
				cout << "Found type3 at " << hex << insn->getAddress()->getVirtualOffset()
					 << " already complete, setting base to " << hex << table_base << " and size to " << dec << i << endl;
			}
			return;
		}

		auto i = 0U;
		for (i = 0; i < 3; i++)
		{
			if ((int)(offset + i * ptrsize + ptrsize) > (int)pSec->get_size())
				return;

			const void *table_entry_ptr = (const int *)&(secdata[offset + i * ptrsize]);

			VirtualOffset_t table_entry = 0;
			switch (ptrsize)
			{
			case 4:
				table_entry = (VirtualOffset_t) * (int *)table_entry_ptr;
				break;
			case 8:
				table_entry = (VirtualOffset_t) * (int **)table_entry_ptr;
				break;
			default:
				assert(0);
			}

			if (getenv("IB_VERBOSE") != 0)
				cout << "Checking target base:" << std::hex << table_entry << ", " << table_base + i * ptrsize << endl;

			/* if there's no base register and no index reg, */
			/* then this jmp can't have more than one valid table entry */
			if (!disasm.getOperand(0)->hasBaseRegister() && !disasm.getOperand(0)->hasIndexRegister())
			{
				/* but the table can have 1 valid entry. */
				if (pSec->get_name() == ".got.plt")
				{
					Instruction_t *ibtarget = lookupInstruction(table_entry);
					if (ibtarget)
					{
						jmptables[I5].insert(ibtarget);
						jmptables[I5].setAnalysisStatus(iasAnalysisModuleComplete);
						possible_target(table_entry, table_base + 0 * ptrsize, ibt_provenance_t::ibtp_gotplt);
						if (getenv("IB_VERBOSE") != 0)
							cout << hex << "Found  plt dispatch (" << disasm.getDisassembly() << "') at " << I5->getAddress()->getVirtualOffset() << endl;
						return;
					}
				}
				if (pSec->isWriteable())
					possible_target(table_entry, table_base + 0 * ptrsize, ibt_provenance_t::ibtp_data);
				else
					possible_target(table_entry, table_base + 0 * ptrsize, ibt_provenance_t::ibtp_rodata);
				if (getenv("IB_VERBOSE") != 0)
					cout << hex << "Found  constant-memory dispatch from non- .got.plt location (" << disasm.getDisassembly() << "') at "
						 << I5->getAddress()->getVirtualOffset() << endl;
				return;
			}
			if (!is_possible_target(table_entry, table_base + i * ptrsize))
			{
				if (getenv("IB_VERBOSE") != 0)
				{
					cout << hex << "Found (type3) candidate for switch dispatch for '" << disasm.getDisassembly() << "' at "
						 << I5->getAddress()->getVirtualOffset() << " with table_base=" << table_base << endl;
					cout << "Found table_entry " << hex << table_entry << " is not valid\n"
						 << endl;
				}
				return;
			}
		}

		cout << hex << "Definitely found (type3) switch dispatch at " << I5->getAddress()->getVirtualOffset() << " with table_base=" << table_base << endl;

		/* did we finish the loop or break out? */
		if (i == 3)
		{
			if (getenv("IB_VERBOSE") != 0)
				cout << "Found switch table (type3)  at " << hex << table_base << endl;
			jmptables[insn].AddSwitchType(prov);
			jmptables[insn].SetTableStart(table_base);
			// finished the loop.
			for (i = 0; true; i++)
			{
				if (i > table_max)
				{
					if (getenv("IB_VERBOSE") != 0)
					{
						cout << hex << "Switch dispatch at " << I5->getAddress()->getVirtualOffset() << " with table_base="
							 << table_base << " is complete!" << endl;
					}
					jmptables[insn].setAnalysisStatus(iasAnalysisComplete);
				}
				if ((int)(offset + i * ptrsize + ptrsize) > (int)pSec->get_size() || i > table_max)
					return;

				const void *table_entry_ptr = (const int *)&(secdata[offset + i * ptrsize]);

				VirtualOffset_t table_entry = 0;
				switch (ptrsize)
				{
				case 4:
					table_entry = (VirtualOffset_t) * (int *)table_entry_ptr;
					break;
				case 8:
					table_entry = (VirtualOffset_t) * (int **)table_entry_ptr;
					break;
				default:
					assert(0);
				}

				Instruction_t *ibt = lookupInstruction(table_entry);
				if (!possible_target(table_entry, table_base + i * ptrsize, prov) || ibt == nullptr)
					return;
				if (getenv("IB_VERBOSE") != 0)
					cout << "Found switch table (thunk-relative) entry[" << dec << i << "], " << hex << table_entry << endl;

				// make table bigger.
				jmptables[insn].SetTableSize(i + 1); /* 0 index, and this index is determined valid. */
				jmptables[insn].insert(ibt);
			}
		}
		else
		{
			if (getenv("IB_VERBOSE") != 0)
				cout << "Found that  " << hex << table_base << endl;
		}
	}

	/* check if this instruction is an indirect jump via a register,
	 * if so, see if we can trace back a few instructions to find a
	 * the start of the table.
	 */
	void check_for_PIC_switch_table64(FileIR_t *firp, Instruction_t *switch_dispatch, const DecodedInstruction_t &p_disasm, EXEIO::exeio *exeiop)
	{
		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type4;
/* here's the pattern we're looking for */
#if 0
I1:   0x000000000044425a <+218>:        cmp    DWORD PTR [rax+0x8],0xd   // bounds checking code, 0xd cases. switch(i) has i stored in [rax+8] in this e.g.
I2:   0x000000000044425e <+222>:        jbe    0x444320 <_gedit_tab_get_icon+416>
<new bb>
I3:   0x0000000000444264 <+228>:        mov    rdi,rbp // default case, also jumped to via indirect branch below
<snip (doesnt fall through)>
I4:   0x0000000000444320 <+416>:        mov    edx,DWORD PTR [rax+0x8]		# load from memory into index reg EDX.

THIS ONE
I5:   0x0000000000444323 <+419>:        lea    rax,[rip+0x3e1b6]        # 0x4824e0
I6:   0x000000000044432a <+426>:        movsxd rdx,DWORD PTR [rax+rdx*4]
I7:   0x000000000044432e <+430>:        add    rax,rdx  // OR: lea rax, [rdx+rax]
I8:   0x0000000000444331 <+433>:        jmp    rax      // relatively standard switch dispatch code


D1:   0x4824e0: .long 0x4824e0-L1       // L1-LN are labels in the code where case statements start.
D2:   0x4824e4: .long 0x4824e0-L2
..
DN:   0x4824XX: .long 0x4824e0-LN




Alternate version:

   	0x00000000000b939d <+25>:	cmp    DWORD PTR [rbp-0x4],0xf
   	0x00000000000b93a1 <+29>:	ja     0xb94a1 <worker_query+285>
   	0x00000000000b93a7 <+35>:	mov    eax,DWORD PTR [rbp-0x4]
   	0x00000000000b93aa <+38>:	lea    rdx,[rax*4+0x0]
I5-1   	0x00000000000b93b2 <+46>:	lea    rax,[rip+0x1f347]        # 0xd8700
I6-2    0x00000000000b93b9 <+53>:	mov    eax,DWORD PTR [rdx+rax*1] 
I6      0x00000000000b93bc <+56>:	movsxd rdx,eax
I5-2   	0x00000000000b93bf <+59>:	lea    rax,[rip+0x1f33a]        # 0xd8700
   	0x00000000000b93c6 <+66>:	add    rax,rdx
   	0x00000000000b93c9 <+69>:	jmp    rax

Note: Since I6 doesnt access memory, do another backup until with to verify address format 

Alternate version 2:

	   0xdcf7 <+7>:	mov    r8,QWORD PTR [rdi+0xa0]
	   0xdcfe <+14>:	cmp    r8,rax
	   0xdd01 <+17>:	jbe    0xdd5c <httpd_got_request+108>
	   0xdd03 <+19>:	mov    r9,QWORD PTR [rdi+0x90]
I5	   0xdd0a <+26>:	lea    rcx,[rip+0x2a427]        # 0x38138
	   0xdd11 <+33>:	cmp    DWORD PTR [rdi+0xb0],0xb
	   0xdd18 <+40>:	movzx  edx,BYTE PTR [r9+rax*1]
	   0xdd1d <+45>:	ja     0xdd4c <httpd_got_request+92>
	   0xdd1f <+47>:	mov    esi,DWORD PTR [rdi+0xb0]
I6	   0xdd25 <+53>:	movsxd rsi,DWORD PTR [rcx+rsi*4]
I7	   0xdd29 <+57>:	add    rsi,rcx
I8	   0xdd2c <+60>:	jmp    rsi

Note: Here the operands of the add are reversed, so lookup code was not finding I5 where it was expected.c


Yet another alternate:
Here, one of the registers used in the switch dispatch is spilled?  How can this be good code?
   0x289e2dc <HUF_decompress4X2_usingDTable_internal_default+3980>:	cmp    di,0x7
   0x289e2e0 <HUF_decompress4X2_usingDTable_internal_default+3984>:	mov    QWORD PTR [rbp-0xe0],rcx
   0x289e2e7 <HUF_decompress4X2_usingDTable_internal_default+3991>:	mov    QWORD PTR [rbp-0x100],rax
   0x289e2ee <HUF_decompress4X2_usingDTable_internal_default+3998>:	mov    QWORD PTR [rbp-0xf0],rax
   0x289e2f5 <HUF_decompress4X2_usingDTable_internal_default+4005>:	ja     0x289e381 <HUF_decompress4X2_usingDTable_internal_default+4145>
   0x289e2fb <HUF_decompress4X2_usingDTable_internal_default+4011>:	lea    rax,[rip+0xd1189e]        # 0x35afba0
   0x289e302 <HUF_decompress4X2_usingDTable_internal_default+4018>:	movsxd rax,DWORD PTR [rax+rdi*4]
   0x289e306 <HUF_decompress4X2_usingDTable_internal_default+4022>:	mov    QWORD PTR [rbp-0x108],rax
   0x289e30d <HUF_decompress4X2_usingDTable_internal_default+4029>:	lea    rax,[rip+0xd1188c]        # 0x35afba0
   0x289e314 <HUF_decompress4X2_usingDTable_internal_default+4036>:	add    rax,QWORD PTR [rbp-0x108]
   0x289e31b <HUF_decompress4X2_usingDTable_internal_default+4043>:	jmp    rax

// Two examples from intel ICX compiler that use a sub instead of an add.  Further, the sub/add happens from memory 
// instead of using a move, then an add/sub


V1:
   0x83d0f6 <__intel_avx_rep_memset+38>:        lea    rsi,[rip+0xc83]        # 0x83dd80 <__intel_avx_rep_memset+3248>
   0x83d102 <__intel_avx_rep_memset+50>:        cmp    r11,0x80
   0x83d109 <__intel_avx_rep_memset+57>:        ja     0x83d120 <__intel_avx_rep_memset+80>
   0x83d111 <__intel_avx_rep_memset+65>:        sub    rsi,QWORD PTR [rsi+r11*8]
   0x83d115 <__intel_avx_rep_memset+69>:        notrack jmp rsi

// This example has no easy-to-infer table size.
V2:

   0x83d120 <__intel_avx_rep_memset+80>:        lea    rsi,[rip+0x359]        # 0x83d480 <__intel_avx_rep_memset+944>
   0x83d127 <__intel_avx_rep_memset+87>:        mov    rcx,r10
   0x83d12a <__intel_avx_rep_memset+90>:        and    rcx,0x1f
   0x83d12e <__intel_avx_rep_memset+94>:        je     0x83d153 <__intel_avx_rep_memset+131>
   0x83d130 <__intel_avx_rep_memset+96>:        neg    rcx
   0x83d133 <__intel_avx_rep_memset+99>:        add    rcx,0x20
   0x83d140 <__intel_avx_rep_memset+112>:       sub    rsi,QWORD PTR [rsi+rcx*8]
   0x83d144 <__intel_avx_rep_memset+116>:       notrack jmp rsi

#endif

		// for now, only trying to find I4-I8.  ideally finding I1 would let us know the size of the
		// jump table.  We'll figure out N by trying targets until they fail to produce something valid.

		Instruction_t *I8 = switch_dispatch;
		Instruction_t *I7 = nullptr;
		Instruction_t *I6 = nullptr;
		Instruction_t *I5 = nullptr;
		// check if I8 is a jump
		if (strstr(p_disasm.getMnemonic().c_str(), "jmp") == nullptr)
			return;

		// return if it's a jump to a constant address, these are common
		if (p_disasm.getOperand(0)->isConstant())
			return;
		// return if it's a jump to a memory address
		if (p_disasm.getOperand(0)->isMemory())
			return;

		// has to be a jump to a register now

		/*
		 * This is the instruction that adds the table value
		 * to the base address of the table. The result is
		 * the target address of the jump.
		 *
		 * Backup and find the instruction that's an add or lea before I8.
		 */
		const auto table_index_reg_str = p_disasm.getOperand(0)->getString();
		const auto table_index_str =
			"(add " + table_index_reg_str + "," +
			"|lea " + table_index_reg_str + "," +
			"|sub " + table_index_reg_str + "," +
			")";
		const auto table_index_stop_if = "^"s + table_index_reg_str + "$";

		// we found a case in a rust program (xsv) where the switch table index
		// was loop invariant and hoisted outside of the loop.  Thus, breaking I8 away from I6-I7.
		// So, we search a bit harder for I7 here.
		if (!backup_until(table_index_str.c_str(), I7, I8, table_index_stop_if, "", true))
			return;

		const auto d7 = DecodedInstruction_t::factory(I7);

		// Check if lea instruction is being used as add (scale=1, disp=0)
		if (d7->getMnemonic() == "lea")
		{
			if (!(d7->getOperand(1)->isMemory()))
				return;
			if (!(d7->getOperand(1)->getScaleValue() == 1 && d7->getOperand(1)->getMemoryDisplacement() == 0))
				return;
		}

		// if we found an lea, or an add/sub with a register
		if (d7->getMnemonic() == "lea" || d7->getOperand(1)->isRegister())
		{

			// calculate the registers we need for the I6 backup.
			const auto I7_reg0 = d7->getMnemonic() == "lea" ? d7->getOperand(1)->getBaseRegister() : d7->getOperand(0)->getRegNumber();
			const auto I7_reg0_32_str = registerToSearchString(RegisterID_t(rn_EAX + I7_reg0));
			const auto I7_reg0_64_str = registerToSearchString(RegisterID_t(rn_RAX + I7_reg0));
			const auto I7_reg1 = d7->getMnemonic() == "lea" ? d7->getOperand(1)->getIndexRegister() : d7->getOperand(1)->getRegNumber();
			const auto I7_reg1_32_str = registerToSearchString(RegisterID_t(rn_EAX + I7_reg1));
			const auto I7_reg1_64_str = registerToSearchString(RegisterID_t(rn_RAX + I7_reg1));

			const auto I6_reg0_str = string() + "(" + I7_reg0_32_str + "|" + I7_reg0_64_str + ")";
			const auto I6_reg1_str = string() + "(" + I7_reg1_32_str + "|" + I7_reg1_64_str + ")";

			// backup and find the instruction that's an movsxd before I7
			/*
			 * This instruction will contain the register names for
			 * the index and the address of the base of the table
			 */
			/* we have to be careful not to stop on an instruction that sets reg0 if we're looking
			 * because we might find the set to reg1.  Thus, we do 2 backups, and continue if either one
			 * is OK.
			 */
			if (
				!backup_until("(mov|movsxd) " + I6_reg0_str + ",", I6, I7, string() + "^" + I6_reg0_str + "$") &&
				!backup_until("(mov|movsxd) " + I6_reg1_str + ",", I6, I7, string() + "^" + I6_reg1_str + "$"))
			{
				// give up if we can't find a mov/movsxd of either register.
				return;
			}
		}
		else if (d7->getMnemonic() != "lea" && d7->getOperand(1)->isMemory())
		{
			assert(d7->getOperand(1)->isMemory());

			/*
			 * Here we handle the icx case where an add/sub is combined with the table memory access
			 * In these cases, I6 and I7 are the same.
			 */
			I6 = I7;
		}
		else
		{
			// sub <reg>, constant?
			return;
		}

		/*
		 * Specify whether the table is multiplied by a value.  Use -1 for subtracts.
		 * TODO:  look for shifts of the value loaded from the table to deal with
		 * size-packing of the table?
		 */
		const auto table_entry_multiplier = d7->getMnemonic() == "sub" ? -1 : 1;

		/*
		 * Now try to find an LEA that loads the table base address into a register.
		 */
		auto lea_string1 = string("lea ");
		auto lea_string2 = string("do_not_mach "); // may be updated later for searching for the index reg
		const auto d6 = DecodedInstruction_t::factory(I6);
		const auto d6_op1 = d6->getOperand(1);
		const auto d6_op1_is_mem = d6_op1->isMemory();
		auto cmp_str = string(" do not match anything ");	   // to be updated inside if statement below
		auto bound_stopif = string(" do not match anything "); // to be updated inside if statement below
		auto and_str = string(" do not match anything ");	   // to be updated inside if statement below

		if (d6_op1_is_mem)
		{
			// try to be smarter for memory types.

			// 64-bit register names are OK here, because this pattern already won't apply to 32-bit code.
			/*
			 * base_reg is the register that holds the address
			 * for the base of the jump table.
			 */
			if (!d6->getOperand(1)->hasBaseRegister())
				return;
			const auto base_reg = regNoToX8664Reg(d6->getOperand(1)->getBaseRegister());

			if (!d6->getOperand(1)->hasIndexRegister())
				return;
			const auto indexRegno = d6->getOperand(1)->getIndexRegister();
			const auto index_reg_64bit = regNoToX8664Reg(indexRegno);
			const auto index_reg_32bit = regNoToX8632Reg(indexRegno);

			cmp_str = "cmp " + index_reg_32bit + "|cmp " + index_reg_64bit;
			bound_stopif = "^" + index_reg_32bit + "$|^" + index_reg_64bit + "$";
			and_str = "and " + index_reg_32bit + "|and " + index_reg_64bit;
			lea_string1 += base_reg;
			if (d6->getOperand(1)->getScaleValue() == 1)
				lea_string2 = "lea " + index_reg_64bit;
		}

		/*
		 * This is the instruction that loads the address of the base
		 * of the table into a register.
		 *
		 * backup and find the instruction that's an lea before I6;
		 * make sure we match the register,
		 * and allow recursion in the search (last parameter as true).
		 *
		 * Convert to return set
		 */

		auto found_leas = InstructionSet_t();

		if (backup_until(lea_string1.c_str(), I5, I6, "", "", true))
			found_leas.insert(I5);
		if (backup_until(lea_string2.c_str(), I5, I6, "", "", true))
			found_leas.insert(I5);

		// if we didn't find anything yet, ....
		if (found_leas.size() == 0 && I6->getFunction())
		{
			cout << "Using find_in_function method." << endl;
			const auto tmp_found_leas = find_in_function("lea ", I6->getFunction());
			found_leas.insert(ALLOF(tmp_found_leas));
		}
		if (found_leas.empty())
		{
			/*
			 * TODO: Write this to warning.txt
			 */
			cout << "WARNING: No I5 candidates found!" << endl;
		}

		/*
		 * Check each one that is returned.
		 */
		const auto allow_unpins = found_leas.size() == 1;
		for (auto I5_cur : found_leas)
		{
			PIC_switch_icc_computed_iterate(firp,I5_cur, I6, I8, table_entry_multiplier);
			PIC_switch_table64_iterate_table(
				firp,
				I8,
				exeiop,
				I5_cur,
				I6,
				table_entry_multiplier,
				prov,
				cmp_str,
				bound_stopif,
				and_str,
				allow_unpins);
		}
		if (allow_unpins)
			jmptables[I8].AddSwitchType(prov);
	}

	/*
	 * Recognize an icc computed jump
	 *
	 * Example:
	 *    0x407488:	lea    rdi,[r8*4+0x0] # indexing_operation
	 *    0x407490:	mov    eax,0x4074c7   # computed_base_insn
     *    0x4074a2:	sub    rax,rdi        # table_entry_multipler = -1
     *    0x4074a5:	jmp    rax   # dispatch_insn
     *    0x4074a7:	movaps XMMWORD PTR [rcx-0xf],xmm7
     *    0x4074ab:	movaps XMMWORD PTR [rcx-0x1f],xmm6
     *    0x4074af:	movaps XMMWORD PTR [rcx-0x2f],xmm5
     *    0x4074b3:	movaps XMMWORD PTR [rcx-0x3f],xmm4
     *    0x4074b7:	movaps XMMWORD PTR [rcx-0x4f],xmm3
     *    0x4074bb:	movaps XMMWORD PTR [rcx-0x5f],xmm2
     *    0x4074bf:	movaps XMMWORD PTR [rcx-0x6f],xmm1
     *    0x4074c3:	movaps XMMWORD PTR [rcx-0x7f],xmm0
     *    0x4074c7:
	 */

	
	void PIC_switch_icc_computed_iterate(
		FileIR_t *firp, 
		Instruction_t *indexing_operation,
		Instruction_t *computed_base_insn,
		Instruction_t *dispatch_insn,
		int32_t table_entry_multiplier
	)
	{

		// sanity check the indexing operation.
		const auto indexing_operation_dis = DecodedInstruction_t::factory(indexing_operation);
		if(indexing_operation_dis->getMnemonic() != "lea") return;
		const auto indexing_operand = indexing_operation_dis->getOperand(1);
		if(indexing_operand->hasMemoryDisplacement() && indexing_operand->getMemoryDisplacement()!=0x0) return;
		if(indexing_operand->hasBaseRegister()) return;
		if(!indexing_operand->hasIndexRegister()) return;

		// sanity check the computed_base.
		const auto computed_base_dis = DecodedInstruction_t::factory(computed_base_insn);
		if(computed_base_dis->getMnemonic() != "mov") return;
		if(!computed_base_dis->getOperand(0)->isRegister()) return;
		if(!computed_base_dis->getOperand(1)->isConstant()) return;

		// check the table multipler is in fact negative.
		// todo:  support positive multipliers?
		if(table_entry_multiplier !=- 1) return;


		// code matches our example above!
		cout << "pic64: found icc computed jump at " 
		     << to_hex_string(dispatch_insn->getAddress()->getVirtualOffset()) << "@" << dispatch_insn->getDisassembly() << "\n"; 

		// extract the fields we need.
		const auto computed_base_address = computed_base_dis->getOperand(1)->getConstant();
		const auto index_size = indexing_operand->getScaleValue();


		// init a jumptables entry
		jmptables[dispatch_insn].SetTableStart(0);
		jmptables[dispatch_insn].SetTableEntrySize(0);
		jmptables[dispatch_insn].SetTableMultiplier(table_entry_multiplier);
		jmptables[dispatch_insn].setAnalysisStatus(iasAnalysisIncomplete);

		auto ibtargets = InstructionSet_t();
		for(auto next_computed_address = computed_base_address ; true; next_computed_address += (int32_t(index_size) * table_entry_multiplier))
		{
			const auto ibtarget = lookupInstruction(next_computed_address);
			if(!ibtarget)  break;
			possible_target(next_computed_address,0, ibt_provenance_t::ibtp_switchtable_type7);
			ibtargets.insert(ibtarget);
		}
		jmptables[dispatch_insn].addTargets(ibtargets);
		cout << "pic64: done icc computed jump \n";

	}


	/*
	 *	Iterate a pic64 switch table.
	 * 	firp -- fileIR
	 * 	exeiop -- pointer to the exeio representing the file we are transforming
	 * 	switch_dispatch -- the switch dispatch instruction, aka I8
	 * 	lea_for_table_base -- the instruction that sets a regsiter to the the table base address. AKA I5/D5
	 * 	table_load_instruction -- the instruction that loads from the switch  table aka I6/D6
	 *
	 */
	void PIC_switch_table64_iterate_table(
		FileIR_t *firp, Instruction_t *dispatch_insn,
		EXEIO::exeio *exeiop,
		Instruction_t *lea_for_table_base,
		Instruction_t *table_load_instruction,
		int32_t table_entry_multiplier,
		const ibt_provenance_t &switch_prov,
		const string &cmp_str,
		const string &bound_stopif,
		const string &and_str,
		const bool allow_unpins)
	{
		auto table_load_disasm = DecodedInstruction_t::factory(lea_for_table_base);

		if (!(table_load_disasm->getOperand(1)->isMemory()))
			return;
		if (!(table_load_disasm->getOperand(1)->isPcrel()))
			return;

		// note that we'd normally have to add the displacement to the
		// instruction address (and include the instruction's size, etc.
		// but, fix_calls has already removed this oddity so we can relocate
		// the instruction.
		auto D1 = VirtualOffset_t(strtol(table_load_disasm->getOperand(1)->getString().c_str(), nullptr, 0));
		D1 += lea_for_table_base->getAddress()->getVirtualOffset();

		// sometimes the lea only points at the image base, and the displacement field here is used for
		// the offset into the image.  This is useful if there are multiple switches (or other constructs)
		// in the same function which can share register assignment of the image-base register.
		// we record the d6_displ field here
		const auto d6 = DecodedInstruction_t::factory(table_load_instruction);
		const auto d6_op1 = d6->getOperand(1);
		const auto d6_op1_is_mem = d6_op1->isMemory();
		const auto d6_displ = d6_op1_is_mem ? d6->getOperand(1)->getMemoryDisplacement() : 0;
		const auto table_entry_size = d6_op1_is_mem ? d6->getOperand(1)->getArgumentSizeInBytes() : 4;

		// find the section with the data table
		const auto table_start_address = VirtualOffset_t(D1+d6_displ);
		const auto pSec = find_section(table_start_address, exeiop);
		if (!pSec)
			return;

		// record for unpinning.
		jmptables[dispatch_insn].SetTableStart(table_start_address);
		jmptables[dispatch_insn].SetTableEntrySize(table_entry_size);
		jmptables[dispatch_insn].SetTableMultiplier(table_entry_multiplier);

		// wait until we scan the table to know if we are complete or not.
		jmptables[dispatch_insn].setAnalysisStatus(iasAnalysisIncomplete);

		// if the section has no data, abort
		const char *secdata = pSec->get_data();
		if (!secdata)
			return;

		//
		// Setting default to 255 without a great reason.  The bad reason
		// is that 255 is pretty big for a switch table, and we very likely shouldn't
		// scan for one bigger than that.  Without this limit, there are some types of switch tables
		// that would scan until the end of the section that the table is in.  (e.g.,
		// Visual Studio will produce tables with 1-byte entries, and it's highly likely that
		// 1-byte offsets from the table base will result in a valid instruction address.
		// Thus, this default is sane for most cases, and is only applied when we absolutely
		// cannot find a bounds check on the table size.
		//
		auto table_size = 255U;
		auto found_table_size = false;
		auto I1 = static_cast<Instruction_t *>(nullptr);
		if (backup_until(cmp_str.c_str(), I1, table_load_instruction, bound_stopif,"^jne$|^je$|^jeq$"))
		{
			const auto d1 = DecodedInstruction_t::factory(I1);
			table_size = d1->getImmediate();

			// notes on table size:
			// readelf on ubuntu20 has a table size of 4.
			cout << "pic64: found cmp-type I1 ('" << d1->getDisassembly() << "'), with table_size=" << table_size << "\n";
			found_table_size = true;
		}
		else if (backup_until(and_str.c_str(), I1, table_load_instruction, bound_stopif))
		{
			const auto d1 = DecodedInstruction_t::factory(I1);
			const auto d1SecondOp = d1->getOperand(1);
			const auto isConstantAnd = d1SecondOp->isConstant();
			if (isConstantAnd)
			{
				table_size = findNextPowerOf2(d1SecondOp->getConstant());
				found_table_size = true;
				cout << "pic64: found and-type I1 ('" << d1->getDisassembly() << "') with table-size = " << dec << table_size << "\n";
			}
			else
			{
				cout << "pic64: found and-type I1 ('" << d1->getDisassembly() << "'), but could not find size of switch table"
					 << "\n";
			}
		}
		else
		{
			// it's very common for the bound_stopif backup to stop before finding the compare
			// because of a code pattern like this:
			//
			// cmp rax, 0x1234
			// ...
			// mov rbx, rax
			// mov ..., [ ... rbx*4 ...]
			//
			// As you can see, the mov rbx,rax would cause backup_until to stop there,
			// missing the compare.
			// For now, we tolerate this and let the no-table-size code find the table size
			// but it might be useful to look for mov/movzx that did a register rename before
			// the compare.
			cout << "pic64: could not find size of switch table" << endl;
		}

		const auto table_start_addr_it = direct_addresses.find(table_start_address);
		const auto da_end = direct_addresses.end();
		const auto next_direct_address_it = table_start_addr_it != da_end ? next(table_start_addr_it) : da_end;
		if(next_direct_address_it != da_end)
		{
			const auto next_direct_address = *next_direct_address_it;
			const auto dont_overflow_size = static_cast<uint32_t>((next_direct_address - table_start_address) / table_entry_size)-1;
			table_size = min(table_size, dont_overflow_size);
			cout << "pic64: clamped switch table via direct_address match of address=" 
			     << hex << next_direct_address << '\n';
		}
		cout << "pic64: Setting table size = " << dec << table_size << '\n';

		// record the set of ibtargets we find here.
		auto ibtargets = InstructionSet_t();

		// offset from address to access - section address
		auto offset = D1 + d6_displ - pSec->get_address();
		auto entry = 0U;
		auto found_table_error = false;
		auto newInstructions = InstructionSet_t();
		do
		{
			// check that we can still grab a word from this section
			if ((int)(offset + table_entry_size) > (int)pSec->get_size())
			{
				found_table_error = true;
				break;
			}

			const auto table_entry_ptr = reinterpret_cast<const char *>(&(secdata[offset]));
			const auto raw_table_entry =
				table_entry_size == 1 ? VirtualOffset_t(*reinterpret_cast<const int8_t *>(table_entry_ptr)) : table_entry_size == 2 ? VirtualOffset_t(*reinterpret_cast<const int16_t *>(table_entry_ptr))
																										  : table_entry_size == 4	? VirtualOffset_t(*reinterpret_cast<const int32_t *>(table_entry_ptr))
																										  : table_entry_size == 8	? VirtualOffset_t(*reinterpret_cast<const int64_t *>(table_entry_ptr))
																																	: throw new invalid_argument("Cannot detect displacement size to load value ");
			const auto table_entry = raw_table_entry * table_entry_multiplier;

			if (!possible_target(D1 + table_entry, 0 /* from addr unknown */, switch_prov))
			{
				if (getenv("IB_VERBOSE"))
				{
					cout << "Found table entry " << hex << D1 << "[" << dec << entry << "] is invalid." << endl;
				}
				found_table_error = true;
				break;
			}

			const auto ibtarget = doDisassemblyForIBT(
				firp,									  // the IR.
				newInstructions,						  // output: the set of instructions that
				D1 + table_entry,						  // jump target from table.:w
				dispatch_insn->getFunction(),			  // the function to add new instructions to.
				dispatch_insn->getAddress()->getFileID(), // the file to add to.
				exeiop,									  // a handle to the executable so we can read raw data.
				true,									  // do pin
				pSec->isExecutable()					  // table in text
			);
			if (ibtarget)
			{
				if (getenv("IB_VERBOSE"))
				{
					cout << "jmp table [" << entry << "]: " << hex << table_entry << dec << endl;
					cout << "Found possible table entry, at: " << std::hex << dispatch_insn->getAddress()->getVirtualOffset()
						 << " insn: " << table_load_disasm->getDisassembly() << " d1: "
						 << D1 << " table_entry:" << table_entry
						 << " target: " << D1 + table_entry << std::dec << endl;
				}
				ibtargets.insert(ibtarget);
			}
			offset += table_entry_size;
			entry++;
		} while (entry <= table_size);

		// record the max entry we found for unpinning.
		const auto max_valid_table_entry = entry;
		jmptables[dispatch_insn].SetTableSize(max_valid_table_entry);

		// valid switch table? may or may not have default: in the switch
		// table size = 8, #entries: 9 b/c of default
		cout << "pic64: max-table-entry (max_int means no found): 0x" << hex << table_size << " #entries: 0x" << entry << " ibtargets.size: " << ibtargets.size() << endl;
		jmptables[dispatch_insn].addTargets(ibtargets);

		// note that there may be an off-by-one error here as table size depends on whether instruction I2 is a jb or jbe.

		// If we've successfully found a table with at least 3 things, or we (oddly) found a table with
		// less than 3 things but did so 100% effectively, go ahead and mark it a successful analysis
		if (!found_table_error || ibtargets.size() > 3)
		{
			if (!found_table_error && found_table_size)
			{
				cout << "pic64: found complete switch table for " << hex << dispatch_insn->getAddress()->getVirtualOffset()
					 << " detected ibtp_switchtable_type4" << endl;
				addSwitchTableScoop(firp, max_valid_table_entry, table_entry_size, D1 + d6_displ, exeiop, table_load_instruction, D1, allow_unpins);
				if (allow_unpins)
					jmptables[dispatch_insn].setAnalysisStatus(iasAnalysisComplete);
			}
			else
			{
				cout << "pic64: found incomplete switch table for " << hex << dispatch_insn->getAddress()->getVirtualOffset()
					 << " detected ibtp_switchtable_type4." 
				     << " found_table_error=" << found_table_error
					 << endl;
				addSwitchTableScoop(firp, max_valid_table_entry, table_entry_size, D1 + d6_displ, exeiop, table_load_instruction, D1, false);
			}
		}
		else
		{
			cout << "pic64: INVALID switch table detected for " << hex
				 << dispatch_insn->getDisassembly() << "@" << dispatch_insn->getAddress()->getVirtualOffset()
				 << " type=ibtp_switchtable_type4 with lea_insn="
				 << lea_for_table_base->getAddress()->getVirtualOffset() << "@" << lea_for_table_base->getDisassembly()
				 << " found_table_error=" << found_table_error
				 << " ibtargets.size() == " << ibtargets.size()
				 << endl;
			// try the next L5.
			return;
		}
		/*
		 * Now, check to see if this was a two level table.
		 * Here's an example from a visual studio compiled program:
		 *
		 *                 cmp    eax,0x23
		 *                 ja     0x1400066d3
		 *    lea_for_table_base:      lea    rcx,[rip+0xffffffffffff9a7d]        # 0x140000000
		 *    I6_2         movzx  eax,BYTE PTR [rcx+rax*1+0x6a50]
		 *    I6:          mov    edx,DWORD PTR [rcx+rax*4+0x6a34]
		 *    I7:          add    rdx,rcx
		 *    I8:          jmp    rdx
		 *
		 */

		// sanity check that I understand the variables of this function properly.
		// and grab the index reg
		assert(table_load_instruction);

		if (d6_op1_is_mem)
		{
			// hack approved by an7s to convert a field from the index register to the actual 32-bit register from RegID_t
			const auto ireg_no = RegisterID_t(rn_EAX + d6_op1->getIndexRegister());
			const auto ireg_str = registerToSearchString(ireg_no);
			const auto I6_2_opcode_str = string() + "movzx " + ireg_str + ",";
			const auto stopif_reg_no = RegisterID_t(rn_RAX + d6_op1->getIndexRegister());
			const auto stopif_reg_str = registerToSearchString(stopif_reg_no);
			const auto stop_if = string() + "^" + stopif_reg_str + "$";

			auto I6_2 = (Instruction_t *)nullptr;
			if (backup_until(I6_2_opcode_str, I6_2, table_load_instruction, stop_if))
			{
				// woo!  found a 2 level table
				// decode d6_2 and check the memory operand
				const auto d6_2 = DecodedInstruction_t::factory(I6_2);
				const auto d6_2_memop = d6_2->getOperand(1);
				if (!d6_2_memop->isMemory())
					return;

				const auto d6_2_displ = d6_2_memop->getMemoryDisplacement();

				// try next L5 if no 2 level table here
				if (d6_2_displ == 0)
					return;

				// look up the section and try next L5 if not found
				const auto lvl2_table_addr = D1 + d6_2_displ;
				const auto lvl2_table_sec = find_section(lvl2_table_addr, exeiop);
				if (lvl2_table_sec == nullptr)
					return;

				const auto lvl2_table_secdata = pSec->get_data();

				// if the section has no data, abort
				if (lvl2_table_secdata == nullptr)
					return;

				// now, scan the lvl2 table, and stop if we find an entry bigger than
				// the lvl1 table's size.  This will calc the size of the lvl2 table.
				//
				// offset from address to access - section address
				auto lvl2_table_offset = lvl2_table_addr - lvl2_table_sec->get_address();
				auto lvl2_table_entry_no = 0U;
				do
				{
					// check that we can still grab a word from this section
					if ((int)(lvl2_table_offset + sizeof(int)) > (int)lvl2_table_sec->get_size())
						break;

					const auto lvl2_table_entry_ptr = (const uint8_t *)&(lvl2_table_secdata[lvl2_table_offset]);
					const auto lvl2_table_entry_val = *lvl2_table_entry_ptr;

					// found an entry that's bigger than our lvl1 table's max element
					if (lvl2_table_entry_val > max_valid_table_entry)
						break;

					lvl2_table_offset += sizeof(uint8_t);
					lvl2_table_entry_no++;
				} while (lvl2_table_entry_no <= table_size); /* table_size from original cmp! */

				// now record the lvl2 table into a scoop
				addSwitchTableScoop(firp, lvl2_table_entry_no + 1, 1, lvl2_table_addr, exeiop, I6_2, D1, false);
			}
		}
	}

	/*
	 * Add a switch table in code.
	 */
	void addSwitchTableScoop(
		FileIR_t *firp,						   // the IR to modify
		const size_t num_entries,			   // how many entries in the switch table.
		const size_t entry_size,			   // how many bytes in an entry.
		const VirtualOffset_t table_base_addr, // the original address of the table.
		EXEIO::exeio *exeiop,				   // a handle to the executable to read bytes from.
		Instruction_t *table_ref,			   // an instruction that depends on the architecture type.
		const VirtualOffset_t table_base_addr_without_disp,
		// the constant in table_ref used to offset into the table.
		const bool do_unpin = true // should we even try to unpin this switch table?
	)
	{
		// make sure we can find the section with the switch table
		const auto sec = exeiop->sections.findByAddress(table_base_addr); // section that contains the data

		// and only add if the section is executable.
		// data sections already have scoops (consider splitting data scoops?)
		if (sec == nullptr)
			return;
		if (!sec->isExecutable())
			return;

		const auto start_vo = do_unpin ? 0u : table_base_addr; // start and end offsets in this file
		const auto end_vo = start_vo + num_entries * entry_size - 1;
		auto startaddr = firp->addNewAddress(firp->getFile()->getBaseID(), start_vo); // start and end address
		auto endaddr = firp->addNewAddress(firp->getFile()->getBaseID(), end_vo);
		assert(sec);
		const auto sec_data = sec->get_data();											// data
		const auto scoop_start_ptr = sec_data + (table_base_addr - sec->get_address()); // relevant start of data
		const auto the_contents = string(scoop_start_ptr, end_vo - start_vo + 1);
		const auto name = string("fii_switch_") + to_hex_string(table_base_addr); // name of new segment
		const auto permissions =												  // permissions and relro bit.
			(sec->isReadable() << 2) |
			(sec->isWriteable() << 1) |
			(sec->isExecutable() << 0);
		const auto is_relro = false;

		// finally, create the new scoop
		const auto switch_tab = firp->addNewDataScoop(name, startaddr, endaddr, NULL, permissions, is_relro, the_contents, max_base_id++);

		const auto mt = firp->getArchitecture()->getMachineType();

		// how to pin the scoop
		const auto repinScoop = [&]()
		{
			startaddr->setVirtualOffset(startaddr->getVirtualOffset() + table_base_addr);
			endaddr->setVirtualOffset(endaddr->getVirtualOffset() + table_base_addr);
		};

		if (do_unpin)
		{
			if (mt == admtX86_64 || mt == admtI386)
			{
				// on x86 we need to rewrite the instruction that loads from the table.
				// Rewrite this for x86-64 pic switch tables.
				// "I6" is the memory instruction that loads from the table.
				auto d6 = DecodedInstruction_t::factory(table_ref);
				auto operands = d6->getOperands();
				auto the_arg = find_if(ALLOF(operands), [](const shared_ptr<DecodedOperand_t> &arg)
									   { return arg->isMemory(); });
				const auto found_arg = the_arg != operands.end();

				// I6 really needs a memory operand, or we just have to repin.
				if (!found_arg)
				{
					// repin the scoop, first
					cout << "Warning:  I6 not a memory access?  Repinning switch table\n";
					repinScoop();
					return;
				}

				// if we have a move operation, we need to check if it has a 4-byte displacement field.
				// If not, we need to modify the move so that it does.
				if (!(*the_arg)->hasMemoryDisplacement() || (*the_arg)->getMemoryDisplacementEncodingSize() != 4)
				{
					const auto oldInsnBits = table_ref->getDataBits();

					auto byteIndex = 0u;
					auto newInsnBits = string();

					// check for and copy rex prefix.
					if ((oldInsnBits[byteIndex] & 0x40) == 0x40)
					{
						newInsnBits += oldInsnBits[byteIndex++];
					}
					if (uint8_t(oldInsnBits[byteIndex]) != uint8_t(0x8a) && uint8_t(oldInsnBits[byteIndex]) != uint8_t(0x8b))
					{
						cout << "Warning:  I6 not a move (opcode=8a or 8b) ?  Repinning switch table\n";
						repinScoop();
						return;
					}
					// copy opcode
					newInsnBits += oldInsnBits[byteIndex++];

					// decode mod/reg/rm byte
					const auto modRegRmByte = uint8_t(oldInsnBits[byteIndex]);
					const auto modBits = (modRegRmByte >> 6) & 0b11;  // bits 7-6
					const auto regBits = (modRegRmByte >> 3) & 0b111; // bits 5-3
					const auto rmBits = (modRegRmByte >> 0) & 0b111;  // bits 2-0

					// check that there's a SIB byte.
					// since we're expecting I6 to be the load from table, there should be a base and index register,
					// requiring a SIB byte.
					if (rmBits != 0b100)
					{
						cout << "Warning:  I6 not a mem. move with mod==0b00, rm=0b100)?  Repinning switch table\n";
						repinScoop();
						return;
					}

					// recreate ModRegRM byte
					const auto newModRegRmByte = (0b10 << 6) | (regBits << 3) | (rmBits << 0);
					newInsnBits += newModRegRmByte;

					// skip the old modRegRM byte
					byteIndex++;

					// copy the SIB byte.
					newInsnBits += oldInsnBits[byteIndex++];

					// add a new displacement field, but that depends on the old disp, which may be a disp8.
					switch (modBits)
					{
					case 0b00:
					{

						// add 4 byte displacement
						newInsnBits += string("\0\0\0\0", 4);

						// good to go!
						break;
					}
					case 0b01:
					{

						// get, sign extend, and add 4 byte displacement
						const auto oldDisp = int8_t(oldInsnBits[byteIndex]);
						const auto newDisp = int32_t(oldDisp);
						newInsnBits += string(reinterpret_cast<const char *>(&newDisp), 4);
						break;
					}
					default:
						throw runtime_error("Invalid decoding of instruction?");
					}

					// set the new data bits to have a 32-bit displacement,
					// and re-decode the isntruction for the code below.
					table_ref->setDataBits(newInsnBits);
					d6 = DecodedInstruction_t::factory(table_ref);
					operands = d6->getOperands();
					the_arg = find_if(ALLOF(operands), [](const shared_ptr<DecodedOperand_t> &arg)
									  { return arg->isMemory(); });
				}
				assert((*the_arg)->getMemoryDisplacementEncodingSize() == 4);

				// Finally, mark table_ref as referencing the scoop, and add a relocation so we can later repin the scoop
				firp->addNewRelocation(table_ref, 0, "absoluteptr_to_scoop", switch_tab, -table_base_addr_without_disp);
			}
			else if (mt == admtArm32 || mt == admtAarch64)
			{
				// on ARM we need need to update the pc-rel relocation to indicate which scoop is the table.
				for (auto &reloc : table_ref->getRelocations())
				{
					if (reloc->getType() == "pcrel")
					{
						assert(reloc->getWRT() == nullptr);
						reloc->setWRT(switch_tab);
					}
				}
			}
			else
			{
				// unknown arch.
				assert(0);
			}
		}
	}

	/*
	  switch table pattern (non-PIC):
		  40062a:	8d 40 ec             	lea  eax,[rax-0x14]
	  I1: 40062d:	83 f8 08             	cmp  eax,0x8                     'size
	  I2: 400630:	0f 87 98 00 00 00    	ja   4006ce <main+0xbb>
		  400636:	89 c0                	mov  eax,eax
	  I3: 400638:	ff 24 c5 10 08 40 00	jmp  QWORD PTR [rax*8+0x400810]  'branch
		  40063f:	bf a9 07 40 00       	mov  edi,0x4007a9
		  400644:	e8 67 fe ff ff       	call 4004b0 <puts@plt>
		  400649:	e9 96 00 00 00       	jmp  4006e4 <main+0xd1>

		nb: handles both 32 and 64 bit
	*/
	void check_for_nonPIC_switch_table_pattern2(FileIR_t *firp, Instruction_t *insn, const DecodedInstruction_t &p_disasm, EXEIO::exeio *exeiop)
	{
		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type5;
		Instruction_t *I1 = nullptr;
		Instruction_t *IJ = insn;

		assert(IJ);

		// check if IJ is a jump
		if (strstr(p_disasm.getMnemonic().c_str(), "jmp") == nullptr)
			return;

		// look for a memory type
		if (!(p_disasm.getOperand(0)->isMemory()))
			return;

		// make sure there's a scaling factor
		if (!p_disasm.getOperand(0)->hasIndexRegister() || p_disasm.getOperand(0)->getScaleValue() < 4)
			return;

		// extract start of jmp table
		VirtualOffset_t table_offset = p_disasm.getAddress(); // disasm.Instruction.AddrValue;
		if (table_offset == 0)
			return;

		cout << hex << "(nonPIC-pattern2): Found switch dispatch at 0x" << hex << IJ->getAddress()->getVirtualOffset() << " with table_offset=" << hex << table_offset << dec << endl;

		if (!backup_until("cmp", I1, IJ))
		{
			cout << "(nonPIC-pattern2): could not find size of switch table" << endl;
			return;
		}

		// extract size off the comparison
		// make sure not off by one
		auto d1p = DecodedInstruction_t::factory(I1);
		auto &d1 = *d1p;
		VirtualOffset_t table_size = d1.getImmediate();

		if (table_size <= 0)
			return;

		cout << "(nonPIC-pattern2): size of jmp table: " << table_size << endl;

		// find the section with the data table
		auto pSec = find_section(table_offset, exeiop);
		if (!pSec)
		{
			return;
		}

		// if the section has no data, abort
		const char *secdata = pSec->get_data();
		if (!secdata)
			return;

		// get the base offset into the section
		auto offset = table_offset - pSec->get_address();
		auto i = 0U;

		InstructionSet_t ibtargets;
		for (i = 0; i < table_size; ++i)
		{
			if ((int)(offset + i * arch_ptr_bytes() + sizeof(int)) > (int)pSec->get_size())
			{
				cout << "jmp table outside of section range ==> invalid switch table" << endl;
				return;
			}

			const VirtualOffset_t *table_entry_ptr = (const VirtualOffset_t *)&(secdata[offset + i * arch_ptr_bytes()]);
			VirtualOffset_t table_entry = *table_entry_ptr;
			possible_target(table_entry, 0, prov);

			auto ibtarget = lookupInstruction(table_entry);
			if (!ibtarget)
			{
				if (getenv("IB_VERBOSE"))
					cout << "0x" << hex << table_entry << " is not an instruction, invalid switch table" << endl;
				return;
			}

			if (getenv("IB_VERBOSE"))
				cout << "jmp table [" << i << "]: " << hex << table_entry << dec << endl;
			ibtargets.insert(ibtarget);
		}

		cout << "(non-PIC) valid switch table found - ibtp_switchtable_type5" << endl;

		jmptables[IJ].addTargets(ibtargets);
		jmptables[IJ].setAnalysisStatus(iasAnalysisComplete);
	}

	/*
	  Handles the following switch table pattern:
	  I1: 400518:   83 7d ec 0c             cmpl   $0xc,-0x14(%rbp)          'size
	  I2: 40051c:   0f 87 b7 00 00 00       ja     4005d9 <main+0xe5>
	  I3: 400522:   8b 45 ec                mov    -0x14(%rbp),%eax
	  I4: 400525:   48 8b 04 c5 20 07 40    mov    0x400720(,%rax,8),%rax    'start jump table
		  40052c:   00
	  IJ: 40052d:   ff e0                   jmpq   *%rax                     'indirect branch
		  40052f:   c7 45 f4 00 00 00 00    movl   $0x0,-0xc(%rbp)
		  400536:   c7 45 f8 03 00 00 00    movl   $0x3,-0x8(%rbp)

		nb: handles both 32 and 64 bit
	*/
	void check_for_nonPIC_switch_table(FileIR_t *firp, Instruction_t *insn, const DecodedInstruction_t &p_disasm, EXEIO::exeio *exeiop)
	{
		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type6;
		Instruction_t *I1 = nullptr;
		Instruction_t *I4 = nullptr;
		Instruction_t *IJ = insn;

		if (!IJ)
			return;

		// check if IJ is a jump
		if (strstr(p_disasm.getMnemonic().c_str(), "jmp") == nullptr)
			return;

		// return if it's a jump to a constant address, these are common
		if (p_disasm.getOperand(0)->isConstant())
			return;

		// return if it's a jump to a memory address
		if (p_disasm.getOperand(0)->isMemory())
			return;

		// has to be a jump to a register now

		// backup and find the instruction that's a mov
		if (!backup_until("mov", I4, IJ))
			return;

		// extract start of jmp table
		auto d4p = DecodedInstruction_t::factory(I4);
		auto &d4 = *d4p;

		// make sure there's a scaling factor
		if (d4.getOperand(1)->isMemory() && d4.getOperand(1)->getScaleValue() < 4)
			return;

		VirtualOffset_t table_offset = d4.getAddress();
		if (table_offset == 0)
			return;

		if (getenv("IB_VERBOSE"))
			cout << hex << "(nonPIC): Found switch dispatch at 0x" << hex << I4->getAddress()->getVirtualOffset() << " with table_offset=" << hex << table_offset << dec << endl;

		if (!backup_until("cmp", I1, I4))
		{
			cout << "(nonPIC): could not find size of switch table" << endl;
			return;
		}

		// extract size off the comparison
		// make sure not off by one
		auto d1p = DecodedInstruction_t::factory(I1);
		auto &d1 = *d1p;
		auto table_size = d1.getImmediate();
		if (table_size <= 0)
			return;

		if (getenv("IB_VERBOSE"))
			cout << "(nonPIC): size of jmp table: " << table_size << endl;

		// find the section with the data table
		auto pSec = find_section(table_offset, exeiop);
		if (!pSec)
		{
			cout << hex << "(nonPIC): could not find jump table in section" << endl;
			return;
		}

		// if the section has no data, abort
		const char *secdata = pSec->get_data();
		if (!secdata)
			return;

		// get the base offset into the section
		auto offset = table_offset - pSec->get_address();
		auto i = 0U;

		if (getenv("IB_VERBOSE"))
			cout << hex << "offset: " << offset << " arch bit width: " << dec << firp->getArchitectureBitWidth() << endl;

		InstructionSet_t ibtargets;
		for (i = 0; i < table_size; ++i)
		{
			if ((int)(offset + i * arch_ptr_bytes() + sizeof(int)) > (int)pSec->get_size())
			{
				cout << "jmp table outside of section range ==> invalid switch table" << endl;
				return;
			}

			VirtualOffset_t table_entry = 0;
			if (firp->getArchitectureBitWidth() == 32)
			{
				const int *table_entry_ptr = (const int *)&(secdata[offset + i * arch_ptr_bytes()]);
				table_entry = *table_entry_ptr;
			}
			else if (firp->getArchitectureBitWidth() == 64)
			{
				const VirtualOffset_t *table_entry_ptr = (const VirtualOffset_t *)&(secdata[offset + i * arch_ptr_bytes()]);
				table_entry = *table_entry_ptr;
			}
			else
				assert(0 && "Unknown arch size.");

			possible_target(table_entry, 0 /* from addr unknown */, prov);
			auto ibtarget = lookupInstruction(table_entry);
			if (!ibtarget)
			{
				if (getenv("IB_VERBOSE"))
					cout << "0x" << hex << table_entry << " is not an instruction, invalid switch table" << endl;
				return;
			}

			if (getenv("IB_VERBOSE"))
				cout << "jmp table [" << i << "]: " << hex << table_entry << dec << endl;
			ibtargets.insert(ibtarget);
		}

		cout << "(non-PIC) valid switch table found - prov=ibt_provenance_t::ibtp_switchtable_type6" << endl;
		jmptables[IJ].addTargets(ibtargets);
		jmptables[IJ].setAnalysisStatus(iasAnalysisComplete);
	}

	void handle_takes_address_annot(FileIR_t *firp, Instruction_t *insn, MEDS_TakesAddressAnnotation *p_takes_address_annotation)
	{
		const auto referenced_addr = p_takes_address_annotation->GetReferencedAddress();
		if (p_takes_address_annotation->isCode())
		{
			const auto refd_addr = referenced_addr;
			possible_target(refd_addr, insn->getAddress()->getVirtualOffset(), ibt_provenance_t::ibtp_text);
		}
		else
		{
			all_add_adrp_results[insn->getFunction()].insert(referenced_addr);
		}
	}

	void handle_ib_annot(FileIR_t *firp, Instruction_t *insn, MEDS_IBAnnotation *p_ib_annotation)
	{
		if (p_ib_annotation->IsComplete())
		{
			jmptables[insn].setAnalysisStatus(iasAnalysisComplete);
		}
	}
	void handle_ibt_annot(FileIR_t *firp, Instruction_t *insn, MEDS_IBTAnnotation *p_ibt_annotation)
	{
		/*
		 * ibt_prov reason codes
		 *              static const provtype_t ibtp_stars_ret=1<<11;
		 *              static const provtype_t ibtp_stars_switch=1<<12;
		 *              static const provtype_t ibtp_stars_data=1<<13;
		 *              static const provtype_t ibtp_stars_unknown=1<<14;
		 *              static const provtype_t ibtp_stars_addressed=1<<15;
		 *              static const provtype_t ibtp_stars_unreachable=1<<15;
		 */
		/* meds annotations
		 *                typedef enum { SWITCH, RET, DATA, UNREACHABLE, ADDRESSED, UNKNOWN } ibt_reason_code_t;
		 */
		// cout<<"at handl_ibt with addr="<<hex<<insn->getAddress()->getVirtualOffset()<<" code="<<p_ibt_annotation->GetReason()<<endl;
		switch (p_ibt_annotation->GetReason())
		{
		case MEDS_IBTAnnotation::SWITCH:
		case MEDS_IBTAnnotation::INDIRCALL:
		{
			possible_target((VirtualOffset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
							0, ibt_provenance_t::ibtp_stars_switch);
			auto addr = (VirtualOffset_t)p_ibt_annotation->GetXrefAddr();
			auto fromib = lookupInstruction(addr);
			auto ibt = lookupInstruction(p_ibt_annotation->getVirtualOffset().getOffset());
			if (fromib && ibt)
			{
				if (getenv("IB_VERBOSE") != nullptr)
					cout << hex << "Adding call/switch icfs: " << fromib->getAddress()->getVirtualOffset() << "->" << ibt->getAddress()->getVirtualOffset() << endl;
				jmptables[fromib].insert(ibt);
			}
			else
			{
				cout << "Warning:  cannot find source or dest for call/switch icfs." << endl;
			}
			break;
		}
		case MEDS_IBTAnnotation::RET:
		{
			/* we are not going to mark return points as IBTs yet.  that's fix-calls job */
			// possible_target((VirtualOffset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
			// 	0,ibt_provenance_t::ibtp_stars_ret);

			auto fromaddr = (VirtualOffset_t)p_ibt_annotation->GetXrefAddr();
			auto fromib = lookupInstruction(fromaddr);
			auto toaddr = p_ibt_annotation->getVirtualOffset().getOffset();
			auto ibt = lookupInstruction(toaddr);
			if (fromib && ibt)
			{
				if (getenv("IB_VERBOSE") != nullptr)
					cout << hex << "Adding ret icfs: " << fromib->getAddress()->getVirtualOffset() << "->" << ibt->getAddress()->getVirtualOffset() << endl;
				jmptables[fromib].insert(ibt);
			}
			else
			{
				cout << "Warning:  cannot find source (" << hex << fromaddr << ") or dest (" << hex << toaddr << ") for ret icfs." << endl;
			}
			break;
		}
		case MEDS_IBTAnnotation::DATA:
		{
			possible_target((VirtualOffset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
							0, ibt_provenance_t::ibtp_stars_data);
			if (getenv("IB_VERBOSE") != nullptr)
				cout << hex << "detected stars data ibt at" << p_ibt_annotation->getVirtualOffset().getOffset() << endl;
			break;
		}
		case MEDS_IBTAnnotation::UNREACHABLE:
		{
			possible_target((VirtualOffset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
							0, ibt_provenance_t::ibtp_stars_unreachable);
			if (getenv("IB_VERBOSE") != nullptr)
				cout << hex << "detected stars unreachable ibt at" << p_ibt_annotation->getVirtualOffset().getOffset() << endl;
			break;
		}
		case MEDS_IBTAnnotation::ADDRESSED:
		{
			possible_target((VirtualOffset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
							0, ibt_provenance_t::ibtp_stars_addressed);
			if (getenv("IB_VERBOSE") != nullptr)
				cout << hex << "detected stars addresssed ibt at" << p_ibt_annotation->getVirtualOffset().getOffset() << endl;
			break;
		}
		case MEDS_IBTAnnotation::UNKNOWN:
		{
			possible_target((VirtualOffset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
							0, ibt_provenance_t::ibtp_stars_unknown);
			if (getenv("IB_VERBOSE") != nullptr)
				cout << hex << "detected stars unknown ibt at" << p_ibt_annotation->getVirtualOffset().getOffset() << endl;
			break;
		}
		default:
		{
			assert(0); // unexpected ibt annotation.
		}
		}
	}

	void read_stars_xref_file(FileIR_t *firp)
	{

		const auto fileBasename = string(basename((char *)firp->getFile()->getURL().c_str()));

		auto annotationParser = MEDS_AnnotationParser();
		// need to map filename to integer annotation file produced by STARS
		// this should be retrieved from the IRDB but for now, we use files to store annotations
		// convention from within the peasoup subdirectory is:
		//      a.ncexe.infoannot
		//      shared_objects/<shared-lib-filename>.infoannot
		const auto annotationFilename = (fileBasename == BINARY_NAME) ? BINARY_NAME : SHARED_OBJECTS_DIR + "/" + fileBasename;

		try
		{
			annotationParser.parseFile(annotationFilename + ".STARSxrefs");
		}
		catch (const string &s)
		{
			cout << "Warning:  annotation parser reports error: " << s << endl;
		}

		for (auto insn : firp->getInstructions())
		{
			const auto irdb_vo = insn->getAddress()->getVirtualOffset();
			const auto vo = MEDS_Annotation::VirtualOffset(irdb_vo);

			/* find it in the annotations */
			const auto ret = annotationParser.getAnnotations().equal_range(vo);

			/* for each annotation for this instruction */
			for (auto ait = ret.first; ait != ret.second; ++ait)
			{
				/* is this annotation a funcSafe annotation? */
				const auto p_ib_annotation = dynamic_cast<MEDS_IBAnnotation *>(ait->second);
				const auto p_ibt_annotation = dynamic_cast<MEDS_IBTAnnotation *>(ait->second);
				const auto p_takes_address_annotation = dynamic_cast<MEDS_TakesAddressAnnotation *>(ait->second);
				if (p_ib_annotation && p_ib_annotation->isValid())
					handle_ib_annot(firp, insn, p_ib_annotation);
				else if (p_ibt_annotation && p_ibt_annotation->isValid())
					handle_ibt_annot(firp, insn, p_ibt_annotation);
				else if (p_takes_address_annotation && p_takes_address_annotation->isValid())
					handle_takes_address_annot(firp, insn, p_takes_address_annotation);
			}
		}
	}

	void process_dynsym(FileIR_t *firp)
	{
		auto dynsymfile = popen("$PS_OBJDUMP -T a.ncexe | $PS_GREP '^[0-9]\\+' | awk '{print $1;}' | $PS_GREP -v '^$'", "r");
		if (!dynsymfile)
		{
			perror("Cannot start pipe to $PS_OBJDUMP a.ncexe");
			exit(2);
		}
		auto target = (unsigned int)0;
		while (fscanf(dynsymfile, "%x", &target) != -1)
		{
			possible_target((VirtualOffset_t)target, 0, ibt_provenance_t::ibtp_dynsym);
		}
	}

	ICFS_t *setup_hellnode(FileIR_t *firp, EXEIO::exeio *exeiop, ibt_provenance_t allowed)
	{
		auto hn = firp->addNewICFS(nullptr, {}, iasAnalysisModuleComplete);

		for (auto insn : firp->getInstructions())
		{
			auto prov = targets[insn->getAddress()->getVirtualOffset()];

			if (prov.isEmpty())
				continue;

			if (prov.isPartiallySet(allowed))
			{
				hn->insert(insn);
			}
		}

		if (hn->size() < 1000 && !exeiop->isDynamicallyLinked())
			hn->setAnalysisStatus(iasAnalysisComplete);

		return hn;
	}

	ICFS_t *setup_call_hellnode(FileIR_t *firp, EXEIO::exeio *exeiop)
	{
		ibt_provenance_t allowed =
			ibt_provenance_t::ibtp_data |
			ibt_provenance_t::ibtp_text |
			ibt_provenance_t::ibtp_stars_addressed |
			ibt_provenance_t::ibtp_unknown |
			ibt_provenance_t::ibtp_stars_unreachable |
			ibt_provenance_t::ibtp_dynsym | // symbol resolved to other module, this module should xfer directly, but could still plt
			ibt_provenance_t::ibtp_rodata |
			ibt_provenance_t::ibtp_initarray | // .init loops through the init_array, and calls them
			ibt_provenance_t::ibtp_finiarray | // .fini loops through the fini_array, and calls them
			ibt_provenance_t::ibtp_user;

		// would like to sanity check better.
		//		ibt_provenance_t::ibtp_stars_data |	// warn if stars reports it's in data, but !allowed.

		/*
		 * these aren't good enough reasons for a call instruction to transfer somewhere.
		 * ibt_provenance_t::ibtp_eh_frame	// only libc should xfer.
		 * ibt_provenance_t::ibtp_gotplt	// only an analyzed jump should xfer.
		 * ibt_provenance_t::ibtp_entrypoint	// only ld.so or kernel should xfer.
		 * ibt_provenance_t::ibtp_texttoprintf	// shouldn't xfer if addr passed to printf.
		 * ibt_provenance_t::ibtp_symtab		// user info only.
		 * ibt_provenance_t::ibtp_stars_ret	// stars says a return goes here, calls shouldn't.
		 * ibt_provenance_t::ibtp_stars_switch	// stars says switch target.
		 * ibt_provenance_t::ibtp_switchtable_type1	// FII switch targets.
		 * ibt_provenance_t::ibtp_switchtable_type2
		 * ibt_provenance_t::ibtp_switchtable_type3
		 * ibt_provenance_t::ibtp_switchtable_type4
		 * ibt_provenance_t::ibtp_switchtable_type5
		 * ibt_provenance_t::ibtp_switchtable_type6
		 * ibt_provenance_t::ibtp_switchtable_type7
		 * ibt_provenance_t::ibtp_switchtable_type8
		 * ibt_provenance_t::ibtp_switchtable_type9
		 * ibt_provenance_t::ibtp_switchtable_type10
		 */

		auto ret = setup_hellnode(firp, exeiop, allowed);

		cout << "# ATTRIBUTE fill_in_indtargs::call_hellnode_size=" << dec << ret->size() << endl;
		return ret;
	}

	ICFS_t *setup_jmp_hellnode(FileIR_t *firp, EXEIO::exeio *exeiop)
	{
		ibt_provenance_t allowed =
			ibt_provenance_t::ibtp_data |
			ibt_provenance_t::ibtp_text |
			ibt_provenance_t::ibtp_stars_addressed |
			ibt_provenance_t::ibtp_unknown |
			ibt_provenance_t::ibtp_stars_unreachable |
			ibt_provenance_t::ibtp_dynsym | // symbol resolved to other module, this module should xfer directly. but that's not true, may still plt.
			ibt_provenance_t::ibtp_rodata |
			ibt_provenance_t::ibtp_gotplt |
			ibt_provenance_t::ibtp_user;

		//		ibt_provenance_t::ibtp_stars_data |	// warn if stars reports it's in data, but !allowed.

		/*
		 * these aren't good enough reasons for a jmp instruction to transfer somewhere.
		 * ibt_provenance_t::ibtp_eh_frame	// only libc should xfer.
		 * ibt_provenance_t::ibtp_initarray	// only ld.so should xfer.
		 * ibt_provenance_t::ibtp_finiarray	// only ld.so should xfer.
		 * ibt_provenance_t::ibtp_entrypoint	// only ld.so or kernel should xfer.
		 * ibt_provenance_t::ibtp_texttoprintf	// shouldn't xfer if addr passed to printf.
		 * ibt_provenance_t::ibtp_symtab		// user info only.
		 * ibt_provenance_t::ibtp_stars_ret	// stars says a return goes here, calls shouldn't.
		 * ibt_provenance_t::ibtp_stars_switch	// stars says switch target.
		 * ibt_provenance_t::ibtp_switchtable_type1	// FII switch targets.
		 * ibt_provenance_t::ibtp_switchtable_type2
		 * ibt_provenance_t::ibtp_switchtable_type3
		 * ibt_provenance_t::ibtp_switchtable_type4
		 * ibt_provenance_t::ibtp_switchtable_type5
		 * ibt_provenance_t::ibtp_switchtable_type6
		 * ibt_provenance_t::ibtp_switchtable_type7
		 * ibt_provenance_t::ibtp_switchtable_type8
		 * ibt_provenance_t::ibtp_switchtable_type9
		 * ibt_provenance_t::ibtp_switchtable_type10
		 */

		auto ret = setup_hellnode(firp, exeiop, allowed);
		cout << "# ATTRIBUTE fill_in_indtargs::jmp_hellnode_size=" << dec << ret->size() << endl;
		return ret;
	}

	ICFS_t *setup_ret_hellnode(FileIR_t *firp, EXEIO::exeio *exeiop)
	{
		ibt_provenance_t allowed =
			ibt_provenance_t::ibtp_stars_ret | // stars says a return goes here, and this return isn't analyzeable.
			ibt_provenance_t::ibtp_unknown |
			ibt_provenance_t::ibtp_stars_unreachable |
			ibt_provenance_t::ibtp_ret | // instruction after a call
			ibt_provenance_t::ibtp_user;

		// would like to sanity check better.
		//		ibt_provenance_t::ibtp_stars_data |	// warn if stars reports it's in data, but !allowed.

		/*
		 * these aren't good enough reasons for a ret instruction to transfer somewhere.
		 * ibt_provenance_t::ibtp_eh_frame	// only libc should xfer.
		 * ibt_provenance_t::ibtp_initarray	// only ld.so should xfer.
		 * ibt_provenance_t::ibtp_finiarray	// only ld.so should xfer.
		 * ibt_provenance_t::ibtp_entrypoint	// only ld.so or kernel should xfer.
		 * ibt_provenance_t::ibtp_texttoprintf	// shouldn't xfer if addr passed to printf.
		 * ibt_provenance_t::ibtp_dynsym		// symbol resolved to other module, this module should xfer directly.
		 * ibt_provenance_t::ibtp_symtab		// user info only.
		 * ibt_provenance_t::ibtp_stars_ret	// stars says a return goes here, calls shouldn't.
		 * ibt_provenance_t::ibtp_stars_switch	// stars says switch target.
		 * ibt_provenance_t::ibtp_switchtable_type1	// FII switch targets.
		 * ibt_provenance_t::ibtp_switchtable_type2
		 * ibt_provenance_t::ibtp_switchtable_type3
		 * ibt_provenance_t::ibtp_switchtable_type4
		 * ibt_provenance_t::ibtp_switchtable_type5
		 * ibt_provenance_t::ibtp_switchtable_type6
		 * ibt_provenance_t::ibtp_switchtable_type7
		 * ibt_provenance_t::ibtp_switchtable_type8
		 * ibt_provenance_t::ibtp_switchtable_type9
		 * ibt_provenance_t::ibtp_switchtable_type10
		 * ibt_provenance_t::ibtp_data  	// returns likely shouldn't be used to jump to data or addressed text chunks.  may need to relax later.
		 * ibt_provenance_t::ibtp_text
		 * ibt_provenance_t::ibtp_stars_addressed
		 * ibt_provenance_t::ibtp_rodata
		 * ibt_provenance_t::ibtp_gotplt
		 */

		auto ret_hell_node = setup_hellnode(firp, exeiop, allowed);
		cout << "# ATTRIBUTE fill_in_indtargs::basicret_hellnode_size=" << dec << ret_hell_node->size() << endl;

		cout << "# ATTRIBUTE fill_in_indtargs::fullret_hellnode_size=" << dec << ret_hell_node->size() << endl;
		return ret_hell_node;
	}

	void mark_return_points(FileIR_t *firp)
	{
		// add unmarked return points.  fix_calls will deal with whether they need to be pinned or not later.
		for (const auto insn : firp->getInstructions())
		{
			const auto d = DecodedInstruction_t::factory(insn);
			if (d->isCall() && insn->getFallthrough())
			{
				targets[insn->getFallthrough()->getAddress()->getVirtualOffset()].add(ibt_provenance_t::ibtp_ret);
			}
		}
	}

	void print_icfs(FileIR_t *firp)
	{
		cout << "Printing ICFS sets." << endl;
		for (const auto insn : firp->getInstructions())
		{
			auto icfs = insn->getIBTargets();

			// not an IB
			if (!icfs)
				continue;

			cout << hex << insn->getAddress()->getVirtualOffset() << " -> ";

			for (auto target : *icfs)
			{
				cout << hex << target->getAddress()->getVirtualOffset() << " ";
			}
			cout << endl;
		}
	}

	void setup_icfs(FileIR_t *firp, EXEIO::exeio *exeiop)
	{
		int total_ibta_set = 0;

		/* setup some IBT categories for warning checking */
		ibt_provenance_t non_stars_data =
			ibt_provenance_t::ibtp_text |
			ibt_provenance_t::ibtp_eh_frame |
			ibt_provenance_t::ibtp_texttoprintf |
			ibt_provenance_t::ibtp_gotplt |
			ibt_provenance_t::ibtp_initarray |
			ibt_provenance_t::ibtp_finiarray |
			ibt_provenance_t::ibtp_data |
			ibt_provenance_t::ibtp_dynsym |
			ibt_provenance_t::ibtp_symtab |
			ibt_provenance_t::ibtp_rodata |
			ibt_provenance_t::ibtp_unknown;
		ibt_provenance_t stars_data = ibt_provenance_t::ibtp_stars_data;

		// setup calls, jmps and ret hell nodes.
		auto call_hell = setup_call_hellnode(firp, exeiop);
		auto jmp_hell = setup_jmp_hellnode(firp, exeiop);
		auto ret_hell = setup_ret_hellnode(firp, exeiop);

		// for each instruction
		for (auto insn : firp->getInstructions())
		{

			// if we already got it complete (via stars or FII)
			if (insn->getIndirectBranchTargetAddress() != nullptr)
				total_ibta_set++;

			// warning check
			ibt_provenance_t prov = targets[insn->getAddress()->getVirtualOffset()];

			// stars calls it data, but printw arning if we didn't find it in data or as a printf addr.
			if (prov.isPartiallySet(stars_data) && !prov.isPartiallySet(non_stars_data))
			{
				// ofstream fout("warning.txt", ofstream::out | ofstream::app);
				cerr << "STARS found an IBT in data that FII wasn't able to classify at " << hex << insn->getAddress()->getVirtualOffset() << "." << endl;
			}

			// create icfs for complete jump tables.
			if (jmptables[insn].isComplete())
			{

				// get the strcuture into the IRDB
				/*
				auto nn=new ICFS_t(jmptables[insn]);
				firp->GetAllICFS().insert(nn);
				insn->SetIBTargets(nn);
				*/
				auto nn = firp->addNewICFS(insn, jmptables[insn], jmptables[insn].getAnalysisStatus());

				if (getenv("IB_VERBOSE") != 0)
				{
					cout << "IB complete for " << hex << insn->getAddress()->getVirtualOffset()
						 << ":" << insn->getDisassembly() << " with " << dec << nn->size() << " targets." << endl;
				}

				// that's all we need to do
				continue;
			}

			// disassemble the instruction, and figure out which type of hell node we need.
			auto d = DecodedInstruction_t::factory(insn);
			if (d->isReturn())
			{
				if (getenv("IB_VERBOSE") != 0)
					cout << "using ret hell node for " << hex << insn->getAddress()->getVirtualOffset() << endl;
				insn->setIBTargets(ret_hell);
				ret_hell->insert(ALLOF(jmptables[insn])); // insert any partially analyzed results from rets.
			}
			else if (d->isCall() && (!d->getOperand(0)->isConstant()))
			{
				if (getenv("IB_VERBOSE") != 0)
					cout << "using call hell node for " << hex << insn->getAddress()->getVirtualOffset() << endl;
				// indirect call
				insn->setIBTargets(call_hell);
				call_hell->insert(ALLOF(jmptables[insn])); // insert any partially analyzed results from calls.
			}
			else if (d->isUnconditionalBranch() && (!d->getOperand(0)->isConstant()))
			{
				if (getenv("IB_VERBOSE") != 0)
					cout << "using jmp hell node for " << hex << insn->getAddress()->getVirtualOffset() << endl;
				// indirect jmp
				insn->setIBTargets(jmp_hell);
				jmp_hell->insert(ALLOF(jmptables[insn])); // insert any partially analyzed results from jmps.
			}
		}

		cout << "# ATTRIBUTE fill_in_indtargs::total_ibtas_set=" << dec << total_ibta_set << endl;

		if (getenv("ICFS_VERBOSE") != nullptr)
			print_icfs(firp);
	}

	void unpin_elf_tables(FileIR_t *firp, int64_t do_unpin_opt)
	{
		map<string, int> unpin_counts;
		map<string, int> missed_unpins;

		for (auto scoop : firp->getDataScoops())
		{
			// 4 or 8
			const auto ptrsize = firp->getArchitectureBitWidth() / 8;
			const auto scoop_contents = scoop->getContents().c_str();
			if (scoop->getName() == ".init_array" || scoop->getName() == ".fini_array" || scoop->getName() == ".got.plt" || scoop->getName() == ".got")
			{
				const auto start_offset = (scoop->getName() == ".got.plt") ? 3 * ptrsize : 0u;
				for (auto i = start_offset; i + ptrsize <= scoop->getSize(); i += ptrsize)
				{
					const auto vo =
						ptrsize == 4 ? (VirtualOffset_t) * (uint32_t *)&scoop_contents[i] : ptrsize == 8 ? (VirtualOffset_t) * (uint64_t *)&scoop_contents[i]
																										 : throw domain_error("Invalid ptr size");

					const auto insn = lookupInstruction(vo);

					// OK for .got scoop to miss, some entries are empty.
					if (scoop->getName() == ".got" && (vo == 0 || insn == nullptr))
					{
						if (getenv("UNPIN_VERBOSE") != 0)
							cout << "Skipping " << scoop->getName() << " unpin for " << hex << vo << " due to no instruction at vo" << endl;
						continue;
					}
					if (vo == 0)
					{
						// some segments may be null terminated.
						assert(i + ptrsize == scoop->getSize());
						break;
					}

					// these asserts are probably overkill, but want them for sanity checking for now.
					assert(insn);
					assert(targets.find(vo) != targets.end());

					if (targets[vo].areOnlyTheseSet(
							ibt_provenance_t::ibtp_initarray |
							ibt_provenance_t::ibtp_finiarray |
							ibt_provenance_t::ibtp_gotplt |
							ibt_provenance_t::ibtp_got |
							ibt_provenance_t::ibtp_stars_data))
					{
						total_unpins++;
						auto allow_unpin = true;
						if (do_unpin_opt != -1)
						{
							if (total_unpins > do_unpin_opt)
							{
								/*
									cout<<"Aborting unpin process mid elf table."<<endl;
									return;
								*/
								allow_unpin = false;
							}
						}

						// when/if they fail, convert to if and guard the reloc creation.
						if (getenv("UNPIN_VERBOSE") != 0)
						{
							if (allow_unpin)
								cout << "Attempting unpin #" << total_unpins << "." << endl;
							else
								cout << "Eliding unpin #" << total_unpins << "." << endl;
						}

						if (allow_unpin || already_unpinned.find(insn) != already_unpinned.end())
						{
							// mark as unpinned
							already_unpinned.insert(insn);
							unpin_counts[scoop->getName()]++;

							// add reloc to IR.
							auto nr = firp->addNewRelocation(scoop, i, "data_to_insn_ptr", insn);
							assert(nr);

							if (getenv("UNPIN_VERBOSE") != 0)
								cout << "Unpinning " + scoop->getName() + " entry at offset " << dec << i << endl;
							if (insn->getIndirectBranchTargetAddress() == nullptr)
							{
								// add ibta to mark as unpipnned
								const auto fileid = insn->getAddress()->getFileID();
								auto newaddr = firp->addNewAddress(fileid, 0);
								insn->setIndirectBranchTargetAddress(newaddr);
							}
							else
							{
								// just mark as unpinned.
								insn->getIndirectBranchTargetAddress()->setVirtualOffset(0);
							}
						}
					}
					else
					{
						if (getenv("UNPIN_VERBOSE") != 0)
							cout << "Skipping " << scoop->getName() << " unpin for " << hex << vo << " due to other references at offset=" << dec << i << endl;
						missed_unpins[scoop->getName()]++;
					}
				}
			}
			else if (scoop->getName() == ".dynsym")
			{
				const auto ptrsize = firp->getArchitectureBitWidth() / 8;
				const auto scoop_contents = scoop->getContents().c_str();
				const auto symsize =
					ptrsize == 8 ? sizeof(Elf64_Sym) : ptrsize == 4 ? sizeof(Elf32_Sym)
																	: throw domain_error("Cannot detect ptr size -> ELF symbol mapping");

				auto table_entry_no = 0U;
				for (auto i = 0U; i + symsize < scoop->getSize(); i += symsize, table_entry_no++)
				{
					int addr_offset = 0;
					VirtualOffset_t vo = 0;
					int st_info_field = 0;
					int shndx = 0;
					switch (ptrsize)
					{
					case 4:
					{
						auto sym32 = (Elf32_Sym *)&scoop_contents[i];
						addr_offset = (uintptr_t) & (sym32->st_value) - (uintptr_t)sym32;
						vo = sym32->st_value;
						st_info_field = sym32->st_info;
						shndx = sym32->st_shndx;
						break;
					}
					case 8:
					{
						auto sym64 = (Elf64_Sym *)&scoop_contents[i];
						addr_offset = (uintptr_t) & (sym64->st_value) - (uintptr_t)sym64;
						vo = sym64->st_value;
						st_info_field = sym64->st_info;
						shndx = sym64->st_shndx;
						break;
					}
					default:
						throw domain_error("Invalid pointer size");
					}

					// this is good for both 32- and 64-bit.
					int type = ELF32_ST_TYPE(st_info_field);

					if (shndx != SHN_UNDEF && type == STT_FUNC)
					{
						auto insn = lookupInstruction(vo);

						// these asserts are probably overkill, but want them for sanity checking for now.
						assert(insn);
						assert(targets.find(vo) != targets.end());

						// check that the ibt is only ref'd by .dynsym (and STARS, which is ambigulous
						// about which section
						if (targets[vo].areOnlyTheseSet(ibt_provenance_t::ibtp_dynsym | ibt_provenance_t::ibtp_stars_data))
						{
							total_unpins++;
							auto allow_unpin = true;
							if (do_unpin_opt != -1)
							{
								if (total_unpins > do_unpin_opt)
								{
									cout << "Disallowing more new unpins in .dynsym." << endl;
									allow_unpin = false;
								}
								else
								{
									cout << "Attempting unpin #" << total_unpins << "." << endl;
								}
							}

							// check if we are allowing a new unpin, in which case, unpin here.
							// or, if we are continuing the rewrite for an already unpinned instruction.
							if (allow_unpin || already_unpinned.find(insn) != already_unpinned.end())
							{
								already_unpinned.insert(insn);
								if (getenv("UNPIN_VERBOSE") != 0)
									cout << "Unpinning .dynsym entry no " << dec << table_entry_no << ". vo=" << hex << vo << endl;

								unpin_counts[scoop->getName()]++;
								auto nr = firp->addNewRelocation(scoop, i + addr_offset, "data_to_insn_ptr", insn);
								(void)nr;

								if (insn->getIndirectBranchTargetAddress() == nullptr)
								{
									auto newaddr = firp->addNewAddress(insn->getAddress()->getFileID(), 0);
									insn->setIndirectBranchTargetAddress(newaddr);
								}
								else
								{
									insn->getIndirectBranchTargetAddress()->setVirtualOffset(0);
								}
							}
						}
						else
						{
							if (getenv("UNPIN_VERBOSE") != 0)
								cout << "Skipping .dynsm unpin for " << hex << vo << " due to other references." << dec << i << endl;
							missed_unpins[scoop->getName()]++;
						}
					}
				}
			}
			else
			{
				if (getenv("UNPIN_VERBOSE") != 0)
					cout << "Skipping unpin of section " << scoop->getName() << endl;
			}
		}

		auto total_elftable_unpins = 0;

		// print unpin stats.
		for (map<string, int>::iterator it = unpin_counts.begin(); it != unpin_counts.end(); ++it)
		{
			auto name = it->first;
			auto count = it->second;
			cout << "# ATTRIBUTE fill_in_indtargs::unpin_count_" << name << "=" << dec << count << endl;
			total_elftable_unpins++;
		}
		for (auto it = missed_unpins.begin(); it != missed_unpins.end(); ++it)
		{
			string name = it->first;
			int count = it->second;
			cout << "# ATTRIBUTE fill_in_indtargs::missed_unpin_count_" << name << "=" << dec << count << endl;
		}
		cout << "# ATTRIBUTE fill_in_indtargs::total_elftable_unpins=" << dec << total_elftable_unpins << endl;
	}

	DataScoop_t *find_scoop(FileIR_t *firp, const VirtualOffset_t &vo)
	{
		for (auto s : firp->getDataScoops())
		{
			if (s->getStart()->getVirtualOffset() <= vo && vo < s->getEnd()->getVirtualOffset())
				return s;
		}
		return nullptr;
	}

	/*
	 * unpin_type3_switchtable -- Unpin table entries for arm32 type switchs.
	 *
	 * 	firp -- the file IR to modify
	 * 	insn -- the instruction that is the switch dispatch, used for obtaining jump table entries
	 * 	scoop -- the scoop that holds the jump table
	 * 	do_unpin_opt -- how many unpins are allowed, so we can abort unpin early for debugging.
	 */
	void unpin_type3_switchtable(
		FileIR_t *firp,
		Instruction_t *insn,
		DataScoop_t *scoop,
		int64_t do_unpin_opt)
	{

		assert(firp && insn && scoop);

		auto switch_targs = set<Instruction_t *>();

		if (getenv("UNPIN_VERBOSE"))
		{
			cout << "Unpinning type3 switch, dispatch is " << hex << insn->getAddress()->getVirtualOffset() << ":"
				 << insn->getDisassembly() << " with tabSz=" << jmptables[insn].GetTableSize() << endl;
		}

		// switch check
		// could be dangerous if the IBT is found in rodata section,
		// but then also used for a switch.  this is unlikely.
		ibt_provenance_t prov = ibt_provenance_t::ibtp_switchtable_type3 | // found as switch
								ibt_provenance_t::ibtp_stars_switch |	   // found as stars switch
								ibt_provenance_t::ibtp_rodata;			   // found in rodata.

		ibt_provenance_t newprov = ibt_provenance_t::ibtp_switchtable_type3 | // found as switch
								   ibt_provenance_t::ibtp_stars_switch;		  // found as stars switch

		// ptr size
		int ptrsize = firp->getArchitectureBitWidth() / 8;

		// offset from start of scoop
		VirtualOffset_t scoop_off = jmptables[insn].GetTableStart() - scoop->getStart()->getVirtualOffset();

		// scoop contents
		const char *scoop_contents = scoop->getContents().c_str();

		for (auto i = 0u; i < jmptables[insn].GetTableSize(); i++)
		{

			// grab the value out of the scoop
			VirtualOffset_t table_entry = 0;
			switch (ptrsize)
			{
			case 4:
				table_entry = (VirtualOffset_t) * (int *)&scoop_contents[scoop_off];
				break;
			case 8:
				table_entry = (VirtualOffset_t) * (int **)&scoop_contents[scoop_off];
				break;
			default:
				assert(0);
			}

			// verify we have an instruction.
			auto ibt = lookupInstruction(table_entry);
			if (ibt)
			{
				// which isn't otherwise addressed.
				if (targets[table_entry].areOnlyTheseSet(prov))
				{
					auto allow_new_unpins = true;
					total_unpins++;
					if (do_unpin_opt != -1 && total_unpins > do_unpin_opt)
					{
						allow_new_unpins = false;

						// don't abort early (e.g. return) as we need to fully unpin the IBT once we've started.
						// just disallow new unpins.
					}

					// if we are out of unpins, and we haven't unpinned this instruction yet, skip unpinning it.
					if (!allow_new_unpins && already_unpinned.find(ibt) == already_unpinned.end())
					{
						if (getenv("UNPIN_VERBOSE"))
							cout << "Eliding switch unpin for entry[" << dec << i << "] ibt=" << hex << table_entry << ", scoop_off=" << scoop_off << endl;

						// do nothing
					}
					else
					{
						if (getenv("UNPIN_VERBOSE"))
							cout << "Unpinning switch entry [" << dec << i << "] for ibt=" << hex << table_entry << ", scoop_off=" << scoop_off << endl;
						already_unpinned.insert(ibt);

						// add reloc to IR.
						auto nr = firp->addNewRelocation(scoop, scoop_off, "data_to_insn_ptr", ibt);
						(void)nr;

						// remove rodata reference for hell nodes.
						targets[table_entry] = newprov;
						switch_targs.insert(ibt);

						if (ibt->getIndirectBranchTargetAddress() == nullptr)
						{
							auto newaddr = firp->addNewAddress(ibt->getAddress()->getFileID(), 0);
							ibt->setIndirectBranchTargetAddress(newaddr);
						}
						else
						{
							ibt->getIndirectBranchTargetAddress()->setVirtualOffset(0);
						}
					}
				}
			}
			scoop_off += ptrsize;
		}

		type3_unpins += switch_targs.size();
		type3_pins += jmptables[insn].size();
	}

	/*
	 * unpin_type4_switchtable -- Unpin table entries for x86-64 pic-type switchs.
	 *
	 * 	firp -- the file IR to modify
	 * 	insn -- the instruction that is the switch dispatch, used for obtaining jump table entries
	 * 	scoop -- the scoop that holds the jump table
	 * 	do_unpin_opt -- how many unpins are allowed, so we can abort unpin early for debugging.
	 */
	void unpin_type4_switchtable(
		FileIR_t *firp,
		Instruction_t *insn,
		DataScoop_t *scoop,
		int64_t do_unpin_opt, 
		const InstructionSet_t& always_complete_targets)
	{
		assert(firp && insn && scoop);

		// switch check
		// could be dangerous if the IBT is found in rodata section,
		// but then also used for a switch.  this is unlikely.
		const auto prov = ibt_provenance_t::ibtp_switchtable_type4 | // found as switch by FII
						  ibt_provenance_t::ibtp_stars_switch |		 // found as stars switch
						  ibt_provenance_t::ibtp_ret;				 // or a ret, which won't stop us from unpinning.

		// extract some fields we will need
		const auto entry_size = jmptables[insn].GetTableEntrySize();
		const auto table_size = jmptables[insn].GetTableSize();
		const auto multiplier = jmptables[insn].GetTableMultiplier();
		const auto table_start = jmptables[insn].GetTableStart();

		if (getenv("UNPIN_VERBOSE"))
		{
			cout << "Unpinning type4 switch, dispatch is "
				 << hex << insn->getAddress()->getVirtualOffset() << "@" << insn->getDisassembly() << '\n'
				 << "\twith tabSz=" << table_size << '\n'
				 << "\twith tableStart address=" << table_start << '\n'
				 << "\twith entrySz=" << entry_size << '\n'
				 << "\twith mult=" << multiplier << '\n';
		}

		// make assumption for now so we can use standard relocation types.
		if (entry_size != 4 || multiplier != 1)
			return;

		// scoop contents
		const auto scoop_contents = scoop->getContents().c_str();

		// inspect each table entry
		auto allow_new_unpins = true;
		for (auto i = 0u; i < table_size; i++)
		{
			// offset of the table entry from start of scoop
			const auto scoop_off = jmptables[insn].GetTableStart() - scoop->getStart()->getVirtualOffset() + (entry_size * i);

			// grab the value out of the scoop
			assert(entry_size == 4);
			const auto table_entry = static_cast<VirtualOffset_t>(*reinterpret_cast<const int32_t *>(&scoop_contents[scoop_off]));

			// a table entry is:
			// (target - table_start)/multiplier
			// multiplier is typically 1 or -1 and applied by the switch dispach code
			assert(multiplier == 1);

			// calculate the target address from the table entry.
			const auto ibt_address = table_entry + table_start;

			if (getenv("UNPIN_VERBOSE"))
				cout << "Trying unpin for address " << hex << ibt_address << '\n';

			// verify we have an instruction.
			auto ibt = lookupInstruction(ibt_address);
			if (!ibt)
			{
				if (getenv("UNPIN_VERBOSE"))
					cout << "No instruciton for address " << hex << ibt_address << '\n';
				type4_missed_unpin_reasons["no-instruction"]++;
				continue;
			}

			// which isn't otherwise addressed.
			const auto pinTypes = targets[ibt_address];
			if (!pinTypes.areOnlyTheseSet(prov))
			{
				if (getenv("UNPIN_VERBOSE"))
					cout << "Instruction has unknown pin types (" << pinTypes << ") for address "
						 << hex << ibt_address << '\n';
				type4_missed_unpin_reasons["other-refs-" + pinTypes.toString()]++;
				continue;
			}

			// Check that we can unpin this target in all type4 switch tables.
			// if a target is pinned in one table, we can't unpin it here.
			const auto always_complete_it = always_complete_targets.find(ibt);
			const auto always_complete = always_complete_it != always_complete_targets.end();
			if(!always_complete)
			{
				if (getenv("UNPIN_VERBOSE"))
				{
					cout << "Instruction is pinned in incomlpete switch tables (" << pinTypes << ") for address "
						 << hex << ibt_address << '\n';
				}
				type4_missed_unpin_reasons["incomplete-switch-refs-" + pinTypes.toString()]++;
				continue;
			}

			total_unpins++;
			if (do_unpin_opt != -1 && total_unpins > do_unpin_opt)
			{
				allow_new_unpins = false;

				// don't abort early (e.g. return) as we need to fully unpin the IBT once we've started.
				// just disallow new unpins.
			}

			// if we are out of unpins, and we haven't unpinned this instruction yet, skip unpinning it.
			if (!allow_new_unpins && already_unpinned.find(ibt) == already_unpinned.end())
			{
				if (getenv("UNPIN_VERBOSE"))
					cout << "Eliding switch unpin for entry[" << dec << i << "] ibt=" << hex << ibt_address << ", scoop_off=" << scoop_off << endl;

				type4_missed_unpin_reasons["unpins-disallowed"]++;
				continue;
			}

			if (getenv("UNPIN_VERBOSE"))
				cout << "Unpinning switch entry [" << hex << i << "] for ibt=" << hex << ibt_address << ", scoop_off=" << scoop_off << endl;
			already_unpinned.insert(ibt);

			// add reloc to IR.
			firp->addNewRelocation(scoop, scoop_off, "switch_type4", ibt, -ibt_address);

			// remove rodata reference for hell nodes.
			type4_unpins++;

			assert(ibt->getIndirectBranchTargetAddress() != nullptr);
			ibt->getIndirectBranchTargetAddress()->setVirtualOffset(0);
		}

		type4_pins += jmptables[insn].size();
	}

	void unpin_switches(FileIR_t *firp, int do_unpin_opt)
	{
		auto all_fiis = vector<JmpTable_t::mapped_type>();
		auto complete_fiis = vector<JmpTable_t::mapped_type>();
		auto other_fiis = vector<JmpTable_t::mapped_type>();
		auto complete_targets = vector<InstructionSet_t>();
		auto other_targets = vector<InstructionSet_t>();

		// reduce complexity of jmptables by dropping the keys, and getting a vector of values.
		transform(ALLOF(jmptables),
				  back_inserter(all_fiis),
				  [](const JmpTable_t::value_type &p)
				  { return p.second; });

		// get the ones that have complete status, and the ones that don't
		copy_if(ALLOF(all_fiis),
				back_inserter(complete_fiis),
				[](const JmpTable_t::mapped_type &p)
				{ return p.isComplete(); });
		copy_if(ALLOF(all_fiis),
				back_inserter(other_fiis),
				[](const JmpTable_t::mapped_type &p)
				{ return !p.isComplete(); });

		// remove complete/not complete status away and flatten
		transform(ALLOF(complete_fiis), back_inserter(complete_targets), [](const fii_icfs& p) { return p; } );
		auto complete_target_insns = accumulate(
			ALLOF(complete_targets), 
			InstructionSet_t(), 
			[](InstructionSet_t &a, const InstructionSet_t& b) { a.insert(ALLOF(b)); return a; }
			);

		transform(ALLOF(other_fiis), back_inserter(other_targets), [](const fii_icfs& p) { return p; } );
		auto other_target_insns = accumulate(
			ALLOF(other_targets), 
			InstructionSet_t(), 
			[](InstructionSet_t &a, const InstructionSet_t& b) { a.insert(ALLOF(b));  return a; }
			);

		auto always_complete_targets = InstructionSet_t();
		set_difference(ALLOF(complete_target_insns), ALLOF(other_target_insns), inserter(always_complete_targets, always_complete_targets.begin()) );;

		// for each instruction
		for (auto insn : firp->getInstructions())
		{
			// check for an insn.
			assert(insn);

			// if we didn't find a jmptable for this insn, try again.
			if (jmptables.find(insn) == jmptables.end())
				continue;

			// sanity check we have a good switch
			if (insn->getIBTargets() == nullptr)
				continue;

			// sanity check we have a good switch
			if (insn->getIBTargets()->getAnalysisStatus() != iasAnalysisComplete)
				continue;

			// find the scoop, try next if we fail.
			auto scoop = find_scoop(firp, jmptables[insn].GetTableStart());
			if (!scoop)
				continue;

			/* handle arm32 unpins */
			if (jmptables[insn].GetSwitchType().areOnlyTheseSet(
					ibt_provenance_t::ibtp_switchtable_type3 |
					ibt_provenance_t::ibtp_rodata |
					ibt_provenance_t::ibtp_stars_switch))
			{
				unpin_type3_switchtable(firp, insn, scoop, do_unpin_opt);
			}
			/* handle x86-64 pic64 unpins */
			else if (jmptables[insn].GetSwitchType().areOnlyTheseSet(
						 ibt_provenance_t::ibtp_switchtable_type4 |
						 ibt_provenance_t::ibtp_rodata |
						 ibt_provenance_t::ibtp_stars_switch))
			{
				unpin_type4_switchtable(firp, insn, scoop, do_unpin_opt, always_complete_targets);
			}
		}
		cout << "# ATTRIBUTE fill_in_indtargs::switch_type3_pins=" << dec << type3_pins << '\n';
		cout << "# ATTRIBUTE fill_in_indtargs::switch_type3_unpins=" << dec << type3_unpins << '\n';
		cout << "# ATTRIBUTE fill_in_indtargs::switch_type4_pins=" << dec << type4_pins << '\n';
		cout << "# ATTRIBUTE fill_in_indtargs::switch_type4_unpins=" << dec << type4_unpins << '\n';
		for (auto type4_missed_unpin_reason : type4_missed_unpin_reasons)
		{
			const auto reason = type4_missed_unpin_reason.first;
			const auto count = type4_missed_unpin_reason.second;
			cout << "# ATTRIBUTE fill_in_indtargs::type4_missed_unpin_" << reason << "=" << dec << count << '\n';
		}
		if (type4_missed_unpin_reasons.size() == 0)
		{
			cout << "# ATTRIBUTE type4_missed_unpin=0\n";
		}
	}

	void print_unpins(FileIR_t *firp)
	{
		// don't print if not asked for this type of verbose
		if (getenv("UNPIN_VERBOSE") == nullptr)
			return;

		for (auto scoop : firp->getDataScoops())
		{
			assert(scoop);
			for (auto reloc : scoop->getRelocations())
			{
				assert(reloc);
				cout << "Found relocation in " << scoop->getName() << " of type " << reloc->getType() << " at offset " << hex << reloc->getOffset() << endl;
			}
		}
	}

	void unpin_well_analyzed_ibts(FileIR_t *firp, int64_t do_unpin_opt)
	{
		unpin_elf_tables(firp, do_unpin_opt);
		unpin_switches(firp, do_unpin_opt);
		print_unpins(firp);
	}

	bool find_arm_address_gen(Instruction_t *insn, const string &reg, VirtualOffset_t &address, bool &page_only)
	{
		// init output args, just in case.
		address = 0;
		page_only = true;

		auto adrp_insn = (Instruction_t *)nullptr;
		if (backup_until(string() + "adrp " + reg + ",",     /* to find */
						 adrp_insn,          /* return insn here */
						 insn,               /* look before here */
						 "^" + reg + "$", "",/* stop if reg is set */
						 true))              /* try hard to find the other half, more expensive */
		{
			assert(adrp_insn);
			const auto adrp_disasm = DecodedInstruction_t::factory(adrp_insn);
			const auto page_no = adrp_disasm->getOperand(1)->getConstant();

			cout << "Found spilled page_no at " << hex << insn->getAddress()->getVirtualOffset() << " in: " << endl;
			cout << "\t" << adrp_disasm->getDisassembly() << endl;
			cout << "\t" << insn->getDisassembly() << endl;
			page_only = true;
			address = page_no;
			return true;
		}

		auto add_insn = (Instruction_t *)nullptr;
		if (backup_until(string() + "add " + reg + ",",      /* to find */
						 add_insn,           /* return insn here */
						 insn,               /* look before here */
						 "^" + reg + "$", "",/* stop if reg is set */
						 true))	             /* try hard to find the other half, more expensive */
		{
			assert(add_insn);
			const auto add_disasm = DecodedInstruction_t::factory(add_insn);
			const auto add_op1 = add_disasm->getOperand(1);
			const auto add_op2 = add_disasm->getOperand(2);
			if (!add_op1->isRegister())
				return false;
			if (add_op1->getString() == "x29")
				return false; // skip arm SP.
			if (!add_op2->isConstant())
				return false;

			const auto add_op1_reg = add_op1->getString();
			const auto add_op2_constant = add_op2->getConstant();

			// try to find an adrp
			auto adrp_insn = (Instruction_t *)nullptr;
			if (!backup_until(string() + "adrp " + add_op1_reg + ",",     /* to find */
							  adrp_insn,                  /* return insn here */
							  add_insn,                   /* look before here */
							  "^" + add_op1_reg + "$", "",/* stop if reg is set */
							  true))                      /* try hard to find the other half, more expensive */
				return false;
			assert(adrp_insn);

			const auto adrp_disasm = DecodedInstruction_t::factory(adrp_insn);
			const auto adrp_page = adrp_disasm->getOperand(1)->getConstant();
			const auto spilled_address = adrp_page + add_op2_constant;
			cout << "Found spilled address at " << hex << insn->getAddress()->getVirtualOffset() << " in: " << endl;
			cout << "\t" << adrp_disasm->getDisassembly() << endl;
			cout << "\t" << add_disasm->getDisassembly() << endl;
			cout << "\t" << insn->getDisassembly() << endl;
			page_only = false;
			address = spilled_address;
			return true;
		}
		return false;
	}

	void find_all_arm_unks(FileIR_t *firp)
	{
		const auto reg_to_spilled_addr = [&](Instruction_t *insn, const string &reg, const SpillPoint_t &spill_loc)
		{
			auto page_only = true;
			auto address = VirtualOffset_t(0);
			if (find_arm_address_gen(insn, reg, address, page_only))
			{
				if (page_only)
					spilled_adrps[spill_loc].insert(address);
				else
					spilled_add_adrp_results[spill_loc].insert(address);
			}
		};
		const auto do_verbose = getenv("IB_VERBOSE");
		/* only valid for arm */
		if (firp->getArchitecture()->getMachineType() != admtAarch64)
			return;

		/* find adrps */
		for (auto insn : firp->getInstructions())
		{
			const auto d = DecodedInstruction_t::factory(insn);
			if (d->getMnemonic() != "adrp")
				continue;
			const auto op1 = d->getOperand(1);
			const auto op1_constant = op1->getConstant();
			all_adrp_results[insn->getFunction()].insert(op1_constant);
		}

		/* find add/adrp pairs */
		for (auto insn : firp->getInstructions())
		{
			const auto d = DecodedInstruction_t::factory(insn);
			if (d->getMnemonic() != "add")
				continue;
			if (!d->hasOperand(1))
				continue;
			if (!d->hasOperand(2))
				continue;
			const auto op0 = d->getOperand(1);
			const auto op1 = d->getOperand(1);
			const auto op2 = d->getOperand(2);
			if (!op1->isRegister())
				continue;
			if (op1->getString() == "x29")
				continue; // skip arm SP.
			if (!op2->isConstant())
				continue;

			const auto op1_reg = op1->getString();
			const auto op2_constant = op2->getConstant();

			// try to find an adrp
			auto adrp_insn = (Instruction_t *)nullptr;
			if (!backup_until(string() + "adrp " + op1_reg + ",",      /* to find */
							  adrp_insn,               /* return insn here */
							  insn,                    /* look before here */
							  "^" + op1_reg + "$", "", /* stop if reg is set */
							  true))                   /* try hard to find the other half, more expensive */
				continue;
			assert(adrp_insn);

			const auto adrp_disasm = DecodedInstruction_t::factory(adrp_insn);
			const auto adrp_page = adrp_disasm->getOperand(1)->getConstant();
			const auto unk_value = adrp_page + op2_constant;

			all_add_adrp_results[insn->getFunction()].insert(unk_value);
			all_add_adrp_results[adrp_insn->getFunction()].insert(unk_value);
			per_reg_add_adrp_results[op0->getString()].insert(unk_value);

			/* check for scoops at the unk address.
			 * if found, we assume that the unk points at data.
			 * else, we mark it as a possible code target.
			 */
			if (firp->findScoop(unk_value) == nullptr)
				possible_target(unk_value, 0, ibt_provenance_t::ibtp_text);

			/* verbose logging */
			if (do_verbose)
				cout << "Detected ARM unk=" << hex << unk_value << " for " << d->getDisassembly()
					 << " and " << adrp_disasm->getDisassembly() << endl;
		}
		/* find spilled adrp's and add/adrp pairs
		 * looking for these patterns:
		 *
		 * adrp reg1, #page_no
		 * add reg2, reg1, #page_offset
		 * str reg2, [x29, #spill loc]  ; or sp instead of x29
		 *
		 * or
		 *
		 * adrp reg1, #page_no
		 * str reg1, [x29, #spill loc]  ; or sp instead of x29
		 *
		 * we record these later, in case we find a switch dispatch that has a spilled jump table address.
		 */
		for (auto insn : firp->getInstructions())
		{
			// look for a spill of an address
			const auto d = DecodedInstruction_t::factory(insn);

			// spills are str instructions
			if (d->getMnemonic() != "str")
				continue;

			// spills of an address are writing an X register.
			const auto spill_op0_reg = d->getOperand(0)->getString();
			if (spill_op0_reg[0] != 'x')
				continue;

			// spills write to the stack with a constant offset.
			const auto spill_op1 = d->getOperand(1);
			assert(spill_op1->isMemory());
			const auto spill_op1_string = spill_op1->getString();
			// needs to have a base reg, which is either sp or x29
			if (!spill_op1->hasBaseRegister())
				continue;
			if (spill_op1_string.substr(0, 2) != "sp" && spill_op1_string.substr(0, 3) != "x29")
				continue;
			if (!spill_op1->hasMemoryDisplacement())
				continue;
			if (spill_op1->hasIndexRegister())
				continue;

			const auto spill_disp = spill_op1->getMemoryDisplacement();

			// found str <x-reg> [sp+const]

			reg_to_spilled_addr(insn, spill_op0_reg, SpillPoint_t({insn->getFunction(), spill_disp}));
		}
		for (auto insn : firp->getInstructions())
		{
			// look for a spill of an address
			const auto d = DecodedInstruction_t::factory(insn);

			// spills are str instructions
			if (d->getMnemonic() != "stp")
				continue;

			// spills of an address are writing an X register.
			const auto spill_op0_reg = d->getOperand(0)->getString();
			const auto spill_op1_reg = d->getOperand(1)->getString();
			if (spill_op0_reg[0] != 'x')
				continue;

			// spills write to the stack with a constant offset.
			const auto spill_op2 = d->getOperand(2);
			assert(spill_op2->isMemory());
			const auto spill_op2_string = spill_op2->getString();
			// needs to have a base reg, which is either sp or x29
			if (!spill_op2->hasBaseRegister())
				continue;
			if (spill_op2_string.substr(0, 2) != "sp" && spill_op2_string.substr(0, 3) != "x29")
				continue;
			if (!spill_op2->hasMemoryDisplacement())
				continue;
			if (spill_op2->hasIndexRegister())
				continue;

			const auto spill_disp = spill_op2->getMemoryDisplacement();

			// found stp <xreg> <xreg> [sp+const]

			reg_to_spilled_addr(insn, spill_op0_reg, SpillPoint_t({insn->getFunction(), spill_disp}));
			reg_to_spilled_addr(insn, spill_op1_reg, SpillPoint_t({insn->getFunction(), spill_disp + 8}));
		}

		// look for spilling an address into a d-register.
		for (auto insn : firp->getInstructions())
		{
			// look for moves to d-regs via fmov.
			const auto d = DecodedInstruction_t::factory(insn);
			if (d->getMnemonic() != "fmov")
				continue;

			// look for a spill of an address
			const auto op0 = d->getOperand(0);
			const auto op1 = d->getOperand(1);
			const auto op0_str = op0->getString();
			const auto op1_str = op1->getString();

			// spills to d-regs are fmov instructions with a d-reg dest and an xreg src.
			if (op0_str[0] != 'd')
				continue;
			if (op1_str[0] != 'x')
				continue;

			auto page_only = true;
			auto address = VirtualOffset_t(0);
			if (find_arm_address_gen(insn, op1_str, address, page_only))
			{
				const auto spill_loc = DregSpillPoint_t({insn->getFunction(), op0_str});
				spilled_to_dreg[spill_loc].insert(address);
			}
		}
	}

	/*
	 * fill_in_indtargs - main driver routine for
	 */
	void fill_in_indtargs(FileIR_t *firp, exeio *exeiop, int64_t do_unpin_opt)
	{
		calc_preds(firp);

		set<VirtualOffset_t> thunk_bases;
		find_all_module_starts(firp, thunk_bases);

		// reset global vars
		bounds.clear();
		ranges.clear();
		targets.clear();
		jmptables.clear();
		already_unpinned.clear();
		lookupInstruction_init(firp);

		int secnum = exeiop->sections.size();
		int secndx = 0;

		/* look through each section and record bounds */
		for (secndx = 0; secndx < secnum; secndx++)
			get_executable_bounds(firp, exeiop->sections[secndx]);

		/* import info from stars */
		read_stars_xref_file(firp);

		/* and find any unks ourselves, as stars is unreliable */
		find_all_arm_unks(firp);

		/* look through each section and look for target possibilities */
		for (secndx = 0; secndx < secnum; secndx++)
			infer_targets(firp, exeiop->sections[secndx], exeiop);

		handle_scoop_scanning(firp);

		/* should move to separate function */
		for (auto pin : forced_pins)
			possible_target(pin, 0, ibt_provenance_t::ibtp_user);

		/* look through the instructions in the program for targets */
		get_instruction_targets(firp, exeiop, thunk_bases);

		/* mark the entry point as a target */
		possible_target(exeiop->get_entry(), 0, ibt_provenance_t::ibtp_entrypoint);

		/* Read the exception handler frame so that those indirect branches are accounted for
		 * then now process the ranges and mark IBTs as necessarthat have exception handling.
		 * But skip it if we are going to read the EH info into the IR.
		 */
		if (!split_eh_frame_opt)
			read_ehframe(firp, exeiop);
		process_ranges(firp);

		/* now, find the .GOT addr and process any pc-rel things for x86-32 ibts. */
		check_for_thunks(firp, thunk_bases);

		/* now deal with dynsym pins */
		process_dynsym(firp);

		/* mark any instructions after a call instruction */
		mark_return_points(firp);

		/* set the IR to have some instructions marked as IB targets, and deal with the ICFS */
		mark_targets(firp);

		cout << "=========================================" << endl;
		cout << "# ATTRIBUTE fill_in_indtargs::total_indirect_targets=" << std::dec << targets.size() << endl;
		print_targets();
		cout << "=========================================" << endl;

		// try to setup an ICFS for every IB.
		setup_icfs(firp, exeiop);

		// do unpinning of well analyzed ibts.
		if (do_unpin_opt != (int64_t)-1)
			unpin_well_analyzed_ibts(firp, do_unpin_opt);
	}

	bool split_eh_frame_opt = true;
	int64_t do_unpin_opt = numeric_limits<int64_t>::max();
	DatabaseID_t variant_id = BaseObj_t::NOT_IN_DATABASE;
	set<VirtualOffset_t> forced_pins;

	int parseArgs(const vector<string> step_args)
	{
		cout << "Parsing parameters with argc= " << step_args.size() << endl;

		// parse dash-style options.
		auto argc_iter = 0u;
		while (argc_iter < step_args.size() && step_args[argc_iter][0] == '-')
		{
			cout << "Parsing parameter: " << step_args[argc_iter] << endl;
			if (step_args[argc_iter] == "--no-unpin")
			{
				do_unpin_opt = -1;
				argc_iter++;
			}
			else if (step_args[argc_iter] == "--unpin")
			{
				do_unpin_opt = numeric_limits<decltype(do_unpin_opt)>::max();
				argc_iter++;
			}
			else if (step_args[argc_iter] == "--max-unpin" || step_args[argc_iter] == "--max-unpins")
			{
				argc_iter++;
				auto arg_as_str = step_args[argc_iter];
				argc_iter++;

				do_unpin_opt = stoul(arg_as_str, nullptr, 0);
				if (do_unpin_opt == 0)
				{
					cerr << "In --max-unpin, cannot convert " << arg_as_str << " to unsigned" << endl;
					exit(1);
				}
			}
			else if (step_args[argc_iter] == "--no-split-eh-frame")
			{
				split_eh_frame_opt = false;
				argc_iter++;
			}
			else if (step_args[argc_iter] == "--split-eh-frame")
			{
				split_eh_frame_opt = true;
				argc_iter++;
			}
			else
			{
				cerr << "Unknown option: " << step_args[argc_iter] << endl;
				return 2;
			}
		}
		// parse addr argumnets
		for (; argc_iter < step_args.size(); argc_iter++)
		{
			char *end_ptr;
			VirtualOffset_t offset = strtol(step_args[argc_iter].c_str(), &end_ptr, 0);
			if (*end_ptr == '\0')
			{
				cout << "force pinning: 0x" << std::hex << offset << endl;
				forced_pins.insert(offset);
			}
		}

		cout << "Setting unpin limit to: " << dec << do_unpin_opt << endl;
		return 0;
	}

	DatabaseID_t max_base_id = BaseObj_t::NOT_IN_DATABASE;

	int executeStep()
	{
		variant_id = getVariantID();
		auto irdb_objects = getIRDBObjects();

		try
		{
			/* setup the interface to the sql server */
			const auto pqxx_interface = irdb_objects->getDBInterface();
			BaseObj_t::setInterface(pqxx_interface);

			auto pidp = irdb_objects->addVariant(variant_id);
			assert(pidp);

			// pidp=new VariantID_t(atoi(argv[1]));

			assert(pidp->isRegistered() == true);

			cout << "New Variant, after reading registration, is: " << *pidp << endl;

			for (const auto &this_file : pidp->getFiles())
			{
				assert(this_file);

				cout << "Analyzing file " << this_file->getURL() << endl;

				// read the db
				m_firp = irdb_objects->addFileIR(variant_id, this_file->getBaseID());
				// firp=new FileIR_t(*pidp, this_file);
				assert(m_firp);

				m_firp->setBaseIDS();
				m_firp->assembleRegistry();

				// record the max base id, in case we add objects
				max_base_id = m_firp->getMaxBaseID();

				// read the executeable file
				auto exeiop = unique_ptr<EXEIO::exeio>(new EXEIO::exeio);
				exeiop->load(string("a.ncexe"));

				// find all indirect branch targets
				init_direct_addresses();
				fill_in_indtargs(m_firp, exeiop.get(), do_unpin_opt);
				if (split_eh_frame_opt)
					split_eh_frame(m_firp, exeiop.get());
				else
				{
					if (m_firp->getArchitecture()->getMachineType() != admtAarch64)
						assert(getenv("SELF_VALIDATE") == nullptr || ranges.size() > 1);
				}
			}

			if (getenv("FII_NOUPDATE") != nullptr)
				return -1;
		}
		catch (const DatabaseError_t &pnide)
		{
			cout << "Unexpected database error: " << pnide << endl;
			return -1;
		}
		catch (const exception &e)
		{
			cout << "Unexpected error: " << e.what() << endl;
			return -1;
		}
		catch (...)
		{
			cerr << "Unexpected error" << endl;
			return -1;
		}

		assert(getenv("SELF_VALIDATE") == nullptr || bounds.size() > 3);
		assert(getenv("SELF_VALIDATE") == nullptr || targets.size() > 100);
		assert(getenv("SELF_VALIDATE") == nullptr || preds.size() > 100);
		assert(getenv("SELF_VALIDATE") == nullptr || lookupInstructionMap.size() > 100);

		return 0;
	}

	std::string getStepName(void) const override
	{
		return std::string("fill_in_indtargs");
	}

	/*
	 * Do a linear-scan disassembly for an IBT.  Start disassemblying at start_addr, and go until you err-out or
	 * find an existing instruction.  Pin the start_addr and return it, output the set of new instructions in the
	 * output parameter.  Put the new instructions in the "func" function.
	 */
	Instruction_t *doDisassemblyForIBT(
		FileIR_t *firp,
		InstructionSet_t &newInstructions,
		VirtualOffset_t start_addr,
		Function_t *func,
		IRDB_SDK::DatabaseID_t newFileID,
		EXEIO::exeio *exeiop,
		const bool doPin,
		const bool fromTableInText)
	{
		// if we already have an instruction, we're done.
		auto ibtarget = lookupInstruction(start_addr);
		if (ibtarget)
			return ibtarget;

		// skip extra disassembly unless the switch table is in the text.
		if (!fromTableInText)
			return nullptr;

		// otherwise, grab the bits we'll need.
		const auto sec = find_section(start_addr, exeiop);

		// sanity check we have a section.
		if (!sec)
			return nullptr;

		const auto sec_addr = sec->get_address();
		const auto sec_size = sec->get_size();
		const auto end_sec_addr = sec_addr + sec_size;
		const auto secdata = sec->get_data();
		const auto sec_offset = start_addr - sec_addr;

		// sanity check we have data
		if (!secdata)
			return nullptr;
		assert(sec_addr <= start_addr && start_addr <= end_sec_addr);

		// no disassembly for non-executable sections
		if (!sec->isExecutable())
			return nullptr;

		// disassemble the instruction
		const auto disasm = DecodedInstruction_t::factory(start_addr, (void *)&secdata[sec_offset], sec_size - sec_offset);
		if (!disasm->valid())
		{
			// done if we can't disassembly
			return nullptr;
		}

		/* create a new instruction */
		const auto instr_len = disasm->length();
		const auto newinsnbits = string(&secdata[sec_offset], instr_len);
		auto newaddr = firp->addNewAddress(newFileID, start_addr);
		auto newpin = doPin ? firp->addNewAddress(newFileID, start_addr) : nullptr;
		auto newinsn = firp->addNewInstruction(newaddr, nullptr, newinsnbits, disasm->getDisassembly() + string(" from fill_in_indtargs "), newpin);

		// bit of a hack here -- set that this instruction was already in the DB
		// so that it can be written w/o the error checks.
		newinsn->setOriginalAddressID(1);

		lookupInstructionMap[start_addr] = newinsn;

		newInstructions.insert(newinsn);
		cout << "Found new instruction from IB analysis " << newinsn->getComment() << "@" << hex << newinsn->getAddress()->getVirtualOffset() << endl;

		const auto fallthroughAddr = start_addr + instr_len;
		auto fallthroughInsn = doDisassemblyForIBT(firp, newInstructions, fallthroughAddr, func, newFileID, exeiop, false, true);

		const auto hasFallthrough = !(disasm->isReturn() || disasm->isUnconditionalBranch());
		if (hasFallthrough)
			newinsn->setFallthrough(fallthroughInsn);

		const auto &operands = disasm->getOperands();
		for (auto operand : operands)
		{
			// check for branches with targets
			if (disasm->isBranch() &&  // it is a branch
				!disasm->isReturn() && // and not a return
				operand->isConstant()) // and has a constant argument
			{
				const auto targetAddress = disasm->getAddress();
				auto target_insn = doDisassemblyForIBT(firp, newInstructions, targetAddress, func, newFileID, exeiop, false, true);
				newinsn->setTarget(target_insn);
			}
		}
		return newinsn;
	}
};

shared_ptr<TransformStep_t> curInvocation;

bool possible_target(VirtualOffset_t p, VirtualOffset_t from_addr, ibt_provenance_t prov)
{
	assert(curInvocation);
	return (dynamic_cast<PopulateIndTargs_t*>(curInvocation.get()))->possible_target(p,from_addr,prov);
}

void range(VirtualOffset_t start, VirtualOffset_t end)
{
	assert(curInvocation);
	return (dynamic_cast<PopulateIndTargs_t*>(curInvocation.get()))->range(start,end);
}

extern "C"
shared_ptr<TransformStep_t> getTransformStep(void)
{
	curInvocation.reset(new PopulateIndTargs_t());
	return curInvocation;
}


