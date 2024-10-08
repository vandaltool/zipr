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
	const auto mask6       = 0b111111;
        const auto first_byte  = (uint8_t)memory_space[from_addr+0];
        const auto second_byte = (uint8_t)memory_space[from_addr+1];
	const auto top6bits    = (first_byte >> 2) & mask6;
	const auto top16bits   = (uint32_t(first_byte) << 8) | second_byte;
	const auto top16bits_nocc  = top16bits & ~(0b11100);


	if(
		top6bits == 0b000100 ||  // beq, 
		top6bits == 0b000001 ||  // bgez, bgezal, bltz, bltzal
		top6bits == 0b000111 ||  // bgtz, 
		top6bits == 0b000110 ||  // blez, 
		top6bits == 0b000110 ||  // blez, 
		top6bits == 0b000101 ||  // bne
		top16bits_nocc == 0b0100010100000000 ||  // bc1f
		top16bits_nocc == 0b0100010100000001     // bc1t
		) 
	{
		const auto new_offset  = (int32_t)((to_addr) - (from_addr+4)) >> 2;
		// Use a branch always.  In mips, this will be a  beq $0, $0, <label> as there is no branch always.
		// format: 0001 00ss sstt iiii iiii iiii iiii iiii 
		// ssss=0b0000
		// tttt=0b0000
		// i...i = (from_addr-to_addr)>>2
		cout<<"Applying cond branch patch from "<<hex<<from_addr<<" to "<<to_addr<<endl;
		const auto non_imm_bits = 16;
		// assert there's no overflow.
		assert((int64_t)(new_offset << non_imm_bits) == ((int64_t)new_offset) << non_imm_bits);
		// or in opcode for first byte.  set remaining bytes.
		const auto mask16         = ((1<<16)-1);
		const auto trimmed_offset = new_offset & mask16;
		memory_space[from_addr+3]  = (trimmed_offset>> 0)&0xff;
		memory_space[from_addr+2]  = (trimmed_offset>> 8)&0xff;
	}
	else if(top6bits == 0b00010) /* j and jal */
	{
		const auto new_offset  = (int32_t)(to_addr) >> 2;
		cout<<"Applying uncond jump patch from "<<hex<<from_addr<<" to "<<to_addr<<endl;
		const auto non_imm_bits = 32-26;
		// assert there's no overflow.
		assert((int64_t)(new_offset << non_imm_bits) == ((int64_t)new_offset) << non_imm_bits);
		// or in opcode for first byte.  set remaining bytes.
		const auto mask26         = ((1<<26)-1);
		const auto trimmed_offset = new_offset & mask26;
		memory_space[from_addr+3]   = (trimmed_offset>> 0)&0b11111111;  /* low 8 bits */
		memory_space[from_addr+2]   = (trimmed_offset>> 8)&0b11111111;  /* 2nd 8 bits */
		memory_space[from_addr+1]   = (trimmed_offset>> 16)&0b11111111; /* 3rd 8 bits */
		memory_space[from_addr+0]  |= (trimmed_offset>> 24)&0b11;       /* last 2 bits of 26 bit address. */

	}
	else
		assert(0);

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


