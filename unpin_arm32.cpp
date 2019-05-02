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
	const auto branch_bytes   =  string("\x00\x00\x00\x0ea",4);
	uint8_t    insn_bytes[insn_bytes_len]; // compiler disallows init on some platforms.
        // but memcpy should init it sufficiently.
        memcpy(insn_bytes, from_insn->getDataBits().c_str(), insn_bytes_len);
        const auto full_insn=*(uint32_t*)insn_bytes;
	const auto mask1  = (1<<1 )-1;
	const auto mask4  = (1<<4 )-1;
	const auto mask8  = (1<<8 )-1;
	const auto mask12 = (1<<12)-1;



	// get the new insn addr 	
	const auto from_insn_location=(VirtualOffset_t)locMap[from_insn];

	// get WRT info
	auto to_addr=VirtualOffset_t(0xdeadbeef); // noteable value that shouldn't be used.
	string convert_string;

	if(scoop_wrt)
	{
		to_addr=scoop_wrt->getStart()->getVirtualOffset();
		convert_string=string("scoop ")+scoop_wrt->getName();
	}
	else if(insn_wrt)
	{
		to_addr=locMap[insn_wrt];
		convert_string=string("insn ")+to_string(insn_wrt->getBaseID())+
			       ":"+insn_wrt->getDisassembly();
	}
	else 
	{
		assert(bo_wrt==nullptr);
		to_addr=0; /* no WRT obj */
		convert_string=string("no-object");
	}
	assert(bo_wrt==nullptr); // not yet imp'd WRT offsetting.
	assert(to_addr==0); // not yet imp'd WRT offsetting.


	// so far, only handling ldr and ldrb
	const auto is_ldr_type = mnemonic.substr(0,3)=="ldr";  // ldr, ldrb, ldreq, ldrbeq, etc.
	const auto is_add_type = mnemonic.substr(0,3)=="add";
	const auto I_bit_set   = (bool)( (full_insn >> 25) & mask1);	// 25th bit indicates if op2 is shifted immediate.  
	const auto S_bit_set   = (bool)( (full_insn >> 20) & mask1);	// 20th bit indicates if the flags are updated.  not valid for all insns.
	const auto dest_reg     = (full_insn >> 12) & mask4;		// destination register, not valid for all insns 

	if(is_ldr_type)
	{
		/* 
		 * We need to patch an ldr[b][cond] reg, [pc + constant]
		 * to be at a new location.
		 *
		 * The plan :
		 * FA: b<cond> L0
		 * FT:
		 * ..
		 * L0  ldr     dest_reg, [L3]
		 * L1  ldr(b)  dest_reg, [pc, dest_reg]
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

		// extract some fields from the orig full_insn
		const auto is_pos_imm   = (bool)((full_insn >> 23) & mask1);
		const auto is_byte_load = (bool)((full_insn >> 22) & mask1);

		assert(dest_reg!=0xf);	 // not the program counter.

		// Create a branch to put over the original ldr 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// ldr dest_reg, [pc+k] (where pc+k == L3)
		auto ldr_imm_insn = string("\x04\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (dest_reg << 4); // set this instruction's dest reg to the ldr's dest reg.
		ms.plopBytes(L0,ldr_imm_insn.c_str(),4);

		// create the modified ldr(b) from the original ldr instruction
		auto new_ldr_word = string("\x00\x00\x9f\xe7",4);
		new_ldr_word[1]  |= (dest_reg << 4);   // set this instruction's dest reg to the ldr's dest reg.
		new_ldr_word[0]  |= dest_reg;          // set this instruction's 2nd src reg to the orig ldr's dest reg.
		new_ldr_word[3]  |= is_byte_load << 6; // set this instruction's B flag to match orig insn's
		ms.plopBytes(L1,new_ldr_word.c_str(),4);

		// put an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L2,branch_bytes.c_str(),4);
		zo->applyPatch(L2,FT);

		// put the calculated pc-rel offset at L3
		const auto ldr_imm_field = int32_t(full_insn & mask12);
		const auto ldr_imm       = is_pos_imm ? ldr_imm_field : - ldr_imm_field;
		const auto orig_target   = orig_insn_addr + 8 +  to_addr + ldr_imm;
		const auto new_offset    = int32_t(orig_target - (L1+8));
		ms.plopBytes(L3,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " ldr_imm = " << ldr_imm << endl;
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

		assert(dest_reg!=0xf);	 // not the program counter.  needs to be handled as IB

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
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// ldr dest_reg, [pc+k] (where pc+k == L3)
		auto ldr_imm_insn = string("\x04\x00\x9f\xe5",4);
		ldr_imm_insn[1]  |= (dest_reg << 4); // set this instruction's dest reg to the ldr's dest reg.
		ms.plopBytes(L0,ldr_imm_insn.c_str(),4);

		// create the modified add from the original ldr instruction
		auto new_add_word = string("\x00\x00\x8f\xe0",4);   // e08f0000	 add r0, pc, r0

		new_add_word[1]  |= (dest_reg << 4);   // set this instruction's dest reg to the origin insn's dest reg.
		new_add_word[0]  |= dest_reg;          // set this instruction's 2nd src reg to the orig insn's dest reg.
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
		const auto orig_target   = orig_insn_addr + 8 +  to_addr + add_imm;
		const auto new_offset    = int32_t(orig_target - (L1+8));
		ms.plopBytes(L3,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << " add_imm = " << add_imm << endl;
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
		 * L0  str    tmp_reg -> [sp + 0xfc] # save reg
		 * L1  add    Rd, ...		     # orig add instruction, without <cond> or <s>
		 * L2  ldr    tmp_reg <- [L3]        # load constant offset from cur pc to other pc
		 * L3  add<s> Rd, tmp_reg, Rd        # add in constant, and set S flag.
		 * L4  ldr    tmp_reg <- [sp + 0xfc] # restore reg
		 * L5: b      FT 
		 * L6: .word <orig_pc - new_pc> # Note:  all other fields factor out!
		 */
		assert(dest_reg!=0xf);	 // not the program counter.  needs to be handled as IB

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

		// find temp_reg
		const auto Rd      = uint32_t(full_insn >> 12) & mask4;
		const auto Rm      = uint32_t(full_insn >>  0) & mask4;
		const auto Rn      = uint32_t(full_insn >> 16) & mask4;
		const auto Rs      = uint32_t(full_insn >>  8) & mask4;
		const auto allocate_reg = [](const set<uint32_t>& used) -> uint32_t
			{
				for(auto i=0u; i<15; i++)
				{
					if(used.find(i) != end(used))
						return i;
				}
				assert(0);

			};
		const auto tmp_reg = allocate_reg({Rd,Rm,Rn,Rs, 13, 15});
		assert(tmp_reg < 15); // sanity check 4 bits


		// Create a branch to put over the original insn 
		// and set the conditional bits equal to 
		// the original instruction conditional bits
		auto my_branch_bytes = branch_bytes;
		my_branch_bytes[3]  |= (insn_bytes[3] & 0xf0);	// set cond bits
		ms.plopBytes(FA,my_branch_bytes.c_str(),4);
		// and make it point at L0
		zo->applyPatch(FA,L0);

		// put down L0, e58d00fc
		auto  spill_insn = string("\xfc\x00\x8d\xe5",4);
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
		auto  restore_insn = string("\xfc\x00\x9d\xe5",4);
		restore_insn[1]   |= (tmp_reg << 4); // set this instruction's Rd reg to the tmp reg 
		ms.plopBytes(L4,restore_insn.c_str(),4);

		// put down L5, an uncond branch the end of the trampoline
		// and make it jump at FT
		ms.plopBytes(L5,branch_bytes.c_str(),4);
		zo->applyPatch(L5,FT);

		// put the calculated pc-rel offset at L3
		const auto new_offset    = int32_t(orig_insn_addr - L1);
		ms.plopBytes(L6,reinterpret_cast<const char*>(&new_offset),4);	// endianness of host must match target

		// should be few enough of these to always print
		cout<< "Had to trampoline " << disasm->getDisassembly() << " @"<<FA<<" to "
		    << hex << L0 << "-" << L0+tramp_size-1 << endl;
	}
	else 
		assert(0);

}


void UnpinArm32_t::HandleAbsptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); } 


void UnpinArm32_t::HandleImmedptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }

void UnpinArm32_t::HandleCallbackReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }




