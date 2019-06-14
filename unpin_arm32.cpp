/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/


#include <string>
#include <algorithm>
#include "unpin.h"
#include <memory>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <irdb-util>


using namespace IRDB_SDK;
using namespace std;
using namespace Zipr_SDK;

static inline uint32_t rotr32 (uint32_t n, unsigned int c)
{
  const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n>>c) | (n<<( (-c)&mask ));
}


#define ALLOF(a) begin(a),end(a)
// per machine stuff
void UnpinArm32_t::HandleRetAddrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }


void UnpinArm32_t::HandlePcrelReloc(Instruction_t* from_insn, Relocation_t* reloc)
{
	// decode the instruction and find the pcrel operand
	const auto disasm         = DecodedInstruction_t::factory(from_insn);
	const auto mnemonic       = disasm->getMnemonic();
	const auto orig_insn_addr = from_insn->getAddress()->getVirtualOffset(); // original location
	const auto insn_bytes_len = 4;	// arm is always 4.
	const auto bo_wrt         = reloc->getWRT();
	const auto scoop_wrt      = dynamic_cast<DataScoop_t* >(reloc->getWRT());
	const auto insn_wrt       = dynamic_cast<Instruction_t*>(reloc->getWRT());
	const auto branch_bytes   =  string("\x00\x00\x00\xea",4);
	uint8_t    insn_bytes[insn_bytes_len]; // compiler disallows init on some platforms.
        // but memcpy should init it sufficiently.
        memcpy(insn_bytes, from_insn->getDataBits().c_str(), insn_bytes_len);
        const auto full_insn=*(uint32_t*)insn_bytes;
	const auto mask1  = (1<<1 )-1;
	const auto mask4  = (1<<4 )-1;
	const auto mask8  = (1<<8 )-1;
	const auto mask12 = (1<<12)-1;
	const auto allocate_reg = [](const set<uint32_t>& used) -> uint32_t
		{
			for(auto i=0u; i<15; i++)
			{
				if(used.find(i) == end(used))
					return i;
			}
			assert(0);
		};

	// get the new insn addr 	
	const auto from_insn_location = VirtualOffset_t(locMap[from_insn]);

	// get WRT info
	const auto to_addr = 
		(scoop_wrt != nullptr) ?  scoop_wrt->getStart()->getVirtualOffset() : // is scoop
		(insn_wrt  != nullptr) ?  locMap[insn_wrt]                          : // is instruction
		(bo_wrt    == nullptr) ?  VirtualOffset_t(0u)                       : // no WRT obj 
		throw invalid_argument("Cannot map pcrel relocation WRT object to address");
	const auto addend       = reloc -> getAddend(); 
	const auto reloc_offset = to_addr + addend;

	const auto to_object_id =  
		(scoop_wrt != nullptr) ?  scoop_wrt->getName()       +"@"+to_hex_string(scoop_wrt->getStart()  ->getVirtualOffset()) : // scoop 
		(insn_wrt  != nullptr) ?  insn_wrt ->getDisassembly()+"@"+to_hex_string(insn_wrt ->getAddress()->getVirtualOffset()) : // instruction
		(bo_wrt    == nullptr) ?  string("No-object") : 
		throw invalid_argument("Cannot map pcrel relocation WRT object to address");


	// so far, only handling ldr and ldrb
	const auto is_ldr_type   = mnemonic.substr(0,3)=="ldr";         // ldr, ldrb, ldreq, ldrbeq, etc.
	const auto is_vldr_type  = mnemonic.substr(0,4)=="vldr";        // vldr <double>, vldr <single> 
	const auto is_add_type   = mnemonic.substr(0,3)=="add";         // add, addne, etc.
	const auto is_addne_type = mnemonic == "addne";                 // exactly addne
	const auto is_addls_type = mnemonic == "addls";                	// exactly addls
	const auto is_ldrls_type = mnemonic == "ldrls";                 // exactly ldrls
	const auto I_bit_set     = 0b1 == ( (full_insn >> 25) & mask1);	// 25th bit indicates if op2 is shifted immediate.  
	const auto S_bit_set     = 0b1 == ( (full_insn >> 20) & mask1);	// 20th bit indicates if the flags are updated.  not valid for all insns.
	const auto Rd            = uint32_t(full_insn >> 12) & mask4;
	const auto Rm            = uint32_t(full_insn >>  0) & mask4;
	const auto Rn            = uint32_t(full_insn >> 16) & mask4;
	const auto Rs            = uint32_t(full_insn >>  8) & mask4;
	const auto is_rn_pc      = Rn == 0b1111;                        // rn reg may not be valid for all instructions
	const auto is_rd_pc      = Rd == 0b1111;	                // destination register, not valid for all insns 
	const auto is_pos_imm   = (bool)((full_insn >> 23) & mask1);	// useful only for ldr and vldr

	// find a temp_reg if we need one
	const auto tmp_reg = allocate_reg({Rd,Rm,Rn,Rs, 13, 15});
	assert(tmp_reg < 15); // sanity check 4 bits


	if(is_vldr_type)
	{
		/* 
		 * We need to patch an vldr[b][cond] fp_reg, [pc + constant]
		 * to be at a new location.
		 *
		 * The plan :
		 * FA: b<cond> L0
		 * FT:
		 * ..
		 * L0  str     rd, [sp,#-fc]
		 * L1  ldr     rd, [L6]
		 * L2  add     rd, pc, rd 
		 * L3  vldr    rd, [rd, <imm>] # orig insn
		 * L4  ldr     rd, [sp,#-fc]
		 * L5: b       FT
		 * L6: .word <constant>
		 */
		const auto tramp_size  = 6*4 + 4 ; // 6 insns, 4 bytes each, plus one word of read-only data
		const auto tramp_range = ms.getFreeRange(tramp_size);
		const auto tramp_start = tramp_range.getStart();
		// don't be too fancy, just reserve 16 bytes.
		ms.splitFreeRange({tramp_start,tramp_start+tramp_size});

		// and give the bytes some names
		const auto FA=from_insn_location;
		const auto FT=from_insn_location+4;
		const auto L0 = tramp_start;
		const auto L1 = L0 + 4;
		const auto L2 = L1 + 4;
		const auto L3 = L2 + 4;
		const auto L4 = L3 + 4;
		const auto L5 = L4 + 4;
		const auto L6 = L5 + 4;

		// Create a branch to put over the original ldr 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  &= 0x0f; // clear always condition bits
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0); // add the vldr cond bits.
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// spill tmp_reg at L0, e50d00fc
		auto  spill_insn = string("\xfc\x00\x0d\xe5",4);
		spill_insn[1]   |= (tmp_reg<<4);
		ms.plopBytes(L0,spill_insn.c_str(),4);

		// ldr dest_reg, [pc+k] (where pc+8+k == L6)
		auto ldr_imm_insn = string("\x0c\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (tmp_reg << 4); // set this instruction's dest reg to the ldr's dest reg.
		ms.plopBytes(L1,ldr_imm_insn.c_str(),4);

		// put down add tmp_reg,pc, temp_reg
		auto new_add_word = string("\x00\x00\x8f\xe0",4);   // e08f0000	 add r0, pc, r0
		new_add_word[1]  |= (tmp_reg<<4);
		new_add_word[0]  |= (tmp_reg<<0);
		ms.plopBytes(L2,new_add_word.c_str(),4);

		// put down orig vldr insn, with 1) cond field removed, and 2) Rn set to -> tmp_reg
		auto vldr_insn_bytes = from_insn->getDataBits();
		vldr_insn_bytes[2] &= 0xf0;           // remove Rn bits.
		vldr_insn_bytes[2] |= (tmp_reg << 0); // set Rn to tmp-reg
		ms.plopBytes(L3,vldr_insn_bytes.c_str(),4);

		// put down L5, restore of scratch reg r0
		auto  restore_insn = string("\xfc\x00\x1d\xe5",4);
		restore_insn[1]   |= (tmp_reg<<4); // set Rd field
		ms.plopBytes(L4,restore_insn.c_str(),4);

		// put an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L5,branch_bytes.c_str(),4);
		zo->applyPatch(L5,FT);

		// put the calculated pc-rel offset at L3
		const auto ldr_imm_field = int32_t(full_insn & mask8)*4;
		const auto ldr_imm       = is_pos_imm ? ldr_imm_field : -ldr_imm_field;
		const auto new_offset    = (bo_wrt == nullptr)   ?
			int32_t(orig_insn_addr - L2 + addend) :
			int32_t(orig_insn_addr - L2 + reloc_offset - (ldr_imm + 8));
		ms.plopBytes(L6,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " WRT=" << to_object_id << endl;

	}
	else if( is_ldr_type && !is_rd_pc && !I_bit_set)	/* ldr <not pc>, [pc, imm] */
	{
		/* 
		 * We need to patch an ldr[b][cond] reg, [pc + constant]
		 * to be at a new location.
		 *
		 * The plan :
		 * FA: b<cond> L0
		 * FT:
		 * ..
		 * L0  ldr     rd, [L3]
		 * L1  ldr(b)  rd, [pc, rd]
		 * L2: b       FT
		 * L3: .word <constant to add>
		 */
		const auto tramp_size  = 4*4; // 3 insns, 4 bytes each, plus one word of read-only data
		const auto tramp_range = ms.getFreeRange(tramp_size);
		const auto tramp_start = tramp_range.getStart();
		// don't be too fancy, just reserve 16 bytes.
		ms.splitFreeRange({tramp_start,tramp_start+tramp_size});

		// and give the bytes some names
		const auto FA=from_insn_location;
		const auto FT=from_insn_location+4;
		const auto L0 = tramp_start;
		const auto L1 = tramp_start + 4;
		const auto L2 = tramp_start + 8;
		const auto L3 = tramp_start + 12;

		const auto is_byte_load = (bool)((full_insn >> 22) & mask1);

		assert(Rd!=0xf);	 // not the program counter.

		// Create a branch to put over the original ldr 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  &= 0x0f; // clear always condition bits
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// ldr dest_reg, [pc+k] (where pc+k == L3)
		auto ldr_imm_insn = string("\x04\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (Rd << 4); // set this instruction's dest reg to the ldr's dest reg.
		ms.plopBytes(L0,ldr_imm_insn.c_str(),4);

		// create the modified ldr(b) from the original ldr instruction
		auto new_ldr_word = string("\x00\x00\x9f\xe7",4);
		new_ldr_word[1]  |= (Rd << 4);   // set this instruction's dest reg to the ldr's dest reg.
		new_ldr_word[0]  |= Rd;          // set this instruction's 2nd src reg to the orig ldr's dest reg.
		new_ldr_word[3]  |= is_byte_load << 6; // set this instruction's B flag to match orig insn's
		ms.plopBytes(L1,new_ldr_word.c_str(),4);

		// put an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L2,branch_bytes.c_str(),4);
		zo->applyPatch(L2,FT);

		// put the calculated pc-rel offset at L3
		const auto ldr_imm_field = int32_t(full_insn & mask12);
		const auto ldr_imm       = is_pos_imm ? ldr_imm_field : - ldr_imm_field;
		const auto new_addend    =  bo_wrt == nullptr ?  8 + ldr_imm : reloc_offset;
		const auto new_offset    = int32_t(orig_insn_addr - L3 + new_addend);
		ms.plopBytes(L3,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " ldr_imm = " << ldr_imm << " WRT=" << to_object_id << endl;
	}
	else if( is_ldr_type && !is_rd_pc && I_bit_set)	/* ldr <not pc>, [pc, reg/shift] */
	{
		/* 
		 * We need to patch a ldr Rd [pc, Rm <shift type> <shift amt>]@FA
		 * to be at a new location.
		 *
		 * The plan:
		 * FA: bne L0
		 * FT:
		 * ..
		 * L0:  str Rt, [sp, # - fc]       # spill tmp reg (Rt), use tmp_reg instead of r0
		 * L1:  ldr Rt, [pc, #k]           # where L1+8+k == L6 or k = L6-L1-8
		 * L2:  add Rt, pc, Rt             # add in pc
		 * L3:  ldr Rd, [Rt, Rm <shift type> <shift smt> ] 
		 *                                 # copy of orig insn with pc (Rn field) replaced with Rt.
		 * L4:  ldr Rt, [sp, # - fc]       # spill tmp reg (Rt), use tmp_reg instead of r0
		 * L5:  b FT
		 * L6:  .word L2 - orig_insn_addr
		 */
		const auto tramp_size  = 6*4 + 4 ; // 6 insns, 4 bytes each, plus one word of read-only data
		const auto tramp_range = ms.getFreeRange(tramp_size);
		const auto tramp_start = tramp_range.getStart();
		// don't be too fancy, just reserve tramp_size bytes.
		ms.splitFreeRange({tramp_start,tramp_start+tramp_size});

		// and give the bytes some names
		const auto FA = from_insn_location;
		const auto FT = FA + 4;

		const auto L0 = tramp_start;
		const auto L1 = L0 + 4;
		const auto L2 = L1 + 4;
		const auto L3 = L2 + 4;
		const auto L4 = L3 + 4;
		const auto L5 = L4 + 4;
		const auto L6 = L5 + 4;

		// Create a branch to put over the original ldr 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  &= 0x0f; // clear always condition bits
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// spill tmp_reg at L0, e50d00fc
		auto  spill_insn = string("\xfc\x00\x0d\xe5",4);
		spill_insn[1]   |= (tmp_reg<<4);
		ms.plopBytes(L0,spill_insn.c_str(),4);

		// ldr dest_reg, [pc+k] (where pc+8+k == L6)
		auto ldr_imm_insn = string("\x0c\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (tmp_reg<<4);
		ms.plopBytes(L1,ldr_imm_insn.c_str(),4);

		// put down L2
		auto new_add_word = string("\x00\x00\x8f\xe0",4);   // e08f0000	 add r0, pc, r0
		new_add_word[1]  |= (tmp_reg<<4);
		new_add_word[0]  |= (tmp_reg<<0);
		ms.plopBytes(L2,new_add_word.c_str(),4);

		// put down L3 (orig insn with pc fields set to r0)
		auto orig_ldr = from_insn->getDataBits();
		orig_ldr[3]  &=  0b00001111;   // clear the cond bits.
		orig_ldr[3]  |=  0b11100000;   // set the cond bits to "always".
		orig_ldr[2]  &= ~(mask4 << 0); // clear this instruction's Rn field (i.e., set to r0)
		orig_ldr[2]  |= (tmp_reg<<0); // set Rn fields o tmp_reg
		ms.plopBytes(L3,orig_ldr.c_str(),4);

		// put down L4, restore of scratch reg r0
		auto  restore_insn = string("\xfc\x00\x1d\xe5",4);
		restore_insn[1]   |= (tmp_reg<<4); // set Rd field
		ms.plopBytes(L4,restore_insn.c_str(),4);

		// put an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L5,branch_bytes.c_str(),4);
		zo->applyPatch(L5,FT);

		// put the calculated pc-rel offset at L6
		const auto new_offset = int32_t(orig_insn_addr - L2 + reloc_offset);
		ms.plopBytes(L6,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout << "Had to trampoline " << disasm->getDisassembly() << " @" << hex << FA 
		     << " to " << L0 << "-" << L0+tramp_size-1 << " WRT=" << to_object_id << endl;

	}
	else if((is_ldrls_type || is_addne_type || is_addls_type) && is_rd_pc && is_rn_pc)
	{
		/* 
		 * We need to patch an addne pc, pc, reg lsl #2
		 * to be at a new location.
		 *
		 * The plan :
		 * FA: bne L0
		 * FT:
		 * ..
		 * L0:  str r0, [sp, # - fc]       # spill tmp reg, use tmp_reg instead of r0
		 * L1:  ldr r0, [pc, #k]           # where L1+8+k == L7 or k = L7-L1-8
		 * L2:  add r0, pc, r0             # add in pc
		 * L3:  op r0, r0, ( r2 lsl #2 )  # copy of orig insn with pc removed from op0 and op1, and replaced with tmp_reg.
		 * L4:  str r0, [sp, # - f8]       # store calculated pc
		 * L5:  ldr r0, [sp, # - fc]       # restore reg
		 * L6:  ldr pc, [sp, # - f8]       # jmp dispatch
		 * L7:  .word L2 - orig_insn_addr

		 */
		const auto tramp_size  = 7*4 + 4 ; // 7 insns, 4 bytes each, plus one word of read-only data
		const auto tramp_range = ms.getFreeRange(tramp_size);
		const auto tramp_start = tramp_range.getStart();
		// don't be too fancy, just reserve tramp_size bytes.
		ms.splitFreeRange({tramp_start,tramp_start+tramp_size});

		// and give the bytes some names
		const auto FA=from_insn_location;
		const auto L0 = tramp_start;
		const auto L1 = L0 + 4;
		const auto L2 = L1 + 4;
		const auto L3 = L2 + 4;
		const auto L4 = L3 + 4;
		const auto L5 = L4 + 4;
		const auto L6 = L5 + 4;
		const auto L7 = L6 + 4;

		// Create a branch to put over the original ldr 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  &= 0x0f; // clear always condition bits
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// spill tmp_reg at L0, e50d00fc
		auto  spill_insn = string("\xfc\x00\x0d\xe5",4);
		spill_insn[1]   |= (tmp_reg<<4);
		ms.plopBytes(L0,spill_insn.c_str(),4);

		// ldr dest_reg, [pc+k] (where pc+8+k == L7)
		auto ldr_imm_insn = string("\x10\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (tmp_reg<<4);
		ms.plopBytes(L1,ldr_imm_insn.c_str(),4);

		// put down L2
		auto new_add_word = string("\x00\x00\x8f\xe0",4);   // e08f0000	 add r0, pc, r0
		new_add_word[1]  |= (tmp_reg<<4);
		new_add_word[0]  |= (tmp_reg<<0);
		ms.plopBytes(L2,new_add_word.c_str(),4);

		// put down L3 (orig insn with pc fields set to r0)
		auto orig_add = from_insn->getDataBits();
		orig_add[3]  &=  0b00001111;   // clear the cond bits.
		orig_add[3]  |=  0b11100000;   // set the cond bits to "always".
		orig_add[1]  &= ~(mask4 << 4); // clear this instruction's Rd field (i.e., set to r0)
		orig_add[2]  &= ~(mask4 << 0); // clear this instruction's Rn field (i.e., set to r0)
		orig_add[1]  |= (tmp_reg<<4); // set Rd and Rn fields to tmp_reg
		orig_add[2]  |= (tmp_reg<<0);
		ms.plopBytes(L3,orig_add.c_str(),4);

		// put down L4, store of calc'd pc.
		auto  spill_targ_insn = string("\xf8\x00\x0d\xe5",4);
		spill_targ_insn[1]   |= (tmp_reg<<4); // set Rd field
		ms.plopBytes(L4,spill_targ_insn.c_str(),4);

		// put down L5, restore of scratch reg r0
		auto  restore_insn = string("\xfc\x00\x1d\xe5",4);
		restore_insn[1]   |= (tmp_reg<<4); // set Rd field
		ms.plopBytes(L5,restore_insn.c_str(),4);

		// put down L6, the actual control transfer using the saved value on the stack
		auto  xfer_insn = string("\xf8\x00\x1d\xe5",4);
		xfer_insn[1]   |= (0b1111 << 4); // set this instruction's Rd reg to PC
		ms.plopBytes(L6,xfer_insn.c_str(),4);

		// put the calculated pc-rel offset at L7
		const auto new_offset = int32_t(orig_insn_addr - L2 + reloc_offset);
		ms.plopBytes(L7,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " WRT=" << to_object_id << endl;
	}
	else if(is_add_type && I_bit_set)
	{
		/* 
		 *
		 * here we've found a add<s><cond> Rd, Rn, #constant<<(#shift * 2)
		 * e.g.: add ip, pc, #0, #12  # IP=PC+8+(12<<(0*2)) 
		 *
		 * the plan :
		 * FA: b<cond> L0
		 * FT:
		 * ..
		 * L0  ldr dest_reg, [L3]
		 * L1  add dest_reg, pc, dest_reg
		 * L2: b   FT
		 * L3: .word <constant to add>
		 */

		assert(Rd!=0xf);	 // not the program counter.  needs to be handled as IB

		const auto tramp_size  = 4*4; // 3 insns, 4 bytes each, plus one word of read-only data
		const auto tramp_range = ms.getFreeRange(tramp_size);
		const auto tramp_start = tramp_range.getStart();
		// don't be too fancy, just reserve 16 bytes.
		ms.splitFreeRange({tramp_start,tramp_start+tramp_size});

		// and give the bytes some names
		const auto FA=from_insn_location;
		const auto FT=from_insn_location+4;
		const auto L0 = tramp_start;
		const auto L1 = tramp_start + 4;
		const auto L2 = tramp_start + 8;
		const auto L3 = tramp_start + 12;

		// Create a branch to put over the original insn 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  &= 0x0f; // clear always condition bits
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// ldr dest_reg, [pc+k] (where pc+k == L3)
		auto ldr_imm_insn = string("\x04\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (Rd << 4); // set this instruction's dest reg to the ldr's dest reg.
		ms.plopBytes(L0,ldr_imm_insn.c_str(),4);

		// create the modified add from the original ldr instruction
		auto new_add_word = string("\x00\x00\x8f\xe0",4);   // e08f0000	 add r0, pc, r0
		new_add_word[1]  |= (Rd << 4);   // set this instruction's dest reg to the origin insn's dest reg.
		new_add_word[0]  |= Rd;          // set this instruction's 2nd src reg to the orig insn's dest reg.
		new_add_word[3]  |= S_bit_set << 4; // set this instruction's S flag to match the orig insn
		ms.plopBytes(L1,new_add_word.c_str(),4);

		// put an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L2,branch_bytes.c_str(),4);
		zo->applyPatch(L2,FT);

		// put the calculated pc-rel offset at L3
		const auto add_imm_field = int32_t(full_insn & mask8);
		const auto add_ror_field = int32_t((full_insn>>8) & mask4);
		const auto add_imm       = rotr32(add_imm_field,add_ror_field*2);
		const auto orig_target   = orig_insn_addr + 8 +  add_imm;
		const auto new_offset    = int32_t(orig_target - L3 + reloc_offset);
		ms.plopBytes(L3,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " add_imm = " << add_imm << " WRT=" << to_object_id << endl;
	}
	else if(is_add_type && !I_bit_set)
	{
		/* 
		 * here we've found a add<s><cond> Rd, Rn, Rm <shift_oper> ( shift * 2 )
		 * e.g.  add Rd, pc, Rn          # Rd = pc + Rn
		 * e.g2. add Rd, Rm, pc          # Rd = Rm + pc 
		 * e.g3. add Rd, pc, Rn ror #2   # Rd = pc + (Rn ror #2)
		 * e.g4. add Rd, Rm, pc lsl #2   # Rd = Rm + pc << 2
		 * e.g5. add Rd, pc, Rn lsl Rs   # Rd = pc + (Rn << Rs)
		 * e.g6. add Rd, Rm, pc lsl Rs   # Rd = Rm + (pc << Rs)
		 *
		 * Note: instruction may have a <cond> or an <s> appended
		 * Note: Rd, Rn, and Rs may all be the same register.  We could optimize if they aren't, TBD.
		 *
		 * Assume: no shifting of pc (as in example 4 and 6.)  
		 * Assume: no pc in Rn position (as in example 2, 4, and 6)
		 * Assume: no pc in Rs position (as in example 6)
		 *
		 * Assume: tmp_reg is a (live) register that's not Rd, Rn, Rs, or pc.
		 *
		 *
		 *
		 * FA: b<cond> L0  # deals with all <cond> properly.
		 * FT:
		 * ..
		 * L0  str    tmp_reg -> [sp - 0xfc] # save reg
		 * L1  add    Rd, ...		     # orig add instruction, without <cond> or <s>
		 * L2  ldr    tmp_reg <- [L3]        # load constant offset from cur pc to other pc
		 * L3  add<s> Rd, tmp_reg, Rd        # add in constant, and set S flag.
		 * L4  ldr    tmp_reg <- [sp - 0xfc] # restore reg
		 * L5: b      FT 
		 * L6: .word <orig_pc - new_pc> # Note:  all other fields factor out!
		 */
		assert(Rd!=0xf); // not the program counter.  needs to be handled as IB

		const auto tramp_size  = 6*4 + 4 ; // 6 insns, 4 bytes each, plus one word of read-only data
		const auto tramp_range = ms.getFreeRange(tramp_size);
		const auto tramp_start = tramp_range.getStart();
		// don't be too fancy, just reserve the bytes.
		ms.splitFreeRange({tramp_start,tramp_start+tramp_size});

		// and give the bytes some names
		const auto FA=from_insn_location;
		const auto FT=from_insn_location+4;
		const auto L0 = tramp_start;
		const auto L1 = L0 + 4;
		const auto L2 = L1 + 4;
		const auto L3 = L2 + 4;
		const auto L4 = L3 + 4;
		const auto L5 = L4 + 4;
		const auto L6 = L5 + 4;


		// Create a branch to put over the original insn 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  &= 0x0f; // clear always condition bits
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);	// set cond bits
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// put down L0
		auto  spill_insn = string("\xfc\x00\x0d\xe5",4);
		spill_insn[1]   |= (tmp_reg << 4); // set this instruction's Rd reg to the tmp reg 
		ms.plopBytes(L0,spill_insn.c_str(),4);

		// put down L1
		auto orig_add = from_insn->getDataBits();
		orig_add[2]  &= ~(1 << 4); // clear the S bit.
		orig_add[3]  &=  0b00001111;   // clear the cond bits.
		orig_add[3]  |=  0b11100000;   // set the cond bits to "always".
		ms.plopBytes(L1,orig_add.c_str(),4);

		// Put down L2 (ldr dest_reg, [pc+k] where pc+k == L6)
		auto ldr_imm_insn = string("\x08\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (tmp_reg << 4); // set this instruction's Rd to tmp_reg
		ms.plopBytes(L2,ldr_imm_insn.c_str(),4);

		// put down L3,   an add Rd, tmp_reg, Rd (e0800000) where Rd comes from the origin insn.
		auto L3_insn = string("\x00\x00\x80\xe0",4);
		L3_insn[1]  |= (Rd      << 4); // set this instruction's Rd field to orig insn's Rd 
		L3_insn[2]  |= (tmp_reg << 0); // set this instruction's Rn field to the tmp reg 
		L3_insn[0]  |= (Rd      << 0); // set this instruction's Rm field to orig insn's Rd
		ms.plopBytes(L3,L3_insn.c_str(),4);

		// put down L4, e59d00fc
		auto  restore_insn = string("\xfc\x00\x1d\xe5",4);
		restore_insn[1]   |= (tmp_reg << 4); // set this instruction's Rd reg to the tmp reg 
		ms.plopBytes(L4,restore_insn.c_str(),4);

		// put down L5, an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L5,branch_bytes.c_str(),4);
		zo->applyPatch(L5,FT);

		// put the calculated pc-rel offset at L3
		const auto new_offset    = int32_t(orig_insn_addr - L1 + reloc_offset);
		ms.plopBytes(L6,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " WRT=" << to_object_id << endl;
	}
	else 
	{
		cout <<"WARN: insn patching help: "<< from_insn->getDisassembly()<<endl;
	}
/*
 * other instructions are probably false positives in the disassembly process.  let's just not patch them
	else 
		assert(0);
*/

}


void UnpinArm32_t::HandleAbsptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); } 


void UnpinArm32_t::HandleImmedptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }

void UnpinArm32_t::HandleCallbackReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }




