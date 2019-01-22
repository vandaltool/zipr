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
#include "patcher/patcher_x86.hpp"
}
#include <libIRDB-core.hpp>
#include <Rewrite_Utility.hpp>
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

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

#define ALLOF(a) begin(a),end(a)

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;
using namespace IRDBUtility;

ZiprPatcherX86_t::ZiprPatcherX86_t(Zipr_SDK::Zipr_t* p_parent) :
	m_parent(dynamic_cast<zipr::ZiprImpl_t*>(p_parent)),     // upcast to ZiprImpl
	m_firp(p_parent->GetFileIR()),
	memory_space(*p_parent->GetMemorySpace())
{
}

void ZiprPatcherX86_t::RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos)
{
	int new_offset=((unsigned int)to_addr)-((unsigned int)from_addr)-((unsigned int)insn_length);

	memory_space[from_addr+offset_pos+0]=(new_offset>>0)&0xff;
	memory_space[from_addr+offset_pos+1]=(new_offset>>8)&0xff;
	memory_space[from_addr+offset_pos+2]=(new_offset>>16)&0xff;
	memory_space[from_addr+offset_pos+3]=(new_offset>>24)&0xff;
}

void ZiprPatcherX86_t::ApplyNopToPatch(RangeAddress_t addr)
{
	/*
	 * TODO: Add assertion that this is really a patch.
	 */

	/*
	 * 0F 1F 44 00 00H
	 */
	memory_space[addr] = (unsigned char)0x0F;
	memory_space[addr+1] = (unsigned char)0x1F;
	memory_space[addr+2] = (unsigned char)0x44;
	memory_space[addr+3] = (unsigned char)0x00;
	memory_space[addr+4] = (unsigned char)0x00;
}

void ZiprPatcherX86_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{
	unsigned char insn_first_byte=memory_space[from_addr];
	unsigned char insn_second_byte=memory_space[from_addr+1];

	switch(insn_first_byte)
	{
		case (unsigned char)0xF: // two byte escape
		{
			assert( insn_second_byte==(unsigned char)0x80 ||	// should be a JCC 
				insn_second_byte==(unsigned char)0x81 ||
				insn_second_byte==(unsigned char)0x82 ||
				insn_second_byte==(unsigned char)0x83 ||
				insn_second_byte==(unsigned char)0x84 ||
				insn_second_byte==(unsigned char)0x85 ||
				insn_second_byte==(unsigned char)0x86 ||
				insn_second_byte==(unsigned char)0x87 ||
				insn_second_byte==(unsigned char)0x88 ||
				insn_second_byte==(unsigned char)0x89 ||
				insn_second_byte==(unsigned char)0x8a ||
				insn_second_byte==(unsigned char)0x8b ||
				insn_second_byte==(unsigned char)0x8c ||
				insn_second_byte==(unsigned char)0x8d ||
				insn_second_byte==(unsigned char)0x8e ||
				insn_second_byte==(unsigned char)0x8f );

			RewritePCRelOffset(from_addr,to_addr,6,2);
			break;
		}

		case (unsigned char)0xe8:	// call
		case (unsigned char)0xe9:	// jmp
		{
			RewritePCRelOffset(from_addr,to_addr,5,1);
			break;
		}

		case (unsigned char)0xf0: // lock
		case (unsigned char)0xf2: // rep/repe
		case (unsigned char)0xf3: // repne
		case (unsigned char)0x2e: // cs override
		case (unsigned char)0x36: // ss override
		case (unsigned char)0x3e: // ds override
		case (unsigned char)0x26: // es override
		case (unsigned char)0x64: // fs override
		case (unsigned char)0x65: // gs override
		case (unsigned char)0x66: // operand size override
		case (unsigned char)0x67: // address size override
		{
			cout << "found patch for instruction with prefix.  prefix is: "<<hex<<insn_first_byte<<".  Recursing at "<<from_addr+1<<dec<<endl;
			// recurse at addr+1 if we find a prefix byte has been plopped.
			return this->ApplyPatch(from_addr+1, to_addr);
		}
		default:
		{
			if(m_firp->GetArchitectureBitWidth()==64) /* 64-bit x86 machine  assumed */
			{
				/* check for REX prefix */
				if((unsigned char)0x40 <= insn_first_byte  && insn_first_byte <= (unsigned char)0x4f)
				{
					cout << "found patch for instruction with prefix.  prefix is: "<<hex<<insn_first_byte<<".  Recursing at "<<from_addr+1<<dec<<endl;
					// recurse at addr+1 if we find a prefix byte has been plopped.
					return this->ApplyPatch(from_addr+1, to_addr);
				}
			}
			std::cerr << "insn_first_byte: 0x" << hex << (int)insn_first_byte << dec << std::endl;
			assert(0);
		}
	}
}

void ZiprPatcherX86_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	uintptr_t off=to_addr-at_addr-2;

	assert(!memory_space.IsByteFree(at_addr));
	
	switch(memory_space[at_addr])
	{
		case (char)0xe9:	/* 5byte jump */
		{
			RewritePCRelOffset(at_addr,to_addr,5,1);
			break;
		}
		case (char)0xeb:	/* 2byte jump */
		{
			assert(off==(uintptr_t)(char)off);

			assert(!memory_space.IsByteFree(at_addr+1));
			memory_space[at_addr+1]=(char)off;
			break;
		}
		default:
		{
			assert(false);
		}
	}
}



void ZiprPatcherX86_t::PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
        uintptr_t off=to_addr-at_addr-5;

        assert(!memory_space.IsByteFree(at_addr));

        switch(memory_space[at_addr])
        {
                case (char)0xe8:        /* 5byte call */
                {
                        assert(off==(uintptr_t)off);
                        assert(!memory_space.AreBytesFree(at_addr+1,4));

                        memory_space[at_addr+1]=(char)(off>> 0)&0xff;
                        memory_space[at_addr+2]=(char)(off>> 8)&0xff;
                        memory_space[at_addr+3]=(char)(off>>16)&0xff;
                        memory_space[at_addr+4]=(char)(off>>24)&0xff;
                        break;
                }
                default:
                        assert(0);

        }
}

void ZiprPatcherX86_t::CallToNop(RangeAddress_t at_addr)
{
        char bytes[]={(char)0x90,(char)0x90,(char)0x90,(char)0x90,(char)0x90}; // nop;nop;nop;nop;nop
        memory_space.PlopBytes(at_addr,bytes,sizeof(bytes));
}



