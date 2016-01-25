#include <zipr_all.h>
#include <iostream>

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
			original_new_dollop = new_dollop = Dollop_t::CreateNewDollop(start);

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
					 * Delete the overlapping instructions.
					 */
					new_dollop->erase(it, it_end);

					/*
					 * Link this dollop to that one.
					 */
					new_dollop->FallthroughDollop(fallthrough_dollop);

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
			while (fallthrough = new_dollop->back()->Instruction()->GetFallthrough())
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
					break;
				}
				/*
				 * Otherwise, create a new dollop from the fallthrough
				 * and link them together.
				 */
				previous_dollop = new_dollop;
				new_dollop = Dollop_t::CreateNewDollop(fallthrough);
				previous_dollop->FallthroughDollop(new_dollop);
			}
			AddDollops(original_new_dollop);
			return original_new_dollop;
		}
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
		try {
			return m_insn_to_dollop.at(insn);
		} catch (const std::out_of_range &oor) {
			return NULL;
		}
		return NULL;
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
		list<DollopEntry_t*>::const_iterator it, it_end;
		for (it = dollop->begin(), it_end = dollop->end();
		     it != it_end;
				 it++) {
			m_insn_to_dollop[(*it)->Instruction()] = dollop;
		}
		/*
		 * Push the actual dollop onto the list of dollops
		 * if it's not already there.
		 */
		if (m_dollops.end()==std::find(m_dollops.begin(), m_dollops.end(), dollop))
			m_dollops.push_back(dollop);
		m_refresh_stats = true;
	}

	bool ZiprDollopManager_t::UpdateTargets(Dollop_t *dollop) {
		list<DollopEntry_t*>::iterator it, it_end;
		bool changed = false;
		bool local_changed = false;
		do {
			local_changed = false;
			for (it = dollop->begin(), it_end = dollop->end();
			     it != it_end;
					 /* nop */) {
				DollopEntry_t *entry = *it;
				it++;
				if (entry->Instruction() &&
				    entry->Instruction()->GetTarget()) {
					Dollop_t *new_target=AddNewDollops(entry->Instruction()->GetTarget());

					/*
					 * In the case there is a change, we have to restart.
					 * The dollop that we are updating could itself have
					 * contained the target and the call would have
					 * split this dollop. That makes the iterator go
					 * haywire.
					 */
					if (new_target != entry->TargetDollop()) {
						entry->TargetDollop(new_target);
						changed = local_changed = true;
						break;
					}
				}
			}
		} while (local_changed);
		return changed;
	}

	void ZiprDollopManager_t::UpdateAllTargets(void) {
		std::list<Dollop_t *>::const_iterator it, it_end;
		bool changed = false;
		do {
			changed = false;
			for (it = m_dollops.begin(), it_end = m_dollops.end();
		     it != m_dollops.end();
				 /* nop */) {
				Dollop_t *entry = *it;
				it++;
				changed |= UpdateTargets(entry);
			}
		} while (changed);
	}

	std::ostream &operator<<(std::ostream &out, const ZiprDollopManager_t &dollop_man) {
		std::list<Dollop_t *>::const_iterator it, it_end;

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

		std::list<Dollop_t*>::iterator dollop_it, dollop_it_end;
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
}
