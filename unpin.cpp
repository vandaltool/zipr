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

using namespace libIRDB;
using namespace std;
using namespace Zipr_SDK;
using namespace ELFIO;

void Unpin_t::DoUnpin()
{
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
					cout<<"Unpin::Found data_to_insn_ptr relocation for pinned insn at "<<hex<<
						insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;
				}
				else
				{
					cout<<"Unpin::Warn: unpin found non-IBTA to unpin.  probably it's unpinned twice.  continuing anyhow."<<endl;
				}
	
				int found=false;
				for(
					RelocationSet_t::iterator rit2=insn->GetRelocations().begin(); 
					rit2!=insn->GetRelocations().end();
					rit2++
				   )
				{

					/* check for a nonce relocation */
					if ( (*rit2) -> GetType().find("cfi_nonce") != string::npos )
					{
						found=true;
					}
				}

				/* don't unpin if we found one */
				if(found)
				{
					cout<<"Unpin::Not unpinning because CFI is requesting a nonce."<<endl;
				}
				else
				{
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
}

void Unpin_t::UpdateScoops()
{
	for(
		DataScoopSet_t::iterator it=zo->GetFileIR()->GetDataScoops().begin();
		it!=zo->GetFileIR()->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;
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

				cout<<"Unpin::Unpinned data_to_insn_ptr insn moved to "<<hex<<newLoc<<endl;

				int found=false;
				for(
					RelocationSet_t::iterator rit2=insn->GetRelocations().begin(); 
					rit2!=insn->GetRelocations().end();
					rit2++
				   )
				{

					/* check for a nonce relocation */
					if ( (*rit2) -> GetType().find("cfi_nonce") != string::npos )
					{
						found=true;
					}
				}

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
