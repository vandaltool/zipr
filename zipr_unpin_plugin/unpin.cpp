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
#include <cstring>
#include <algorithm>
#include "unpin.h"
#include <memory>
#include <inttypes.h>


using namespace IRDB_SDK;
using namespace std;
using namespace Zipr_SDK;


#define ALLOF(a) begin(a),end(a)

bool Unpin_t::should_cfi_pin(Instruction_t* insn)
{
	// add command line option that:
	// 	1) return false if !has_cfi_reloc(insn)
	// 	2) return true if option is on.
	return *m_should_cfi_pin;
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
	if((int64_t)*m_max_unpins != (int64_t)-1 && (int64_t)unpins>=(int64_t)*m_max_unpins)
		return;
	auto insn_unpins=0;
	auto missed_unpins=0;

	for(auto from_insn : zo->getFileIR()->getInstructions())
	{
		for(auto reloc : from_insn->getRelocations())
		{
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->getType()==string("32-bit") || reloc->getType()==string("push64"))
			{
				// skip if there's no WRT, that means it's unpinned for something besides a fixed call.
				if(reloc->getWRT()==NULL)
					continue;

				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
				assert(wrt_insn);
		
				unpins++;
				insn_unpins++;
				if((int64_t)*m_max_unpins != (int64_t)-1 && (int64_t)unpins>=(int64_t)*m_max_unpins)
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
	if((int64_t)*m_max_unpins != (int64_t)-1 && (int64_t)unpins>=(int64_t)*m_max_unpins)
		return;

	auto missed_unpins=0;
	auto scoop_unpins=0;

	for(auto scoop : zo->getFileIR()->getDataScoops())
	{
		for(auto reloc : scoop->getRelocations())
		{
			if(reloc->getType()==string("data_to_insn_ptr"))
			{
				auto insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				assert(insn);

				unpins++;
				scoop_unpins++;
				if((int64_t)*m_max_unpins != (int64_t)-1 && (int64_t)unpins>=(int64_t)*m_max_unpins)
					return;
			}
		}
	}

	cout<<"# ATTRIBUTE Zipr_Unpinning::scoop_unpin_total_unpins="<<dec<<scoop_unpins<<endl;
	cout<<"# ATTRIBUTE Zipr_Unpinning::scoop_unpin_missed_unpins="<<dec<<missed_unpins<<endl;
}

Zipr_SDK::ZiprPreference Unpin_t::retargetCallback(
	const RangeAddress_t &callback_address,
	const DollopEntry_t *callback_entry,
	RangeAddress_t &target_address)
{
	if(!*m_on) return Zipr_SDK::ZiprPluginInterface_t::retargetCallback(callback_address, callback_entry, target_address);

	unpins++;// unpinning a call to a scoop.
	if((int64_t)*m_max_unpins != (int64_t)-1 && (int64_t)unpins>=(int64_t)*m_max_unpins)
		return Zipr_SDK::ZiprPluginInterface_t::retargetCallback(callback_address, callback_entry, target_address);


	auto  insn = callback_entry->getInstruction();
	for(auto reloc : insn->getRelocations())
	{
		if (reloc->getType()==string("callback_to_scoop"))
		{
			auto wrt = dynamic_cast<DataScoop_t*>(reloc->getWRT());
			auto addend = reloc->getAddend();

			target_address = wrt->getStart()->getVirtualOffset() + addend;
		
			if (*m_verbose) {
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
	for(auto from_insn : zo->getFileIR()->getInstructions())
	{
		for(auto reloc : from_insn->getRelocations())
		{
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->getType()==string("32-bit") || reloc->getType()==string("push64"))
				HandleRetAddrReloc(from_insn,reloc);

			// instruction has a pcrel memory operand.
			else if(reloc->getType()==string("pcrel")) //  && reloc->getWRT()!=NULL)
				HandlePcrelReloc(from_insn,reloc);

			// instruction has a absolute  memory operand that needs it's displacement updated.
			else if(reloc->getType()==string("absoluteptr_to_scoop"))
				HandleAbsptrReloc(from_insn,reloc);

			// instruction has an immediate that needs an update.
			else if(reloc->getType()==string("immedptr_to_scoop"))
				HandleImmedptrReloc(from_insn,reloc);

			// deal with a callback, think this isn't used anymore
			else if(reloc->getType()==string("callback_to_scoop"))
				HandleCallbackReloc(from_insn,reloc);
		}
	}
}

void Unpin_t::DoUpdateForScoops()
{
	auto byte_width=zo->getFileIR()->getArchitectureBitWidth()/8;
	for(auto scoop : zo->getFileIR()->getDataScoops())
	{
		if(scoop->isExecuteable()) continue;
		assert(scoop->getEnd()->getVirtualOffset() - scoop->getStart()->getVirtualOffset()+1 == scoop->getSize());
		assert(scoop->getContents().size() == scoop->getSize());
		auto scoop_contents=scoop->getContents();

		for(auto reloc : scoop->getRelocations())
		{
			if(reloc->getType()==string("switch_type4"))
			{
				const auto reloff=reloc->getOffset();
				auto insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				assert(insn);
				auto &locMap=*(zo->getLocationMap());
				const auto newLoc=locMap[insn];

				cout << "Unpin::Unpinned switch_type4 insn (" << hex << insn->getBaseID() << ":"
				     << insn->getDisassembly() << ") with offset=" << hex << reloc->getOffset()
				     << ".  Insn moved to " << hex << newLoc << endl;

				/* don't unpin if we found one */
				if(should_cfi_pin(insn))
				{
					cout<<"Unpin::Skipping update because CFI is requesting a nonce."<<endl;
					continue;
				}

				// load the scoop contents
				auto oldVal=0u;
				memcpy(&oldVal, &(scoop_contents.c_str()[reloff]), 4);
				const auto newValue=int32_t(oldVal + reloc->getAddend() + newLoc);

				cout << "Adjusting  to " << newValue 
					 << "(0x" << oldVal <<  " + 0x" 
					 << reloc->getAddend() << " + " << newLoc << ")\n";
				for(auto i=0u; i < 4u; i++)
					scoop_contents[reloff+i]=reinterpret_cast<const char*>(&newValue)[i];

			}
			else if(reloc->getType()==string("data_to_insn_ptr"))
			{
				VirtualOffset_t reloff=reloc->getOffset();
				Instruction_t* insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				assert(insn);
				Zipr_SDK::InstructionLocationMap_t &locMap=*(zo->getLocationMap());
				IRDB_SDK::VirtualOffset_t newLoc=locMap[insn];

				cout<<"Unpin::Unpinned data_to_insn_ptr insn ("<<hex<<insn->getBaseID()<<":"
				    <<insn->getDisassembly()<<") with offset="<<hex<<reloc->getOffset()
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
					const int ptrsize=zo->getFileIR()->getArchitectureBitWidth()/8;
					char addr[ptrsize];
					memset(addr,0,ptrsize);
		
					// convert it to bytes.
					switch(ptrsize)
					{
						case 4:
						{
							const auto newVal=(int)newLoc;
							memcpy(addr,&newVal,ptrsize);
							break;
						}
						case 8:
						{
							const auto newVal=(long long)newLoc;
							memcpy(addr,&newVal,ptrsize);
							break;
						}
						default:
							assert(0);
					}
					// copy in new ptr.
					for(int i=0;i<ptrsize;i++)
						scoop_contents[reloff+i]=addr[i];
				}
				
			}
			else if(reloc->getType()==string("dataptr_to_scoop"))
			{
				DataScoop_t *wrt=dynamic_cast<DataScoop_t*>(reloc->getWRT());
				assert(wrt);

				VirtualOffset_t val_to_patch=0;
                		const char* data=scoop_contents.c_str();

				if(byte_width==4)
				{
					auto newVal=(int)0;
					memcpy(&newVal, &data[reloc->getOffset()], byte_width);
					val_to_patch=newVal;
				}
				else if(byte_width==8)
				{
					auto newVal=(long long)0;
					memcpy(&newVal, &data[reloc->getOffset()], byte_width);
					val_to_patch=newVal;
				}
				else
					assert(0);

				// wrt scoop should be placed.
				assert(wrt->getStart()->getVirtualOffset() !=0 );
				VirtualOffset_t new_val_to_patch=val_to_patch + wrt->getStart()->getVirtualOffset();

                                if(byte_width==4)
                                {
                                        unsigned int intnewval=(unsigned int)new_val_to_patch;     // 64->32 narrowing OK. 
                                        scoop_contents.replace(reloc->getOffset(), byte_width, (char*)&intnewval, byte_width);
                                }
                                else if(byte_width==8)
                                {
                                        scoop_contents.replace(reloc->getOffset(), byte_width, (char*)&new_val_to_patch, byte_width);
                                }
                                else
                                        assert(0);

				cout<<"Patched "<<scoop->getName()<<"+"<<hex<<reloc->getOffset()<<" to value "<<hex<<new_val_to_patch<<endl;
			}
		}
		scoop->setContents(scoop_contents);
	}
}


extern "C" 
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
	Zipr_SDK::Zipr_t* zipr_object)
{
	const auto mt=zipr_object->getFileIR()->getArchitecture()->getMachineType();

	return 
		mt==admtX86_64  ? (Unpin_t*)new UnpinX86_t    (zipr_object) :
		mt==admtI386    ? (Unpin_t*)new UnpinX86_t    (zipr_object) :
		mt==admtAarch64 ? (Unpin_t*)new UnpinAarch64_t(zipr_object) :
		mt==admtArm32   ? (Unpin_t*)new UnpinArm32_t  (zipr_object) :
		mt==admtMips32  ? (Unpin_t*)new UnpinMips32_t (zipr_object) :
		throw invalid_argument("Cannot determine machine type");
}
