#include <zipr_all.h>
#include <iostream>
#include <cstdlib>

using namespace zipr;
using namespace std;
using namespace Zipr_SDK;
using namespace IRDB_SDK;

namespace zipr {

	Dollop_t *ZiprDollopManager_t::AddNewDollops(Instruction_t *start) {
		Dollop_t *new_dollop = nullptr;
		Dollop_t *existing_dollop = getContainingDollop(start);

		/*
		 * This is the target dollop *only*
		 * if the target instruction is the first instruction.
		 */
		if (existing_dollop)
		{
			/*
			 * There is a target dollop. But, do we need to split it?
			 */
			if (existing_dollop->getDollopEntryCount() &&
			    existing_dollop->front()->Instruction() == start) {
				/*
				 * Just return the existing dollop.
				 */
				return existing_dollop;
			}
			else {
				/*
				 * Split at this dollop to make a new one!
				 */
				AddDollops(new_dollop = existing_dollop->Split(start));
				return new_dollop;
			}
		}
		else {
			/*
			 * There is no target dollop. Let's create one!
			 */
			std::list<DollopEntry_t*>::iterator it, it_end;
			Dollop_t *original_new_dollop = nullptr, *previous_dollop = nullptr;
			Instruction_t *fallthrough = nullptr;
			original_new_dollop = new_dollop = Dollop_t::CreateNewDollop(start,this);

			for (it = new_dollop->begin(), it_end = new_dollop->end();
			     it != it_end;
					 it++)
			{
				Dollop_t *containing_dollop = getContainingDollop((*it)->Instruction());
				if (containing_dollop) 
				{
					Dollop_t *fallthrough_dollop = nullptr;
					if (true)
						cout << "Found an instruction in a new dollop that "
						     << "is already in a dollop: " << std::hex
								 << ((nullptr!=(*it)->Instruction()->getAddress()) ?
								    (*it)->Instruction()->getAddress()->getVirtualOffset():0x0)
								 << endl;
					/*
					 * Reliably get a pointer to the containing dollop.
					 */
					fallthrough_dollop = AddNewDollops((*it)->Instruction());
					
					/*
					 * Link this dollop to that one. Do this before
					 * removing the entries because RemoveDollopEntries()
					 * will recalculate the size and needs to know about
					 * the updated fallthrough dollop!
					 */
					new_dollop->FallthroughDollop(fallthrough_dollop);
					fallthrough_dollop->FallbackDollop(new_dollop);

					/*
					 * Delete the overlapping instructions.
					 */
					new_dollop->RemoveDollopEntries(it, it_end);

					/*
					 * Put the new dollop in!
					 */
					AddDollop(new_dollop);

					return new_dollop;
				}
			}
			/*
			 * This is to handle the case where
			 * we stopped creating a dollop because
			 * the next instruction is pinned. We do
			 * not want to forget about the remaining
			 * entries here. So, we attempt to link
			 * to those, where possible.
			 */
			while ((fallthrough = new_dollop->back()
			                                ->Instruction()
			                                ->getFallthrough()) != nullptr)
			{
				/*
				 * Look FIRST for a containing dollop.
				 *
				 * TODO: We *assert* that we do not have
				 * to check whether or not the fallthrough
				 * instruction is at the top of the stack.
				 * This is because we are only at this case
				 * when the dollop construction ended because
				 * the fallthrough is pinned. This implicitly
				 * means that it is the first instruction
				 * in the containing dollop.
				 */
				Dollop_t *existing_dollop = getContainingDollop(fallthrough);
				if (existing_dollop)
				{
					assert(existing_dollop->front()->Instruction() == fallthrough);
					new_dollop->FallthroughDollop(existing_dollop);
					existing_dollop->FallbackDollop(new_dollop);
					break;
				}
				/*
				 * Otherwise, create a new dollop from the fallthrough
				 * and link them together.
				 */
				previous_dollop = new_dollop;

				// cannot do this:
				// new_dollop = Dollop_t::CreateNewDollop(fallthrough, this);
				// because CreateNewDollop does not adaquately trim the dollop
				// and it might result in an instruction being in two dollops
				// Using AddNewDollops instead.
				new_dollop = this->AddNewDollops(fallthrough);
				previous_dollop->FallthroughDollop(new_dollop);
				new_dollop->FallbackDollop(previous_dollop);
			}
			AddDollops(original_new_dollop);
			return original_new_dollop;
		}
	}

	size_t ZiprDollopManager_t::DetermineDollopEntrySize(DollopEntry_t *entry) 
	{
		const auto l_zipr=dynamic_cast<ZiprImpl_t*>(m_zipr);
		const auto sizer=l_zipr->getSizer();
		if (m_zipr != nullptr)
			return m_zipr->DetermineDollopEntrySize(entry, false);
		else
			return sizer->DetermineInsnSize(entry->Instruction(), false);
	}

	void ZiprDollopManager_t::PrintDollopPatches(const ostream &out) {
		std::list<DollopPatch_t*>::const_iterator patch_it, patch_it_end;

		for (patch_it = m_patches.begin(), patch_it_end = m_patches.end();
		     patch_it != patch_it_end;
				 patch_it++) {
			cout << *(*patch_it) << endl;
		}
	}

