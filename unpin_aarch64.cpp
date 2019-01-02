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


#include <zipr_sdk.h>
#include <string>
#include <algorithm>
#include "utils.hpp"
#include "Rewrite_Utility.hpp"
#include "unpin.h"
#include <memory>
#include <inttypes.h>


using namespace libIRDB;
using namespace std;
using namespace Zipr_SDK;
using namespace ELFIO;


#define ALLOF(a) begin(a),end(a)
// per machine stuff
void UnpinAarch64_t::HandleRetAddrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }


void UnpinAarch64_t::HandlePcrelReloc(Instruction_t* from_insn, Relocation_t* reloc)
{
	// decode the instruction and find the pcrel operand
	const auto disasm=DecodedInstruction_t(from_insn);
	const auto operands=disasm.getOperands();
	const auto the_arg_it=find_if(ALLOF(operands),[](const DecodedOperand_t& op){ return op.isPcrel(); });
	const auto bo_wrt=reloc->GetWRT();
	const auto scoop_wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
	const auto insn_wrt=dynamic_cast<Instruction_t*>(reloc->GetWRT());
	assert(the_arg_it!=operands.end());
	const auto the_arg=*the_arg_it;
	const auto mt=firp.GetArchitecture()->getMachineType();

	// get the new insn addr 	
	const auto from_insn_location=(virtual_offset_t)locMap[from_insn];

	// get WRT info
	libIRDB::virtual_offset_t to_addr=0xdeadbeef; // noteable value that shouldn't be used.
	string convert_string;

	if(scoop_wrt)
	{
		to_addr=scoop_wrt->GetStart()->GetVirtualOffset();
		convert_string=string("scoop ")+scoop_wrt->GetName();
	}
	else if(insn_wrt)
	{
		to_addr=locMap[insn_wrt];
		convert_string=string("insn ")+to_string(insn_wrt->GetBaseID())+
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
	const auto mnemonic        = disasm.getMnemonic();
	const auto is_adr_type     = mnemonic=="adr";
	const auto is_adrp_type    = mnemonic=="adrp";
	const auto is_ldr_type     = mnemonic=="ldr";
	const auto is_ldr_int_type = is_ldr_type && disasm.getOperand(0).isGeneralPurposeRegister();
	const auto is_ldr_fp_type  = is_ldr_type && disasm.getOperand(0).isFpuRegister();
	const auto is_ldrsw_type   = mnemonic=="ldrsw";
	const auto mask1 =(1<< 1)-1;
	const auto mask2 =(1<< 2)-1;
	const auto mask5 =(1<< 5)-1;
	const auto mask12=(1<<12)-1;
	const auto mask19=(1<<19)-1;
	const auto orig_insn_addr=from_insn->GetAddress()->GetVirtualOffset(); // original location
	const auto insn_bytes_len=4;	// arm is always 4.
	uint8_t insn_bytes[insn_bytes_len]; // compiler disallows init on some platforms.
	// but memcpy should init it sufficiently.
	memcpy(insn_bytes, from_insn->GetDataBits().c_str(), insn_bytes_len);
	const auto full_insn=*(uint32_t*)insn_bytes;
	const auto op_byte=insn_bytes[3];

	if(is_adrp_type || is_adr_type)
	{

		// adr : 0 immlo2 10000 immhi19 Rd5
		// adrp: 1 immlo2 10000 immhi19 Rd5
		assert((op_byte&mask5) == 0x10); // sanity check adr(p) opcode bytes.
		const auto immlo2    = (op_byte >> 5)&mask2; // grab immlo2
		const auto immhi19   = (full_insn >> 5)&mask19; // grab immhi19
		const auto imm21     = immhi19<<2 | immlo2;   // get full immediate in one field.
		const auto imm21_ext = (((int64_t)imm21)<<43) >> 43; // sign extend to 64-bit

		const auto shift_dist= 
			is_adrp_type ? 12 : // bits in page for adrp 
			is_adr_type  ?  0 : // no shift for adr
			throw invalid_argument("Unknown adr insn");

		const auto orig_insn_pageno = orig_insn_addr>>shift_dist;
		const auto new_insn_pageno  = from_insn_location>>shift_dist;
		const auto new_imm21_ext = imm21_ext + (int64_t)orig_insn_pageno - 
				(int64_t)new_insn_pageno + (int64_t)reloc->GetAddend()+(int64_t)to_addr;

		// make sure no overflow.
		if(((new_imm21_ext << 43) >> 43) == new_imm21_ext)
		{
			const auto new_immhi19   = new_imm21_ext >> 2;
			const auto new_immlo2    = new_imm21_ext  & mask2;
			const auto clean_new_insn= full_insn & ~(mask2<<29) & ~ (mask19 << 5);
			const auto new_insn      = clean_new_insn | ((new_immlo2&mask2) << 29) | ((new_immhi19&mask19)<<5);
			// put the new instruction in the output
			ms.PlopBytes(from_insn_location, (const char*)&new_insn, insn_bytes_len);
			if (m_verbose)
			{
				cout << "Relocating a adr(p) pcrel relocation with orig_pageno=" << hex
				     << (orig_insn_pageno << 12) << " offset=(page-pc+" << imm21_ext << ")"  << endl;
				cout << "Based on: " << disasm.getDisassembly() << hex << " originally at "  << orig_insn_addr
				     << " now located at : 0x" << hex << from_insn_location << " with offset=(page-pc + "
				     << new_imm21_ext << ")" << endl;
			}
		}
		else
		{
			assert(is_adr_type); // don't even know what to do if the PAGE is too far away!
			// imm21->64 bit address didn't work.  Split it up into two parts.

			/* the plan :
			 * FA: b   L0
			 * FT:
			 * ..
			 * L0  adrp dest_reg, <addr-page number>
			 * L1  add dest_reg, dest_reg, (addr-page offset)
			 * L2: b ft
			 */
			const auto tramp_size=3*4; // 3 insns, 4 bytes each
			const auto address_to_generate=imm21_ext+orig_insn_addr+(int64_t)reloc->GetAddend()+(int64_t)to_addr;
			const auto destreg=full_insn&mask5;
			const auto tramp_range=ms.GetFreeRange(tramp_size);
			const auto tramp_start=tramp_range.GetStart();
			// don't be too fancy, just reserve 12 bytes.
			ms.SplitFreeRange({tramp_start,tramp_start+12});


			const auto FA=from_insn_location;
			const auto FT=from_insn_location+4;
			const auto L0=tramp_start;
			const auto L1=tramp_start+4;
			const auto L2=tramp_start+8;
			const auto branch_bytes=string("\x00\x00\x00\x14",4);
			// const auto updated_orig_insn_pageno = orig_insn_addr>>12; // orig_insn_pageno was shifted by 0 for adr
			const auto relocd_insn_pageno  = L1>>12;
			const auto address_to_generate_pageno = address_to_generate >> 12;
			const auto address_to_generate_page_offset = address_to_generate & mask12;
			const auto relocd_imm21_ext = (int64_t)address_to_generate_pageno - (int64_t)relocd_insn_pageno;
			const auto relocd_immhi19   = relocd_imm21_ext >> 2;
			const auto relocd_immlo2    = relocd_imm21_ext  & mask2;

			// this should be +/- 4gb, so we shouldn't fail now!
			assert(((relocd_imm21_ext << 43) >> 43) == relocd_imm21_ext);

			// put an uncond branch at where the adr was.
			// and make it point at L0
			ms.PlopBytes(FA,branch_bytes.c_str(),4);
			zo->ApplyPatch(FA,L0);

			// adrp: 1 imm2lo 1 0000 immhi19 Rd
			auto adrp_bytes=string("\x00\x00\x00\x90",4);
			auto adrp_word =*(int*)adrp_bytes.c_str();
			adrp_word|=destreg<<0;
			adrp_word |=  ((relocd_immlo2&mask2) << 29) | ((relocd_immhi19&mask19)<<5);
			ms.PlopBytes(L0,(char*)&adrp_word,4);

			// add64 imm12 = 1001 0001 00 imm12 Rn Rd
			auto add_bytes=string("\x00\x00\x00\x91",4);
			auto add_word =*(int*)add_bytes.c_str();
			add_word|=destreg<<0;
			add_word|=destreg<<5;
			add_word|=address_to_generate_page_offset << 10 ;
			ms.PlopBytes(L1,(char*)&add_word,4);

			// put an uncond branch the end of the trampoline
			// and make it jump at FT
			ms.PlopBytes(L2,branch_bytes.c_str(),4);
			zo->ApplyPatch(L2,FT);

			// should be few enough of these to always print
			cout<< "Had to trampoline " << disasm.getDisassembly() << "@"<<FA<<" to "
			    << hex << L0 << "-" << L0+tramp_size << endl;
		}
	}
	else if(is_ldr_type)
	{
		// ldr w/x reg    : 0 x1 0110 0 0 imm19 Rt5, x1   indicate size (0,1 -> w/x) 
		// ldr s/d/q reg  : opc2 0111 0 0 imm19 Rt5, opc2 indicate size (00,01,10 -> s/d/q)
		const auto imm19    = ((int64_t)full_insn >> 5 ) & mask19;
		const auto imm19_ext= (imm19 << 45) >> 45;
		const auto referenced_addr=(imm19_ext<<2)+from_insn->GetAddress()->GetVirtualOffset()+4;
		const auto new_imm19_ext  =((int64_t)referenced_addr-(int64_t)from_insn_location-4+(int64_t)reloc->GetAddend()+(int64_t)to_addr)>>2;
		if( ((new_imm19_ext << 45) >> 45) == new_imm19_ext)
		{
			const auto clean_new_insn = full_insn & ~(mask19 << 5);
			const auto new_insn       = clean_new_insn | ((new_imm19_ext & mask19)<<5);
			// put the new instruction in the output
			ms.PlopBytes(from_insn_location, (const char*)&new_insn, insn_bytes_len);
			if (m_verbose)
			{
				cout << "Relocating a ldr pcrel relocation with orig_addr=" << hex
				     << (referenced_addr) << " offset=(pc+" << imm19_ext << ")"  << endl;
				cout << "Based on: " << disasm.getDisassembly() 
				     << " now located at : 0x" << hex << from_insn_location << " with offset=(pc + "
				     << new_imm19_ext << ")" << endl;
			}
		}
		else
		{
			const auto address_to_generate=(imm19_ext<<2)+orig_insn_addr+(int64_t)reloc->GetAddend()+(int64_t)to_addr;
			const auto destreg=full_insn&mask5;
			const auto FA=from_insn_location;
			const auto FT=from_insn_location+4;
			const auto branch_bytes=string("\x00\x00\x00\x14",4);
			const auto address_to_generate_pageno = address_to_generate >> 12;
			const auto address_to_generate_page_offset = address_to_generate & mask12;

			if(is_ldr_int_type)
			{
				// imm19->64 bit address didn't work.  Split it up into two parts.

				/* the plan :
				 * FA: b   L0
				 * FT:
				 * ..
				 * L0  adrp dest_reg, <addr-page number>
				 * L1  ldr dest_reg, [dst_reg, #addr-page offset]
				 * L2: b ft
				 */
				const auto tramp_size=3*4; // 3 insns, 4 bytes each
				const auto tramp_range=ms.GetFreeRange(tramp_size);
				const auto tramp_start=tramp_range.GetStart();
				// don't be too fancy, just reserve 12 bytes.
				ms.SplitFreeRange({tramp_start,tramp_start+12});


				// and give the bytes some names
				const auto L0=tramp_start;
				const auto L1=tramp_start+4;
				const auto L2=tramp_start+8;

				// calculate the immediates for the new adr and ldr instruction.
				const auto relocd_insn_pageno  = L1>>12;
				const auto relocd_imm21_ext = (int64_t)address_to_generate_pageno - (int64_t)relocd_insn_pageno;
				const auto relocd_immhi19   = relocd_imm21_ext >> 2;
				const auto relocd_immlo2    = relocd_imm21_ext  & mask2;
				// this should be +/- 4gb, so we shouldn't fail now!
				assert(((relocd_imm21_ext << 43) >> 43) == relocd_imm21_ext);


				// put an uncond branch at where the adr was.
				// and make it point at L0
				ms.PlopBytes(FA,branch_bytes.c_str(),4);
				zo->ApplyPatch(FA,L0);

				// adrp: 1 imm2lo 1 0000 immhi19 Rd
				auto adrp_bytes=string("\x00\x00\x00\x90",4);
				auto adrp_word =*(int*)adrp_bytes.c_str();
				adrp_word|=destreg<<0;
				adrp_word |=  ((relocd_immlo2&mask2) << 29) | ((relocd_immhi19&mask19)<<5);
				ms.PlopBytes(L0,(char*)&adrp_word,4);

				// convert: ldr w/x reg : 0 x1 011 0 00 ---imm19---- Rt5    x1 indicates size (0,1 -> w/x) 
				// to     : ldr x/w reg : 1 x1 111 0 01 01 imm12 Rn5 Rt5    x1 indciates size (0,1 -> w/x)
				auto new_ldr_bytes=string("\x00\x00\x40\xb9",4);
				auto new_ldr_word =*(int*)new_ldr_bytes.c_str();
				const auto orig_ldr_size_bit=(full_insn>>30)&mask1;
				const auto scale=0x2|orig_ldr_size_bit;
				const auto scaled_page_offset=(address_to_generate_page_offset>>scale) ;
				new_ldr_word|=destreg<<0; // Rt
				new_ldr_word|=destreg<<5; // Rn
				new_ldr_word|=scaled_page_offset << 10 ; // imm12
				new_ldr_word|=orig_ldr_size_bit << 30; // x1
				ms.PlopBytes(L1,(char*)&new_ldr_word,4);

				// put an uncond branch the end of the trampoline
				// and make it jump at FT
				ms.PlopBytes(L2,branch_bytes.c_str(),4);
				zo->ApplyPatch(L2,FT);

				// should be few enough of these to always print
				cout<< "Had to trampoline " << disasm.getDisassembly() << "@"<<FA<<" to "
				    << hex << L0 << "-" << L0+tramp_size-1 << endl;

			}
			else if(is_ldr_fp_type)
			{
				/* the scheme for int-type operations doesn't work
				 * for fp-type because  there is no free register.
				 * use this plan:
				 * FA: b   L0
				 * FT:
				 * ..
				 * L0  str x0, [sp+128] ; 128 for red zoning
				 * L1  adrp x0, <addr-page number>
				 * L2  ldr dest_reg, [x0, #addr-page offset]
				 * L3  ldr x0, [sp+128] ; 128 for red zoning
				 * L4  b FT
				 */



				// allocate and reserve space for the code.
				const auto tramp_size=5*4; // 3 insns, 4 bytes each
				const auto tramp_range=ms.GetFreeRange(tramp_size);
				const auto tramp_start=tramp_range.GetStart();
				// don't be too fancy, just reserve 12 bytes.
				ms.SplitFreeRange({tramp_start,tramp_start+12});


				// give the bytes some names
				const auto L0=tramp_start   ;
				const auto L1=tramp_start+4 ;
				const auto L2=tramp_start+8 ;
				const auto L3=tramp_start+12;
				const auto L4=tramp_start+16;

				// calculate the immediates for the new adr and ldr instruction.
				const auto relocd_insn_pageno  = L1>>12;
				const auto relocd_imm21_ext    = (int64_t)address_to_generate_pageno - (int64_t)relocd_insn_pageno;
				const auto relocd_immhi19      = relocd_imm21_ext >> 2;
				const auto relocd_immlo2       = relocd_imm21_ext  & mask2;
				// this should be +/- 4gb, so we shouldn't fail now!
				assert(((relocd_imm21_ext << 43) >> 43) == relocd_imm21_ext);

				// put an uncond branch at where the adr was.
				// and make it point at L0
				ms.PlopBytes(FA,branch_bytes.c_str(),4);
				zo->ApplyPatch(FA,L0);

				// put save of x0 in place.
				// diassembly: f81803e0        stur    x0, [sp, #-128]
				const auto strx0_bytes=string("\xe0\x03\x18\xf8",4);
				ms.PlopBytes(L0,strx0_bytes.c_str(),4);

				// adrp: 1 imm2lo 1 0000 immhi19 Rd
				auto adrp_bytes=string("\x00\x00\x00\x90",4);
				auto adrp_word =*(int*)adrp_bytes.c_str();
				// adrp_word|=destreg<<0; ; destreg for this insn is x0.
				adrp_word |=  ((relocd_immlo2&mask2) << 29) | ((relocd_immhi19&mask19)<<5);
				ms.PlopBytes(L1,(char*)&adrp_word,4);


				// convert: ldr   s/d/q reg: opc2  01 11 00 imm19 Rt5, opc2 indicate size (00,01,10 -> s/d/q)
				// to:      ldr b/s/d/q reg: size2 11 11 01 opc2 imm12 Rn Rt
				auto new_ldr_bytes=string("\x00\x00\x00\x3d",4);
				auto new_ldr_word =*(int*)new_ldr_bytes.c_str();
				const auto orig_ldr_opc_bits=(full_insn>>30)&mask2;

				// decode size out of old ldr
				const auto ldr_size= 
					orig_ldr_opc_bits == 0x0 ? 4u  :
					orig_ldr_opc_bits == 0x1 ? 8u  :
					orig_ldr_opc_bits == 0x2 ? 16u :
					throw invalid_argument("cannot decode ldr floating-point access size");

				// encode size field for new ldr.
				const auto new_ldr_size_bits=
					ldr_size == 4  ? 0x2u :
					ldr_size == 8  ? 0x3u :
					ldr_size == 16 ? 0x0u :
					throw invalid_argument("cannot decode ldr floating-point access size");

				// encode opc2
				const auto new_ldr_opc2_bits=
					ldr_size == 4  ? 0x1u :
					ldr_size == 8  ? 0x1u :
					ldr_size == 16 ? 0x3u :
					throw invalid_argument("cannot decode ldr floating-point access size");

				// add variable fields to new insn and drop it in the mem space.
				new_ldr_word|=destreg<<0; // Rt -- should be actual dest reg, not x0
				// new_ldr_word|=destreg<<5; // Rn -- should be x0
				new_ldr_word|=((address_to_generate_page_offset/ldr_size) << 10); // imm12
				new_ldr_word|=(new_ldr_size_bits<<30); // size2
				new_ldr_word|=(new_ldr_opc2_bits<<22); // opc2
				ms.PlopBytes(L2,(char*)&new_ldr_word,4);


				// drop in the restore of x0
				// disassembly:   f85803e0        ldur    x0, [sp, #-128]
				const auto ldrx0_bytes=string("\xe0\x03\x58\xf8",4);
				ms.PlopBytes(L3,ldrx0_bytes.c_str(),4);

				// put an uncond branch the end of the trampoline
				// and make it jump at FT
				ms.PlopBytes(L4,branch_bytes.c_str(),4);
				zo->ApplyPatch(L4,FT);

				// should be few enough of these to always print
				cout<< "Had to trampoline " << disasm.getDisassembly() << "@"<<FA<<" to "
				    << hex << L0 << "-" << L0+tramp_size-1 << endl;

			}
			else
				assert(0);
		}

	}
	else if(is_ldrsw_type)
	{
		// ldrsw x reg    : 1001 1000 imm19 Rt
		const auto imm19    = ((int64_t)full_insn >> 5 ) & mask19;
		const auto imm19_ext= (imm19 << 45) >> 45;
		const auto referenced_addr=(imm19_ext<<2)+from_insn->GetAddress()->GetVirtualOffset()+4;
		const auto new_imm19_ext  =((int64_t)referenced_addr-(int64_t)from_insn_location-4+(int64_t)reloc->GetAddend()+(int64_t)to_addr)>>2;
		if( ((new_imm19_ext << 45) >> 45) == new_imm19_ext)
		{
			const auto clean_new_insn = full_insn & ~(mask19 << 5);
			const auto new_insn       = clean_new_insn | ((new_imm19_ext & mask19)<<5);
			// put the new instruction in the output
			ms.PlopBytes(from_insn_location, (const char*)&new_insn, insn_bytes_len);
			if (m_verbose)
			{
				cout << "Relocating a ldrsw pcrel relocation with orig_addr=" << hex
				     << (referenced_addr) << " offset=(pc+" << imm19_ext << ")"  << endl;
				cout << "Based on: " << disasm.getDisassembly() 
				     << " now located at : 0x" << hex << from_insn_location << " with offset=(pc + "
				     << new_imm19_ext << ")" << endl;
			}
		}
		else
		{
			// imm19->64 bit address didn't work.  Split it up into two parts.

			/* the plan :
			 * FA: b   L0
			 * FT:
			 * ..
			 * L0  adrp dest_reg, <addr-page number>
			 * L1  ldr dest_reg, [dst_reg, #addr-page offset]
			 * L2: b ft
			 */
			const auto tramp_size=3*4; // 3 insns, 4 bytes each
			const auto address_to_generate=(imm19_ext<<2)+orig_insn_addr+(int64_t)reloc->GetAddend()+(int64_t)to_addr;
			const auto destreg=full_insn&mask5;
			const auto tramp_range=ms.GetFreeRange(tramp_size);
			const auto tramp_start=tramp_range.GetStart();
			// don't be too fancy, just reserve 12 bytes.
			ms.SplitFreeRange({tramp_start,tramp_start+12});


			const auto FA=from_insn_location;
			const auto FT=from_insn_location+4;
			const auto L0=tramp_start;
			const auto L1=tramp_start+4;
			const auto L2=tramp_start+8;
			const auto branch_bytes=string("\x00\x00\x00\x14",4);
			const auto relocd_insn_pageno  = L1>>12;
			const auto address_to_generate_pageno = address_to_generate >> 12;
			const auto address_to_generate_page_offset = address_to_generate & mask12;
			const auto relocd_imm21_ext = (int64_t)address_to_generate_pageno - (int64_t)relocd_insn_pageno;
			const auto relocd_immhi19   = relocd_imm21_ext >> 2;
			const auto relocd_immlo2    = relocd_imm21_ext  & mask2;

			// this should be +/- 4gb, so we shouldn't fail now!
			assert(((relocd_imm21_ext << 43) >> 43) == relocd_imm21_ext);

			// put an uncond branch at where the adr was.
			// and make it point at L0
			ms.PlopBytes(FA,branch_bytes.c_str(),4);
			zo->ApplyPatch(FA,L0);

			// adrp: 1 imm2lo 1 0000 immhi19 Rd
			auto adrp_bytes=string("\x00\x00\x00\x90",4);
			auto adrp_word =*(int*)adrp_bytes.c_str();
			adrp_word|=destreg<<0;
			adrp_word |=  ((relocd_immlo2&mask2) << 29) | ((relocd_immhi19&mask19)<<5);
			ms.PlopBytes(L0,(char*)&adrp_word,4);

			// convert: ldrsw x reg : 1001 1000 ---imm19--- Rt
			// to     : ldrsw x reg : 1011 1001 10 imm12 Rn Rt
			auto new_ldr_bytes=string("\x00\x00\x80\xb9",4);
			auto new_ldr_word =*(int*)new_ldr_bytes.c_str();
			const auto scale=0x2;
			const auto scaled_page_offset=(address_to_generate_page_offset>>scale) ;
			new_ldr_word|=destreg<<0; // Rt
			new_ldr_word|=destreg<<5; // Rn
			new_ldr_word|=scaled_page_offset << 10 ; // imm12
			ms.PlopBytes(L1,(char*)&new_ldr_word,4);

			// put an uncond branch the end of the trampoline
			// and make it jump at FT
			ms.PlopBytes(L2,branch_bytes.c_str(),4);
			zo->ApplyPatch(L2,FT);

			// should be few enough of these to always print
			cout<< "Had to trampoline " << disasm.getDisassembly() << "@"<<FA<<" to "
			    << hex << L0 << "-" << L0+tramp_size-1 << endl;
		}
	}
}


void UnpinAarch64_t::HandleAbsptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); } 


void UnpinAarch64_t::HandleImmedptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }

void UnpinAarch64_t::HandleCallbackReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }




