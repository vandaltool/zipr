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

void UnpinX86_t::HandleRetAddrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{
	// skip if there's no WRT, that means it's unpinned for something besides a fixed call.
	if(reloc->GetWRT()==NULL)
		return;

	// getWRT returns an BaseObj, but this reloc type expects an instruction
	// safe cast and check.
	auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
	assert(wrt_insn);
	if(should_cfi_pin(wrt_insn)) 
		return;

	auto wrt_insn_location =locMap[wrt_insn];
	auto from_insn_location=locMap[from_insn];

	// 32-bit code and main executables just push a full 32-bit addr.
	if(zo->GetELFIO()->get_type()==ET_EXEC)
	{
// not handled in push64_relocs which is disabled for shared objects.
		// expecting a 32-bit push, length=5
		assert(from_insn->GetDataBits()[0]==0x68);
		assert(from_insn->GetDataBits().size()==5);
		// down and upcast to ensure we fit in 31-bits.
		assert(wrt_insn_location == (libIRDB::virtual_offset_t)(int)wrt_insn_location);
		assert(sizeof(int)==4); // paranoid.

		unsigned char newpush[5];
		newpush[0]=0x68;
		*(int*)&newpush[1]=(int)wrt_insn_location;

		cout<<"Unpin::Updating push32/push64-exe insn:"
		    <<dec<<from_insn->GetBaseID()<<":"<<from_insn->getDisassembly()<<"@"<<hex<<from_insn_location<<" to point at "
		    <<dec<<wrt_insn ->GetBaseID()<<":"<<wrt_insn ->getDisassembly()<<"@"<<hex<<wrt_insn_location <<endl;

		for(auto i=0U;i<from_insn->GetDataBits().size();i++)
		{ 
			unsigned char newbyte=newpush[i];
			ms[from_insn_location+i]=newbyte;
		}
	}
	// shared object
	// gets a call/sub [$rsp], const pair, handled in push64_relocs
	// else { }

}


void UnpinX86_t::HandlePcrelReloc(Instruction_t* from_insn, Relocation_t* reloc)
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

	const auto rel_addr1=the_arg.getMemoryDisplacement()+from_insn->GetDataBits().size();
	const auto disp_offset=(int)disasm.getMemoryDisplacementOffset(the_arg,from_insn); 
	const auto disp_size=(int)the_arg.getMemoryDisplacementEncodingSize(); 
	assert(disp_size==4);
	assert(0<disp_offset && disp_offset<=from_insn->GetDataBits().size() - disp_size);
		
	const auto new_disp=(int)(rel_addr1 + to_addr - from_insn->GetDataBits().size()-from_insn_location);
	const auto newbits=from_insn->GetDataBits().replace(disp_offset, disp_size, (char*)&new_disp, disp_size); 
	from_insn->SetDataBits(newbits);
	ms.PlopBytes(from_insn_location, newbits.c_str(), newbits.size());
	const auto disasm2=DecodedInstruction_t(from_insn);
	cout<<"unpin:pcrel:new_disp="<<hex<<new_disp<<endl;
	cout<<"unpin:pcrel:new_insn_addr="<<hex<<from_insn_location<<endl;
	cout<<"unpin:pcrel:Converting "<<hex<<from_insn->GetBaseID()<<":"<<disasm.getDisassembly() 
	    <<" to "<<disasm2.getDisassembly() <<" wrt "<< convert_string <<endl;

}

