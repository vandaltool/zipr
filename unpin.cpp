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

static std::string findAndReplace(const std::string& in_str, const std::string& oldStr, const std::string& newStr)
{
        std::string str=in_str;
        size_t pos = 0;
        while((pos = str.find(oldStr, pos)) != std::string::npos)
        {
                str.replace(pos, oldStr.length(), newStr);
                pos += newStr.length();
        }
        return str;
}

static bool has_cfi_reloc(Instruction_t* insn)
{
	for(auto reloc : insn->GetRelocations())
	{
		/* check for a nonce relocation */
		if ( reloc -> GetType().find("cfi_nonce") != string::npos )
		{
			return true;
		}
	}
	return false;
}

bool Unpin_t::should_cfi_pin(Instruction_t* insn)
{
	// add command line option that:
	// 	1) return false if !has_cfi_reloc(insn)
	// 	2) return true if option is on.
	return m_should_cfi_pin;
}

ZiprOptionsNamespace_t *Unpin_t::RegisterOptions(ZiprOptionsNamespace_t *global)
{
	auto unpin_ns = new ZiprOptionsNamespace_t("unpin");
	global->AddOption(&m_verbose);

	m_should_cfi_pin.SetDescription("Pin CFI instructions.");
	unpin_ns->AddOption(&m_should_cfi_pin);

	m_on.SetDescription("Turn unpin plugin on/off.");
	unpin_ns->AddOption(&m_on);

	m_max_unpins.SetDescription("Set how many unpins are allowed, useful for debugging.");
	unpin_ns->AddOption(&m_max_unpins);

	return unpin_ns;
}

// CAN BE DELETED, left in just for stats? (Would speed up zipr step to delete)
void Unpin_t::DoUnpin()
{
	DoUnpinForScoops();
	DoUnpinForFixedCalls();
}

// CAN BE DELETED, left in just for stats?
// scan instructions and process instruction relocs that can be unpinned.
void Unpin_t::DoUnpinForFixedCalls()
{
	if(m_max_unpins != -1 && unpins>=m_max_unpins)
		return;
	auto insn_unpins=0;
	auto missed_unpins=0;

	for(auto from_insn : zo->GetFileIR()->GetInstructions())
	{
		for(auto reloc : from_insn->GetRelocations())
		{
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->GetType()==string("32-bit") || reloc->GetType()==string("push64"))
			{
				// skip if there's no WRT, that means it's unpinned for something besides a fixed call.
				if(reloc->GetWRT()==NULL)
					continue;

				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				assert(wrt_insn);
		
				unpins++;
				insn_unpins++;
				if(m_max_unpins != -1 && unpins>=m_max_unpins)
					return;
			}
		}
	}

	cout<<"# ATTRIBUTE Zipr_Unpinning::insn_unpin_total_unpins="<<dec<<insn_unpins<<endl;
	cout<<"# ATTRIBUTE Zipr_Unpinning::insn_unpin_missed_unpins="<<dec<<missed_unpins<<endl;
}

// CAN BE DELETED, left in just for stats?
void Unpin_t::DoUnpinForScoops()
{
	if(m_max_unpins != -1 && unpins>=m_max_unpins)
		return;
	auto missed_unpins=0;
	auto scoop_unpins=0;

	for(auto scoop : zo->GetFileIR()->GetDataScoops())
	{
		for(auto reloc : scoop->GetRelocations())
		{
			if(reloc->GetType()==string("data_to_insn_ptr"))
			{
				auto insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				assert(insn);

				unpins++;
				scoop_unpins++;
				if(m_max_unpins != -1 && unpins>=m_max_unpins)
					return;
			}
		}
	}

	cout<<"# ATTRIBUTE Zipr_Unpinning::scoop_unpin_total_unpins="<<dec<<scoop_unpins<<endl;
	cout<<"# ATTRIBUTE Zipr_Unpinning::scoop_unpin_missed_unpins="<<dec<<missed_unpins<<endl;
}

Zipr_SDK::ZiprPreference Unpin_t::RetargetCallback(
	const RangeAddress_t &callback_address,
	const DollopEntry_t *callback_entry,
	RangeAddress_t &target_address)
{
	if(!m_on) return Zipr_SDK::ZiprPluginInterface_t::RetargetCallback(callback_address, callback_entry, target_address);

	unpins++;// unpinning a call to a scoop.
	if(m_max_unpins != -1 && unpins>=m_max_unpins)
		return Zipr_SDK::ZiprPluginInterface_t::RetargetCallback(callback_address, callback_entry, target_address);


	auto& ms=*zo->GetMemorySpace();
	auto  insn = callback_entry->Instruction();
	auto& locMap=*(zo->GetLocationMap());
	for(auto reloc : insn->GetRelocations())
	{
		if (reloc->GetType()==string("callback_to_scoop"))
		{
			auto wrt = dynamic_cast<DataScoop_t*>(reloc->GetWRT());
			auto addend = reloc->GetAddend();

			target_address = wrt->GetStart()->GetVirtualOffset() + addend;
		
			if (m_verbose) {
				cout << "Unpin::callback_to_scoop: target_addr "
				     << std::hex << target_address << endl;
			}
		}
	}
	return Must;
}

void Unpin_t::DoUpdate()
{
	DoUpdateForScoops();
	DoUpdateForInstructions();
}

