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
#include "patcher/patcher_mips32.hpp"
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

ZiprPatcherMIPS32_t::ZiprPatcherMIPS32_t(Zipr_SDK::Zipr_t* p_parent) :
	m_parent(dynamic_cast<zipr::ZiprImpl_t*>(p_parent)),     // upcast to ZiprImpl
        m_firp(p_parent->getFileIR()),
        memory_space(*p_parent->getMemorySpace())

{ 
}

void ZiprPatcherMIPS32_t::ApplyNopToPatch(RangeAddress_t addr)
{ assert(0); }

void ZiprPatcherMIPS32_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{ 
        const auto first_byte  = (uint8_t)memory_space[from_addr+0];
        const auto new_offset  = (int32_t)((to_addr) - (from_addr+8)) >> 2;

	// A Branch in binary is: op= 000010 imm26=0000  00000000  00000000  00000000
	// it includes a 26-bit immediate, which is +/- 128mb, which should be a good enough "jump anywhere"
	// for now.
	const auto mask6             = (0b111111);
	const auto is_uncond_branch  = ((first_byte >> 2) & mask6) == (0b000010);  // jump branch

	if(is_uncond_branch)
	{
		cout<<"Applying uncond branch patch from "<<hex<<from_addr<<" to "<<to_addr<<endl;
		const auto non_imm_bits = 32U-26U;	// 32 bits - imm26
		// assert there's no overflow.
		assert((int64_t)(new_offset << non_imm_bits) == ((int64_t)new_offset) << non_imm_bits);
		// or in opcode for first byte.  set remaining bytes.
		const auto mask26         = ((1<<26)-1);
		const auto trimmed_offset = new_offset & mask26;
		memory_space[from_addr+3]  = (trimmed_offset>> 0)&0xff;
		memory_space[from_addr+2]  = (trimmed_offset>> 8)&0xff;
		memory_space[from_addr+1]  = (trimmed_offset>>16)&0xff;
		memory_space[from_addr+0] |= (trimmed_offset>>24)&0x02;
	}
	else 
	{
		cout<<"Applying cond branch patch from "<<hex<<from_addr<<" to "<<to_addr<<endl;
		const auto non_imm_bits = 32U-24U;	// 32 bits - imm24
		// assert there's no overflow.
		assert((int64_t)(new_offset << non_imm_bits) == ((int64_t)new_offset) << non_imm_bits);
		// or in opcode for first byte.  set remaining bytes.
		const auto mask24         = ((1<<24)-1);
		const auto trimmed_offset = new_offset & mask24;
		memory_space[from_addr+3]  = (trimmed_offset>> 0)&0xff;
		memory_space[from_addr+2]  = (trimmed_offset>> 8)&0xff;
		memory_space[from_addr+1]  = (trimmed_offset>>16)&0xff;
		memory_space[from_addr+0] |= (trimmed_offset>>24)&0x02;
	}

}

void ZiprPatcherMIPS32_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	return this->ApplyPatch(at_addr,to_addr);
}
void ZiprPatcherMIPS32_t::PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	assert(0);
}


void ZiprPatcherMIPS32_t::CallToNop(RangeAddress_t at_addr)
{
        assert(0);
}