	Dollop_t *ZiprDollopManager_t::getContainingDollop(IRDB_SDK::Instruction_t *insn) {
		InsnToDollopMap_t::iterator it=m_insn_to_dollop.find(insn);
		if(it!=m_insn_to_dollop.end())
			return it->second;
		return nullptr;
			
	}

	void ZiprDollopManager_t::AddDollops(Dollop_t *dollop_head) {
		Dollop_t *dollop = dollop_head;
		while (dollop != nullptr)
		{
			AddDollop(dollop);
			dollop = dollop->FallthroughDollop();
		}
		m_refresh_stats = true;
	}




	/* TODO: Write a test case for the new conditional push_back. Make
	 * sure to test whether or not the instruction-to-dollop map
	 * is properly updated in all cases.
	 */
	void ZiprDollopManager_t::AddDollop(Dollop_t *dollop) {
		/*
		 * We always want to update the isntruction-to-dollop map.
		 * However, we might not always want to push it on to the
		 * list of dollops -- it might already be there!
		 */
		/*
		 * Populate/update the instruction-to-dollop map.
		 */
		std::list<DollopEntry_t*>::iterator it, it_end;
		for (it = dollop->begin(), it_end = dollop->end();
		     it != it_end;
				 it++) {
			m_insn_to_dollop[(*it)->Instruction()] = dollop;
		}
		/*
		 * Push the actual dollop onto the list of dollops
		 * if it's not already there.
		 */
		m_dollops.insert(dollop);
		m_refresh_stats = true;
	}

	bool ZiprDollopManager_t::UpdateTargets(Dollop_t *dollop) {
		auto changed = false;
		const auto local_dollop=list<DollopEntry_t*>(dollop->begin(), dollop->end());
		for (auto &entry : local_dollop )
		{
			auto insn=entry->Instruction();
			if (insn->getTarget()) 
			{
				auto new_target=AddNewDollops(insn->getTarget());

				/*
				 * In the case there is a change, we have to restart.
				 * The dollop that we are updating could itself have
				 * contained the target and the call would have
				 * split this dollop. That makes the iterator go
				 * haywire.
				 * 
				 * But!  We could avoid the break by using a copy of the set, which we do.
				 */
				if (new_target != entry->TargetDollop()) {
					entry->TargetDollop(new_target);
					changed = true;
				}
			}
		}
			
		return changed;
	}

	void ZiprDollopManager_t::UpdateAllTargets(void) {
		// Used to make sure dollops are created for instructions referenced by
                // relocations. 
                const auto handle_reloc=[this](const Relocation_t* reloc)
                {
                        auto wrt_insn=dynamic_cast<Instruction_t*>(reloc->getWRT());
                        if(wrt_insn)
                        {
                                // we don't bother marking a change because
                                // we only need to do this once for relocs
                                // and we are certain to get here once for every dollop
                                AddNewDollops(wrt_insn);
                                cout<<"Adding new dollop for reloc of type="<<reloc->getType()<<endl;

                        }
                };
	
		// Make sure dollops are created for instructions referenced by
                // relocations.
		for(auto &reloc : m_zipr->getFileIR()->getRelocations())
                	handle_reloc(reloc);


		auto changed = false;
		auto changed_count=0;
		auto update_count=0;
		do {
			changed = false;
			const auto local_dollops=m_dollops;
			for (auto entry : local_dollops)
			{
				changed |= UpdateTargets(entry);
				update_count++;
				if((update_count%1000000) == 0 )
					cout<<"number of dollops="<<dec<<m_dollops.size()<<".  "<<dec<<update_count<<" iterations attempted."<<endl;
			}
			changed_count++;
		} while (changed);
		cout<<"All Targets updated.  changed_count="<<dec<<changed_count<<". Update_count="<<update_count<<"."<<endl;
	}

	std::ostream &operator<<(std::ostream &out, const ZiprDollopManager_t &dollop_man) {
		DollopList_t::iterator it, it_end;

		for (it = dollop_man.m_dollops.begin(), it_end = dollop_man.m_dollops.end();
		     it != it_end;
				 it++) {
			Dollop_t *entry = *it;
			out << std::hex << entry << std::endl;
			out << *entry << std::endl;
		}
		return out;
	}

	void ZiprDollopManager_t::CalculateStats()
	{
		m_truncated_dollops = 0;
		m_total_dollop_entries = 0;
		m_total_dollops = Size();

		DollopList_t::iterator dollop_it, dollop_it_end;
		for (dollop_it = m_dollops.begin(), dollop_it_end = m_dollops.end();
		     dollop_it != m_dollops.end();
				 dollop_it++)
		{
			Dollop_t *dollop = *dollop_it;
			m_total_dollop_entries += dollop->getDollopEntryCount();
			if (dollop->WasTruncated())
				m_truncated_dollops++;
		}
		m_refresh_stats = false;
	}

