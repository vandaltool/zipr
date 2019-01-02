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
	for(auto from_insn : zo->GetFileIR()->GetInstructions())
	{
		for(auto reloc : from_insn->GetRelocations())
		{
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->GetType()==string("32-bit") || reloc->GetType()==string("push64"))
				HandleRetAddrReloc(from_insn,reloc);

			// instruction has a pcrel memory operand.
			else if(reloc->GetType()==string("pcrel")) //  && reloc->GetWRT()!=NULL)
				HandlePcrelReloc(from_insn,reloc);

			// instruction has a absolute  memory operand that needs it's displacement updated.
			else if(reloc->GetType()==string("absoluteptr_to_scoop"))
				HandleAbsptrReloc(from_insn,reloc);

			// instruction has an immediate that needs an update.
			else if(reloc->GetType()==string("immedptr_to_scoop"))
				HandleImmedptrReloc(from_insn,reloc);

			// deal with a callback, think this isn't used anymore
			else if(reloc->GetType()==string("callback_to_scoop"))
				HandleCallbackReloc(from_insn,reloc);
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
	const auto mt=zipr_object->GetFileIR()->GetArchitecture()->getMachineType();

	return 
		mt==admtX86_64  ? (Unpin_t*)new UnpinX86_t    (zipr_object) :
		mt==admtI386    ? (Unpin_t*)new UnpinX86_t    (zipr_object) :
		mt==admtAarch64 ? (Unpin_t*)new UnpinAarch64_t(zipr_object) :
		throw invalid_argument("Cannot determine machine type");
}
