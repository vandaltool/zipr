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


static bool arg_has_memory(const ARGTYPE &arg)
{
        /* if it's relative memory, watch out! */
        if(arg.ArgType&MEMORY_TYPE)
                return true;

        return false;
}

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
	for(
		RelocationSet_t::iterator rit2=insn->GetRelocations().begin(); 
		rit2!=insn->GetRelocations().end();
		rit2++
	   )
	{

		/* check for a nonce relocation */
		if ( (*rit2) -> GetType().find("cfi_nonce") != string::npos )
		{
			return true;
		}
	}
	return false;
}

static bool should_cfi_pin(Instruction_t* insn)
{
	// add command line option that:
	// 	1) return false if !has_cfi_reloc(insn)
	// 	2) return true if option is on.
	return false;
}


void Unpin_t::DoUnpin()
{
	DoUnpinForScoops();
	DoUnpinForFixedCalls();
}


// scan instructions and process instruction relocs that can be unpinned.
void Unpin_t::DoUnpinForFixedCalls()
{
	int unpins=0;
	int missed_unpins=0;

	for(
		InstructionSet_t::iterator it=zo->GetFileIR()->GetInstructions().begin();
		it!=zo->GetFileIR()->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* from_insn=*it;

		for(
			RelocationSet_t::iterator rit=from_insn->GetRelocations().begin(); 
			rit!=from_insn->GetRelocations().end();
			rit++
		   )
		{
			Relocation_t* reloc=*rit;

			
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->GetType()==string("32-bit") || reloc->GetType()==string("push64"))
			{
				// skip if there's no WRT, that means it's unpinned for something besides a fixed call.
				if(reloc->GetWRT()==NULL)
					continue;

				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				Instruction_t* wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				assert(wrt_insn);
				if(should_cfi_pin(wrt_insn))
					continue;


				if(wrt_insn->GetIndirectBranchTargetAddress())
				{
					cout<<"Unpin::Found "<<reloc->GetType()<<" relocation for pinned insn at "<<hex<<
						wrt_insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;
				}
				else
				{
					cout<<"Unpin::Warn: unpin found non-IBTA to unpin.  probably it's unpinned twice.  continuing anyhow."<<endl;
				}
	
				unpins++;
				wrt_insn->SetIndirectBranchTargetAddress(NULL);

				PlacementQueue_t* pq=zo->GetPlacementQueue();
				assert(pq);

				// create a new dollop for the unpinned IBT
				// and add it to the placement queue.
				Dollop_t *newDoll=zo->GetDollopManager()->AddNewDollops(wrt_insn);
				pq->insert(std::pair<Dollop_t*,RangeAddress_t>(newDoll, 0));
			}
		}
	}

	cout<<"#ATTRIBUTE insn_unpin_total_unpins="<<dec<<unpins<<endl;
	cout<<"#ATTRIBUTE insn_unpin_missed_unpins="<<dec<<missed_unpins<<endl;
}