void UnpinX86_t::HandleAbsptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{
	const auto disasm=DecodedInstruction_t(from_insn);
	const auto operands=disasm.getOperands();

	// push/pop from memory might have a memory operand with no string to represent the implicit stack operand.
	const auto the_arg_it=find_if(ALLOF(operands),[](const DecodedOperand_t& op){ return op.isMemory() && op.getString()!=""; });
	DataScoop_t* wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
	assert(wrt);
	assert(the_arg_it!=operands.end());
	const auto &the_arg=*the_arg_it;
	virtual_offset_t rel_addr1=the_arg.getMemoryDisplacement(); 

	int disp_offset=disasm.getMemoryDisplacementOffset(the_arg,from_insn); 
	int disp_size=the_arg.getMemoryDisplacementEncodingSize(); 
	assert(disp_size==4);
	assert(0<disp_offset && disp_offset<=from_insn->GetDataBits().size() - disp_size);
	assert(reloc->GetWRT());

	unsigned int new_disp=the_arg.getMemoryDisplacement() + wrt->GetStart()->GetVirtualOffset();
	from_insn->SetDataBits(from_insn->GetDataBits().replace(disp_offset, disp_size, (char*)&new_disp, disp_size));
	// update the instruction in the memory space.
	libIRDB::virtual_offset_t from_insn_location=locMap[from_insn];
	for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
	{ 
		unsigned char newbyte=from_insn->GetDataBits()[i];
		ms[from_insn_location+i]=newbyte;

		//cout<<"Updating push["<<i<<"] from "<<hex<<oldbyte<<" to "<<newbyte<<endl;
	}
	const auto disasm2=DecodedInstruction_t(from_insn);
	cout<<"unpin:absptr_to_scoop:Converting "<<hex<<from_insn->GetBaseID()<<":"<<disasm.getDisassembly()
	    <<" to "<<disasm2.getDisassembly() <<" for scoop: "<<wrt->GetName()<<endl;
}

void UnpinX86_t::HandleImmedptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{
	DataScoop_t* wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
	assert(wrt);

	const auto disasm=DecodedInstruction_t(from_insn);
	virtual_offset_t rel_addr2=disasm.getImmediate(); 
	virtual_offset_t new_addr = rel_addr2 + wrt->GetStart()->GetVirtualOffset();

	from_insn->SetDataBits(from_insn->GetDataBits().replace(from_insn->GetDataBits().size()-4, 4, (char*)&new_addr, 4));

	libIRDB::virtual_offset_t from_insn_location=locMap[from_insn];
	for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
	{ 
		unsigned char newbyte=from_insn->GetDataBits()[i];
		ms[from_insn_location+i]=newbyte;

		//cout<<"Updating push["<<i<<"] from "<<hex<<oldbyte<<" to "<<newbyte<<endl;
	}

	const auto disasm2=DecodedInstruction_t(from_insn);
	cout<<"unpin:immedptr_to_scoop:Converting "<<hex<<from_insn->GetBaseID()<<":"<<disasm.getDisassembly() 
	    <<" to "<<disasm2.getDisassembly() <<" for scoop: "<<wrt->GetName()<<endl;

}

void UnpinX86_t::HandleCallbackReloc(Instruction_t* from_insn, Relocation_t* reloc)
{
	DataScoop_t *wrt = dynamic_cast<DataScoop_t*>(reloc->GetWRT());
	int addend = reloc->GetAddend();
	char bytes[]={(char)0x48,
		      (char)0x8d,
		      (char)0x64,
		      (char)0x24,
		      (char)(64/0x08)}; // lea rsp, [rsp+8]
	uintptr_t call_addr = 0x0, at = 0x0;
	uint32_t target_addr = 0x0;

	if (m_verbose)
		cout << "The call insn is " 
		     << from_insn->GetDataBits().length() << " bytes long." << endl;
	
	call_addr = locMap[from_insn];

	if (m_verbose) {
		cout << "Unpin::callback_to_scoop: call_addr " 
		     << std::hex << call_addr << endl;
	}

	/*
	 * Put down the bogus pop.
	 */
	at = call_addr + 1;
	at = call_addr + from_insn->GetDataBits().length();
	ms.PlopBytes(at, bytes, sizeof(bytes));

	/*
	 * Turn off the following flags so that this
	 * is left alone when it is being plopped.
	 */
	from_insn->SetTarget(NULL);
	from_insn->SetCallback("");
}