	void ZiprDollopManager_t::PrintStats(std::ostream &out)
	{
		if (m_refresh_stats)
			CalculateStats();

		PrintStat(out, "Total dollops", m_total_dollops);
		//PrintStat(out, "Total dollop size", total_dollop_space);
		PrintStat(out, "Total dollop entries", m_total_dollop_entries);
		PrintStat(out, "Truncated dollops", m_truncated_dollops);
		PrintStat(out, "Avg dollop entries per dollop",
			(double)m_total_dollop_entries/(double)m_total_dollops);
		PrintStat(out, "Truncated dollop fraction",
			(double)m_truncated_dollops/(double)m_total_dollops);
	}

#define LINE_LENGTH 32
#define PRINT_LINE_HEADER(x) \
	map_output << endl << std::hex << (x) << ": ";

	void ZiprDollopManager_t::PrintPlacementMap(
		const MemorySpace_t &_memory_space,
		const std::string &map_filename)
	{
		const ZiprMemorySpace_t &memory_space = static_cast<const ZiprMemorySpace_t &>(_memory_space);
		RangeSet_t original_ranges = memory_space.getOriginalFreeRanges();
		RangeSet_t::const_iterator range_it, range_it_end;
		ofstream map_output(map_filename.c_str(), std::ofstream::out);

		if (!map_output.is_open())
			return;
		/*
		 * Loop through the original ranges.
		 */
		for (range_it=original_ranges.begin(), range_it_end=original_ranges.end();
		     range_it != range_it_end;
				 range_it++)
		{
			/*
			 * Now loop through the dollops and
			 * record those contained in this range.
			 */
			DollopList_t::iterator dollop_it, dollop_it_end;
			Range_t current_range = *range_it;
			map<RangeAddress_t, Dollop_t*> dollops_in_range;
			map<RangeAddress_t, Dollop_t*>::const_iterator dollops_in_range_it,
			                                               dollops_in_range_end;
			RangeAddress_t previous_dollop_end = 0;
			Dollop_t *dollop_to_print = nullptr;

			for (dollop_it = m_dollops.begin(), dollop_it_end = m_dollops.end();
			     dollop_it != dollop_it_end;
					 dollop_it++)
			{
				Dollop_t *dollop = (*dollop_it);
				if (current_range.getStart() <= dollop->Place() &&
				    current_range.getEnd() >= dollop->Place())
					dollops_in_range[dollop->Place()] = dollop;
			}
			
			map_output << "==========" << endl;
			map_output << "Range: 0x" << std::hex << current_range.getStart()
			           << " - 0x" << std::hex << current_range.getEnd() 
								 << endl;
			
			previous_dollop_end = current_range.getStart();
			unsigned byte_print_counter = 0;
			for (dollops_in_range_it = dollops_in_range.begin(),
			     dollops_in_range_end = dollops_in_range.end();
			     dollops_in_range_it != dollops_in_range_end;
					 dollops_in_range_it++)
			{
				dollop_to_print = (*dollops_in_range_it).second;
				if (previous_dollop_end < dollop_to_print->Place())
				{
					for (unsigned i=0;i<(dollop_to_print->Place()-previous_dollop_end);i++)
					{
						if (!((byte_print_counter) % LINE_LENGTH)) 
							PRINT_LINE_HEADER((current_range.getStart()+byte_print_counter))
						map_output << "_";
						byte_print_counter++;
					}
#if 0
					map_output << "0x" << std::hex << previous_dollop_end
					           << " - 0x" <<std::hex <<(dollop_to_print->Place())
					           << ": (" << std::dec 
					           << (dollop_to_print->Place() - previous_dollop_end)
					           << ") EMPTY" << endl;
#endif
				}
				for (unsigned i=0;i<(dollop_to_print->getSize());i++)
				{
					if (!((byte_print_counter) % LINE_LENGTH))
						PRINT_LINE_HEADER((current_range.getStart()+byte_print_counter))
					map_output << "X";
					byte_print_counter++;
				}
#if 0
				map_output << "0x" << std::hex << dollop_to_print->Place()
				           << " - 0x" << std::hex
									 <<(dollop_to_print->Place()+dollop_to_print->getSize())
									 << ": (" << std::dec << dollop_to_print->getSize()
									 << ") "
									 << endl;

#endif
				previous_dollop_end = dollop_to_print->Place() + 
				                      dollop_to_print->getSize();
			}

			if (dollop_to_print && current_range.getEnd() != (RangeAddress_t)-1 &&
			   (previous_dollop_end < current_range.getEnd())) 
			{
				for (unsigned i=0;i<(current_range.getEnd() - previous_dollop_end);i++)
				{
					if (!((byte_print_counter) % LINE_LENGTH))
						PRINT_LINE_HEADER((current_range.getStart()+byte_print_counter))
					map_output << "_";
					byte_print_counter++;
				}
#if 0
				map_output << "0x" << std::hex << dollop_to_print->Place()
				           << " - 0x" << std::hex
				           <<(dollop_to_print->Place()+dollop_to_print->getSize())
				           << ": (" << std::dec 
				           << (current_range.getEnd() - previous_dollop_end)
				           << ") EMPTY" << endl;
#endif
			}
			map_output << endl;
		}
		map_output.close();
	}
}