void Unpin_t::DoUnpinForScoops()
{
	int unpins=0;
	int missed_unpins=0;

	for(
		DataScoopSet_t::iterator it=zo->GetFileIR()->GetDataScoops().begin();
		it!=zo->GetFileIR()->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;

		for(
			RelocationSet_t::iterator rit=scoop->GetRelocations().begin(); 
			rit!=scoop->GetRelocations().end();
			rit++
		   )
		{
			Relocation_t* reloc=*rit;

			
			if(reloc->GetType()==string("data_to_insn_ptr"))
			{
				Instruction_t* insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				assert(insn);


				if(insn->GetIndirectBranchTargetAddress())
				{
					cout<<"Unpin::Found data_to_insn_ptr relocation for pinned insn:"
					    <<hex<<insn->GetBaseID()<<":" <<insn->getDisassembly()<<" at "
					    <<hex<< insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;
				}
				else
				{
					cout<<"Unpin::Warn: unpin found non-IBTA to unpin for insn:"
					    <<hex<<insn->GetBaseID()<<":" <<insn->getDisassembly()
					    <<".  probably it's unpinned twice.  continuing anyhow."<<endl;
				}
	
				bool found=should_cfi_pin(insn);

				/* don't unpin if we found one */
				if(found)
				{
					cout<<"Unpin::Not unpinning because CFI is requesting a nonce."<<endl;
					missed_unpins++;
				}
				else
				{
					unpins++;
					insn->SetIndirectBranchTargetAddress(NULL);

					PlacementQueue_t* pq=zo->GetPlacementQueue();
					assert(pq);

					// create a new dollop for the unpinned IBT
					// and add it to the placement queue.
					Dollop_t *newDoll=zo->GetDollopManager()->AddNewDollops(insn);
					pq->insert(std::pair<Dollop_t*,RangeAddress_t>(newDoll, 0));
				}
			}
		}
	}

	cout<<"#ATTRIBUTE scoop_unpin_total_unpins="<<dec<<unpins<<endl;
	cout<<"#ATTRIBUTE scoop_unpin_missed_unpins="<<dec<<missed_unpins<<endl;
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
	MemorySpace_t &ms=*zo->GetMemorySpace();
	Zipr_SDK::InstructionLocationMap_t &locMap=*(zo->GetLocationMap());

	for(
		InstructionSet_t::iterator it=zo->GetFileIR()->GetInstructions().begin();
		it!=zo->GetFileIR()->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* from_insn=*it;
                DISASM disasm;
                from_insn->Disassemble(disasm);

                // find memory arg.
                ARGTYPE* the_arg=NULL;
                if(arg_has_memory(disasm.Argument1))
                        the_arg=&disasm.Argument1;
                if(arg_has_memory(disasm.Argument2))
                        the_arg=&disasm.Argument2;
                if(arg_has_memory(disasm.Argument3))
                        the_arg=&disasm.Argument3;
                if(arg_has_memory(disasm.Argument4))
                        the_arg=&disasm.Argument4;

		for(
			RelocationSet_t::iterator rit=from_insn->GetRelocations().begin(); 
			rit!=from_insn->GetRelocations().end();
			rit++
		   )
		{
			Relocation_t* reloc=*rit;

			
			// this probably won't work on shared objects.
			// complicated with the push64-reloc plugin also rewriting these things?
			if(reloc->GetType()==string("32-bit") || reloc->GetType()==string("push64"))
			{
				// skip if there's no WRT, that means it's unpinned for something besides a fixed call.
				if(reloc->GetWRT()==NULL)
					continue;

				// getWRT returns an BaseObj, but this reloc type expects an instruction
				// safe cast and check.
				Instruction_t* wrt_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				assert(wrt_insn);
				if(should_cfi_pin(wrt_insn)) 
					continue;

				libIRDB::virtual_offset_t wrt_insn_location=locMap[wrt_insn];
				libIRDB::virtual_offset_t from_insn_location=locMap[from_insn];

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

					for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
					{ 
						unsigned char newbyte=newpush[i];
						ms[from_insn_location+i]=newbyte;
					}
				}
				// shared object
				// gets a call/sub [$rsp], const pair.
                        	else 
				{
// handled in push64_relocs which is required for shared objects.
#if 0
					auto call_insn=from_insn;
					auto sub_insn=call_insn->GetTarget();
					libIRDB::virtual_offset_t sub_insn_location=locMap[sub_insn];

					assert(sub_insn);
					auto subbits=sub_insn->GetDataBits();

					// must be sub [$rsp], const
					assert( subbits[0]==(char)0x48 && subbits[1]==(char)0x81 && 
					        subbits[2]==(char)0x2c && subbits[3]==(char)0x24); 
					unsigned char newoffset[sizeof(int)]={};
					// grab old offset from memory space
					for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
						newoffset[i]=ms[sub_insn_location+4+i];

					// update it.
					*(unsigned int*)newoffset += wrt_insn_location;

					// write memory space.
					for(unsigned int i=0;i<sizeof(int); i++)
						ms[sub_insn_location+4+i]=newoffset[i];

					cout<<"Unpin::Updating push64-so insn:"
						<<dec<<from_insn->GetBaseID()<<"@"<<hex<<from_insn_location<<" to point at "
						<<dec<<wrt_insn ->GetBaseID()<<"@"<<hex<<wrt_insn_location <<endl;

	
#endif
				}

			}
			// instruction has a pcrel memory operand.
			else if(reloc->GetType()==string("pcrel") && reloc->GetWRT()!=NULL)
			{

				BaseObj_t* bo_wrt=reloc->GetWRT();
				DataScoop_t* scoop_wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
				Instruction_t* insn_wrt=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				virtual_offset_t rel_addr1=the_arg->Memory.Displacement;
				rel_addr1+=from_insn->GetDataBits().size();

				int disp_offset=the_arg->Memory.DisplacementAddr-disasm.EIP;
				int disp_size=the_arg->Memory.DisplacementSize;
				libIRDB::virtual_offset_t from_insn_location=locMap[from_insn];
				assert(disp_size==4);
				assert(0<disp_offset && disp_offset<=from_insn->GetDataBits().size() - disp_size);

				libIRDB::virtual_offset_t to_addr=0xdeadbeef; // noteable value that shouldn't be used.
				string convert_string;

				assert(bo_wrt);
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
				else assert(0);
					
				int new_disp=rel_addr1 + to_addr - from_insn->GetDataBits().size()-from_insn_location;
	
				from_insn->SetDataBits(from_insn->GetDataBits().replace(disp_offset, 
					disp_size, (char*)&new_disp, disp_size));
				// update the instruction in the memory space.
				for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
				{ 
					unsigned char newbyte=from_insn->GetDataBits()[i];
					ms[from_insn_location+i]=newbyte;
				}
				DISASM disasm2;
				from_insn->Disassemble(disasm2);	
				cout<<"unpin:pcrel:new_disp="<<hex<<new_disp<<endl;
				cout<<"unpin:pcrel:new_insn_addr="<<hex<<from_insn_location<<endl;
				cout<<"unpin:pcrel:Converting "<<hex<<from_insn->GetBaseID()<<":"<<disasm.CompleteInstr
					<<" to "<<disasm2.CompleteInstr<<" wrt "<< convert_string <<endl;
			}
			// instruction has a absolute  memory operand that needs it's displacement updated.
			else if(reloc->GetType()==string("absoluteptr_to_scoop"))
			{

				DataScoop_t* wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
				assert(wrt);
				virtual_offset_t rel_addr1=the_arg->Memory.Displacement;

				int disp_offset=the_arg->Memory.DisplacementAddr-disasm.EIP;
				int disp_size=the_arg->Memory.DisplacementSize;
				assert(disp_size==4);
				assert(0<disp_offset && disp_offset<=from_insn->GetDataBits().size() - disp_size);
				assert(reloc->GetWRT());

                                unsigned int new_disp=the_arg->Memory.Displacement + wrt->GetStart()->GetVirtualOffset();
                                from_insn->SetDataBits(from_insn->GetDataBits().replace(disp_offset, disp_size, (char*)&new_disp, disp_size));
				// update the instruction in the memory space.
				libIRDB::virtual_offset_t from_insn_location=locMap[from_insn];
				for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
				{ 
					unsigned char newbyte=from_insn->GetDataBits()[i];
					ms[from_insn_location+i]=newbyte;

					//cout<<"Updating push["<<i<<"] from "<<hex<<oldbyte<<" to "<<newbyte<<endl;
				}
                		DISASM disasm2;
                		from_insn->Disassemble(disasm2);
				cout<<"unpin:absptr_to_scoop:Converting "<<hex<<from_insn->GetBaseID()<<":"<<disasm.CompleteInstr
			 	    <<" to "<<disasm2.CompleteInstr<<" for scoop: "<<wrt->GetName()<<endl;
			}
			// instruction has an immediate that needs an update.
			else if(reloc->GetType()==string("immedptr_to_scoop"))
			{
				DataScoop_t* wrt=dynamic_cast<DataScoop_t*>(reloc->GetWRT());
				assert(wrt);

        			virtual_offset_t rel_addr2=disasm.Instruction.Immediat;
				virtual_offset_t new_addr = rel_addr2 + wrt->GetStart()->GetVirtualOffset();

                                from_insn->SetDataBits(from_insn->GetDataBits().replace(from_insn->GetDataBits().size()-4, 4, (char*)&new_addr, 4));

				libIRDB::virtual_offset_t from_insn_location=locMap[from_insn];
				for(unsigned int i=0;i<from_insn->GetDataBits().size();i++)
				{ 
					unsigned char newbyte=from_insn->GetDataBits()[i];
					ms[from_insn_location+i]=newbyte;

					//cout<<"Updating push["<<i<<"] from "<<hex<<oldbyte<<" to "<<newbyte<<endl;
				}

                		DISASM disasm2;
                		from_insn->Disassemble(disasm2);
				cout<<"unpin:immedptr_to_scoop:Converting "<<hex<<from_insn->GetBaseID()<<":"<<disasm.CompleteInstr
			 	    <<" to "<<disasm2.CompleteInstr<<" for scoop: "<<wrt->GetName()<<endl;


			}

		}
	}

}

void Unpin_t::DoUpdateForScoops()
{
	unsigned int byte_width=zo->GetFileIR()->GetArchitectureBitWidth()/8;
	for(
		DataScoopSet_t::iterator it=zo->GetFileIR()->GetDataScoops().begin();
		it!=zo->GetFileIR()->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;
		assert(scoop->GetEnd()->GetVirtualOffset() - scoop->GetStart()->GetVirtualOffset()+1 == scoop->GetSize());
		assert(scoop->GetContents().size() == scoop->GetSize());
		string scoop_contents=scoop->GetContents();

		for(
			RelocationSet_t::iterator rit=scoop->GetRelocations().begin(); 
			rit!=scoop->GetRelocations().end();
			rit++
		   )
		{
			Relocation_t* reloc=*rit;

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
