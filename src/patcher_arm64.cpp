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

#include <zipr_all.h>
namespace zipr
{
#include "patcher/patcher_arm64.hpp"
}
#include <irdb-core>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <fstream>

#define ALLOF(a) begin(a),end(a)

using namespace IRDB_SDK;
using namespace std;
using namespace zipr;

ZiprPatcherARM64_t::ZiprPatcherARM64_t(Zipr_SDK::Zipr_t* p_parent) :
	m_parent(dynamic_cast<zipr::ZiprImpl_t*>(p_parent)),     // upcast to ZiprImpl
        m_firp(p_parent->getFileIR()),
        memory_space(*p_parent->getMemorySpace())

{ 
}

void ZiprPatcherARM64_t::ApplyNopToPatch(RangeAddress_t addr)
{ assert(0); }

void ZiprPatcherARM64_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{ 

        const auto first_byte =(uint8_t)memory_space[from_addr+3];
        const auto second_byte=(uint8_t)memory_space[from_addr+2];
        const auto third_byte =(uint8_t)memory_space[from_addr+1];
        const auto fourth_byte=(uint8_t)memory_space[from_addr+0];
	const auto opcode = (first_byte >> 2) ;
//	const auto insn_length=4U;
        const auto new_offset=(int32_t)((to_addr)-from_addr) >> 2;

	const auto full_word=
			(((uint32_t)first_byte ) << 24)|
			(((uint32_t)second_byte) << 16)|
			(((uint32_t)third_byte ) <<  8)|
			(((uint32_t)fourth_byte) <<  0);

	// A Bracdh unconditional, in binary is: op=000101 imm26=00  00000000  00000000  00000000
	// it includes a 26-bit immediate, which is +/- 128MB, which should be a good enough "jump anywhere"
	// for now.
	//
	const auto is_uncond_branch  =(first_byte >>2) == (0x14 >> 2);  // unconditional branch  
	const auto is_uncond_branch_and_link=(first_byte >>2) == (0x97 >> 2); 	// uncond branch and link 

	// B.cond
	// 01010100 imm19 0 cond
	const auto is_branch_cond = first_byte== 0x54; 			// conditional branch

	// compare and branch 
	// sf 011 0101 imm19 Rt
	// sf 011 0100 imm19 Rt
	const auto is_compare_and_branch_nz = (first_byte & 0x7f) == 0x35; 
	const auto is_compare_and_branch_z  = (first_byte & 0x7f) == 0x34; 
	const auto is_compare_and_branch    = (is_compare_and_branch_nz || is_compare_and_branch_z);

	// b5 011 0111 b40 imm14 Rt -- tbnz
	// b5 011 0110 b40 imm14 Rt -- tbz
	const auto is_test_and_branch_nz = (first_byte & 0x7f) == 0x37; 
	const auto is_test_and_branch_z  = (first_byte & 0x7f) == 0x36; 
	const auto is_test_and_branch    = is_test_and_branch_nz || is_test_and_branch_z;

	if(is_uncond_branch || is_uncond_branch_and_link)
	{
		// cout<<"Applying uncond branch patch from "<<hex<<from_addr<<" to "<<to_addr<<endl;
		const auto non_imm_bits=32U-26U;	// 32 bits, imm26
		// assert there's no overflow.
		assert((uint64_t)(new_offset << non_imm_bits) == ((uint64_t)new_offset) << non_imm_bits);
		// or in opcode for first byte.  set remaining bytes.
		const auto mask26=((1<<26)-1);
		const auto trimmed_offset=new_offset & mask26;
		memory_space[from_addr+0]=               (trimmed_offset>> 0)&0xff;
		memory_space[from_addr+1]=               (trimmed_offset>> 8)&0xff;
		memory_space[from_addr+2]=               (trimmed_offset>>16)&0xff;
		memory_space[from_addr+3]=(opcode<<2) | ((trimmed_offset>>24)&0xff);
	}
	else if (is_branch_cond || is_compare_and_branch)
	{
		const auto non_mask_bits=32U-19;	// 32 bits, imm19
		const auto mask19=(1<<19U)-1;
		if((uint64_t)(new_offset << non_mask_bits) == ((uint64_t)new_offset) << non_mask_bits)
		{
			// the branch offset works here!
			const auto full_word_clean=full_word & ~(mask19<<5);
			const auto full_word_new_offset=full_word_clean | ((new_offset&mask19)<<5);
			memory_space[from_addr+0]=(full_word_new_offset>> 0)&0xff;
			memory_space[from_addr+1]=(full_word_new_offset>> 8)&0xff;
			memory_space[from_addr+2]=(full_word_new_offset>>16)&0xff;
			memory_space[from_addr+3]=(full_word_new_offset>>24)&0xff;
		}
		else
		{
			// branch offset didn't work.
			// hopefully we can get there with a direct branch.
			/* the plan when the branch offset doesn't fit:
			 * FA: b   L0
			 * FT: 
			 * ..
			 * L0  b<cond> <args>, L2 # at tramp_start
			 * L1  b FT
			 * L2: b <target>
			 */
			const auto tramp_size=12;

			// check to see if we already had to trampoline from_addr.  If so, 
			// patch the trampoline, not the actual redirect.
			auto tramp_start=RangeAddress_t(0);
			const auto redirect_it=redirect_map.find(from_addr);
			if(redirect_it==redirect_map.end())
			{
				// allocate new space in memory
				const auto tramp_range=memory_space.getFreeRange(tramp_size);
				tramp_start=tramp_range.getStart();
				// don't be too fancy, just reserve 12 bytes.
				memory_space.splitFreeRange({tramp_start,tramp_start+tramp_size});
				// record that we had to trampoline this!
				redirect_map[from_addr]=tramp_start;
			}
			else
			{
				// use previous tramp space.
				tramp_start=redirect_it->second;
			}

			const auto FA=from_addr;
			const auto FT=from_addr+4;
			const auto L0=tramp_start;
			const auto L1=tramp_start+4;
			const auto L2=tramp_start+8;
			const auto branch_bytes=string("\x00\x00\x00\x014",4);

			// put the cond branch in the trampline, make it jump to L2
			memory_space[L0+0]=memory_space[FA+0];
			memory_space[L0+1]=memory_space[FA+1];
			memory_space[L0+2]=memory_space[FA+2];
			memory_space[L0+3]=memory_space[FA+3];
			ApplyPatch(L0,L2);

			// now make the original location jump to the trampoline
			memory_space.plopBytes(FA, branch_bytes.c_str(), 4);
			ApplyPatch(FA,L0);// make it jump to FT
			
			// now drop down a uncond jump for L1, and make it go to FT
			// (i.e., jump around the jump to the target)
			memory_space.plopBytes(L1, branch_bytes.c_str(), 4);
			ApplyPatch(L1,FT);// make it jump to FT

			// lastly, put down the uncond jump at L2, and make it go to the target
			memory_space.plopBytes(L2, branch_bytes.c_str(), 4);
			ApplyPatch(L2,to_addr);// make it jump to +8

			const auto disasm_str=DecodedInstruction_t::factory(from_addr, (const void*)&full_word, 4)->getDisassembly();

			cout << "Had to trampline "<<disasm_str<< " at "<<hex<<from_addr
			     << " to " << L0 << " - " << L0+tramp_size<< " for target "<<to_addr<<endl;


		}
	}
	else if (is_test_and_branch)
	{
		const auto non_mask_bits=32U-14;	// 32 bits, imm14
		const auto mask14=(1<<14U)-1;
		assert((uint64_t)(new_offset << non_mask_bits) == ((uint64_t)new_offset) << non_mask_bits);
		const auto full_word_clean=full_word & ~(mask14<<5);
		const auto full_word_new_offset=full_word_clean | ((new_offset&mask14)<<5);
		memory_space[from_addr+0]=(full_word_new_offset>> 0)&0xff;
		memory_space[from_addr+1]=(full_word_new_offset>> 8)&0xff;
		memory_space[from_addr+2]=(full_word_new_offset>>16)&0xff;
		memory_space[from_addr+3]=(full_word_new_offset>>24)&0xff;
	}
	else 
		assert(0);

}

void ZiprPatcherARM64_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	return this->ApplyPatch(at_addr,to_addr);
}
void ZiprPatcherARM64_t::PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	assert(0);
}


void ZiprPatcherARM64_t::CallToNop(RangeAddress_t at_addr)
{
        assert(0);
}


