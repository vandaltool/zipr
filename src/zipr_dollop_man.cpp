#include <zipr_all.h>
#include <iostream>

using namespace zipr;
using namespace std;
using namespace Zipr_SDK;
using namespace libIRDB;

namespace zipr {

	Dollop_t *ZiprDollopManager_t::AddNewDollop(Instruction_t *start) {
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
				AddDollop(new_dollop = existing_dollop->Split(start));
				return new_dollop;
			}
		}
		else {
			/*
			 * There is no target dollop. Let's create one!
			 */
			new_dollop = Dollop_t::CreateNewDollop(start);
			AddDollop(new_dollop);
			return new_dollop;
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

	void ZiprDollopManager_t::AddDollop(Dollop_t *dollop) {
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
		 * Push the actual dollop onto the list of dollops.
		 */
		m_dollops.push_back(dollop);
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
					Dollop_t *new_target=AddNewDollop(entry->Instruction()->GetTarget());

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
}
