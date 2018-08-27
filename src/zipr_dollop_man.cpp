#include <zipr_all.h>
#include <iostream>
#include <cstdlib>

using namespace zipr;
using namespace zipr::Utils;
using namespace std;
using namespace Zipr_SDK;
using namespace libIRDB;

namespace zipr {

	Dollop_t *ZiprDollopManager_t::AddNewDollops(Instruction_t *start) {
		Dollop_t *new_dollop = NULL;
		Dollop_t *existing_dollop = GetContainingDollop(start);

		/*
		 * This is the target dollop *only*
		 * if the target instruction is the first instruction.
		 */
		if (existing_dollop)
		{
			/*
			 * There is a target dollop. But, do we need to split it?
			 */
			if (existing_dollop->GetDollopEntryCount() &&
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
			Dollop_t *original_new_dollop = NULL, *previous_dollop = NULL;
			Instruction_t *fallthrough = NULL;
			original_new_dollop = new_dollop = Dollop_t::CreateNewDollop(start,this);

			for (it = new_dollop->begin(), it_end = new_dollop->end();
			     it != it_end;
					 it++)
			{
				Dollop_t *containing_dollop = GetContainingDollop((*it)->Instruction());
				if (containing_dollop) 
				{
					Dollop_t *fallthrough_dollop = NULL;
					if (true)
						cout << "Found an instruction in a new dollop that "
						     << "is already in a dollop: " << std::hex
								 << ((NULL!=(*it)->Instruction()->GetAddress()) ?
								    (*it)->Instruction()->GetAddress()->GetVirtualOffset():0x0)
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
			                                ->GetFallthrough()) != NULL)
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
				Dollop_t *existing_dollop = GetContainingDollop(fallthrough);
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

	size_t ZiprDollopManager_t::DetermineWorstCaseDollopEntrySize(DollopEntry_t *entry) {
		if (m_zipr != NULL)
			return m_zipr->DetermineWorstCaseDollopEntrySize(entry, false);
		else
			return Utils::DetermineWorstCaseInsnSize(entry->Instruction(), false);
	}

	void ZiprDollopManager_t::PrintDollopPatches(const ostream &out) {
		std::list<DollopPatch_t*>::const_iterator patch_it, patch_it_end;

		for (patch_it = m_patches.begin(), patch_it_end = m_patches.end();
		     patch_it != patch_it_end;
				 patch_it++) {
			cout << *(*patch_it) << endl;
		}
	}

	Dollop_t *ZiprDollopManager_t::GetContainingDollop(libIRDB::Instruction_t *insn) {
#if 0
		try {
			return m_insn_to_dollop.at(insn);
		} catch (const std::out_of_range &oor) {
			return NULL;
		}
		return NULL;
#else
		InsnToDollopMap_t::iterator it=m_insn_to_dollop.find(insn);
		if(it!=m_insn_to_dollop.end())
			return it->second;
		return NULL;
			
#endif
	}

	void ZiprDollopManager_t::AddDollops(Dollop_t *dollop_head) {
		Dollop_t *dollop = dollop_head;
		while (dollop != NULL)
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
#if 0		
		if (m_dollops.end()==std::find(m_dollops.begin(), m_dollops.end(), dollop))
			m_dollops.push_back(dollop);
#else
			m_dollops.insert(dollop);

#endif
	
		m_refresh_stats = true;
	}

	bool ZiprDollopManager_t::UpdateTargets(Dollop_t *dollop) {
		bool changed = false;
		bool local_changed = false;
		int local_changed_count=0;
		int and_count=0;
		do {
			local_changed = false;
			local_changed_count++;
			const auto local_dollop=list<DollopEntry_t*>(dollop->begin(), dollop->end());
			list<DollopEntry_t*>::const_iterator it, it_end;
			for (it = local_dollop.begin(), it_end = local_dollop.end();
			     it != it_end;
					 /* nop */) {
				DollopEntry_t *entry = *it;
				it++;
				if (entry->Instruction() &&
				    entry->Instruction()->GetTarget()) {
					Dollop_t *new_target=AddNewDollops(entry->Instruction()->GetTarget());
					and_count++;

					/*
					 * In the case there is a change, we have to restart.
					 * The dollop that we are updating could itself have
					 * contained the target and the call would have
					 * split this dollop. That makes the iterator go
					 * haywire.
					 * 
					 * But!  We could avoid the break by using a copy of the set.
					 */
					if (new_target != entry->TargetDollop()) {
						entry->TargetDollop(new_target);
						changed = local_changed = true;
						//break;
					}
				}
			}
			
		} while (false); // while (local_changed);
		return changed;
	}

	void ZiprDollopManager_t::UpdateAllTargets(void) {
		DollopList_t::iterator it, it_end;
		bool changed = false;
		int changed_count=0;
		int update_count=0;
		do {
			changed = false;
			const auto local_dollops=m_dollops;
			for (it = local_dollops.begin(), it_end = local_dollops.end(); it != it_end; /* nop */) 
			{
				Dollop_t *entry = *it;
				it++;
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
			m_total_dollop_entries += dollop->GetDollopEntryCount();
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
		RangeSet_t original_ranges = memory_space.GetOriginalFreeRanges();
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
			Dollop_t *dollop_to_print = NULL;

			for (dollop_it = m_dollops.begin(), dollop_it_end = m_dollops.end();
			     dollop_it != dollop_it_end;
					 dollop_it++)
			{
				Dollop_t *dollop = (*dollop_it);
				if (current_range.GetStart() <= dollop->Place() &&
				    current_range.GetEnd() >= dollop->Place())
					dollops_in_range[dollop->Place()] = dollop;
			}
			
			map_output << "==========" << endl;
			map_output << "Range: 0x" << std::hex << current_range.GetStart()
			           << " - 0x" << std::hex << current_range.GetEnd() 
								 << endl;
			
			previous_dollop_end = current_range.GetStart();
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
							PRINT_LINE_HEADER((current_range.GetStart()+byte_print_counter))
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
				for (unsigned i=0;i<(dollop_to_print->GetSize());i++)
				{
					if (!((byte_print_counter) % LINE_LENGTH))
						PRINT_LINE_HEADER((current_range.GetStart()+byte_print_counter))
					map_output << "X";
					byte_print_counter++;
				}
#if 0
				map_output << "0x" << std::hex << dollop_to_print->Place()
				           << " - 0x" << std::hex
									 <<(dollop_to_print->Place()+dollop_to_print->GetSize())
									 << ": (" << std::dec << dollop_to_print->GetSize()
									 << ") "
									 << endl;

#endif
				previous_dollop_end = dollop_to_print->Place() + 
				                      dollop_to_print->GetSize();
			}

			if (dollop_to_print && current_range.GetEnd() != (RangeAddress_t)-1 &&
			   (previous_dollop_end < current_range.GetEnd())) 
			{
				for (unsigned i=0;i<(current_range.GetEnd() - previous_dollop_end);i++)
				{
					if (!((byte_print_counter) % LINE_LENGTH))
						PRINT_LINE_HEADER((current_range.GetStart()+byte_print_counter))
					map_output << "_";
					byte_print_counter++;
				}
#if 0
				map_output << "0x" << std::hex << dollop_to_print->Place()
				           << " - 0x" << std::hex
				           <<(dollop_to_print->Place()+dollop_to_print->GetSize())
				           << ": (" << std::dec 
				           << (current_range.GetEnd() - previous_dollop_end)
				           << ") EMPTY" << endl;
#endif
			}
			map_output << endl;
		}
		map_output.close();
	}
}