// scan for instructions that were placed, and now need an update.
void Unpin_t::DoUpdateForInstructions()
{
	int unpins=0;
	int missed_unpins=0;
	auto& ms=*zo->GetMemorySpace();
	auto& locMap=*(zo->GetLocationMap());
	auto& firp=*(zo->GetFileIR());

	for(auto from_insn : zo->GetFileIR()->GetInstructions())
	{
		for(auto reloc : from_insn->GetRelocations())
		{
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->GetType()==string("32-bit") || reloc->GetType()==string("push64"))
			{
				// skip if there's no WRT, that means it's unpinned for something besides a fixed call.
				if(reloc->GetWRT()==NULL)
					continue;

				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				assert(wrt_insn);
				if(should_cfi_pin(wrt_insn)) 
					continue;

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
			// instruction has a pcrel memory operand.
			else if(reloc->GetType()==string("pcrel")) //  && reloc->GetWRT()!=NULL)
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

				if(mt==admtX86_64 || mt==admtI386)
				{
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
	                        else if(mt==admtAarch64)
                        	{
					assert(bo_wrt==nullptr); // not yet imp'd WRT offsetting.
					assert(to_addr==0); // not yet imp'd WRT offsetting.
					const auto mnemonic    =disasm.getMnemonic();
					const auto is_adr_type =mnemonic=="adr";
					const auto is_adrp_type=mnemonic=="adrp";
					const auto is_ldr_type =mnemonic=="ldr";
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
						assert( ((new_imm21_ext << 43) >> 43) == new_imm21_ext);
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
					else if(is_ldr_type)
					{
						// ldr: 0 x1 0110 0 0 imm19 Rt5
						const auto imm19    = ((int64_t)full_insn >> 5 ) & mask19;
						const auto imm19_ext= (imm19 << 45) >> 45;
						const auto referenced_addr=(imm19_ext<<2)+from_insn->GetAddress()->GetVirtualOffset()+4;
						const auto new_imm19_ext  =((int64_t)referenced_addr-(int64_t)from_insn_location-4+(int64_t)reloc->GetAddend()+(int64_t)to_addr)>>2;
						assert( ((new_imm19_ext << 45) >> 45) == new_imm19_ext);
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
						assert(0);



				}

			}
			// instruction has a absolute  memory operand that needs it's displacement updated.
			else if(reloc->GetType()==string("absoluteptr_to_scoop"))
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
			// instruction has an immediate that needs an update.
			else if(reloc->GetType()==string("immedptr_to_scoop"))
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
			else if(reloc->GetType()==string("callback_to_scoop"))
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
		}
	}
}

void Unpin_t::DoUpdateForScoops()
{
	auto byte_width=zo->GetFileIR()->GetArchitectureBitWidth()/8;
	for(auto scoop : zo->GetFileIR()->GetDataScoops())
	{
		assert(scoop->GetEnd()->GetVirtualOffset() - scoop->GetStart()->GetVirtualOffset()+1 == scoop->GetSize());
		assert(scoop->GetContents().size() == scoop->GetSize());
		auto scoop_contents=scoop->GetContents();

		for(auto reloc : scoop->GetRelocations())
		{
			if(reloc->GetType()==string("data_to_insn_ptr"))
			{
				virtual_offset_t reloff=reloc->GetOffset();
				Instruction_t* insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				assert(insn);
				Zipr_SDK::InstructionLocationMap_t &locMap=*(zo->GetLocationMap());
				libIRDB::virtual_offset_t newLoc=locMap[insn];

				cout<<"Unpin::Unpinned data_to_insn_ptr insn ("<<hex<<insn->GetBaseID()<<":"
				    <<insn->getDisassembly()<<") with offset="<<hex<<reloc->GetOffset()
				    <<".  Insn moved to "<<hex<<newLoc<<endl;

				bool found=should_cfi_pin(insn);

				/* don't unpin if we found one */
				if(found)
				{
					cout<<"Unpin::Skipping update because CFI is requesting a nonce."<<endl;
				}
				else
				{
					// determine how big the ptr is.
					int ptrsize=zo->GetFileIR()->GetArchitectureBitWidth()/8;
					char addr[ptrsize];
		
					// convert it to bytes.
					switch(ptrsize)
					{
						case 4:
							*(int*)addr=newLoc;
							break;
						case 8:
							*(long long*)addr=newLoc;
							break;
						default:
							assert(0);
					}
					// copy in new ptr.
					for(int i=0;i<ptrsize;i++)
						scoop_contents[reloff+i]=addr[i];
				}
				
			}
			else if(reloc->GetType()==string("dataptr_to_scoop"))
			{
				DataScoop_t *wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
				assert(wrt);

				virtual_offset_t val_to_patch=0;
                		const char* data=scoop_contents.c_str();

				if(byte_width==4)
					val_to_patch=*(int*)&data[reloc->GetOffset()];
				else if(byte_width==8)
					val_to_patch=*(long long*)&data[reloc->GetOffset()];
				else
					assert(0);

				// wrt scoop should be placed.
				assert(wrt->GetStart()->GetVirtualOffset() !=0 );
				virtual_offset_t new_val_to_patch=val_to_patch + wrt->GetStart()->GetVirtualOffset();

                                if(byte_width==4)
                                {
                                        unsigned int intnewval=(unsigned int)new_val_to_patch;     // 64->32 narrowing OK. 
                                        scoop_contents.replace(reloc->GetOffset(), byte_width, (char*)&intnewval, byte_width);
                                }
                                else if(byte_width==8)
                                {
                                        scoop_contents.replace(reloc->GetOffset(), byte_width, (char*)&new_val_to_patch, byte_width);
                                }
                                else
                                        assert(0);

				cout<<"Patched "<<scoop->GetName()<<"+"<<hex<<reloc->GetOffset()<<" to value "<<hex<<new_val_to_patch<<endl;
			}
		}
		scoop->SetContents(scoop_contents);
	}
}



extern "C" 
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
	Zipr_SDK::Zipr_t* zipr_object)
{
	return new Unpin_t(zipr_object);
}
