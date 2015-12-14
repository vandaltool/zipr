#include <zipr_all.h>
#include <iostream>

using namespace zipr;
using namespace std;
using namespace Zipr_SDK;
using namespace libIRDB;

namespace zipr {

	Dollop_t *ZiprDollopManager_t::AddNewDollop(Instruction_t *start) {
		Dollop_t *new_dollop = Dollop_t::CreateNewDollop(start);
		AddDollop(new_dollop);
		return new_dollop;
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

	void ZiprDollopManager_t::UpdateTargets(Dollop_t *dollop) {
		list<DollopEntry_t*>::iterator it, it_end;
		for (it = dollop->begin(), it_end = dollop->end();
		     it != it_end;
				 it++) {
			DollopEntry_t *entry = *it;
			if (entry->Instruction() &&
			    entry->Instruction()->GetTarget()) {
				/*
				 * Update target.
				 */
				Dollop_t *target_dollop = GetContainingDollop(entry
				                                              ->Instruction()
																											->GetTarget());
				/*
				 * This is the target dollop *only*
				 * if the target instruction is the first instruction.
				 */
				if (target_dollop)
				{
					/*
					 * There is a target dollop. But, do we need to split it?
					 */
					if (target_dollop->GetDollopEntryCount() &&
					    target_dollop->front()->Instruction() == entry->Instruction()->GetTarget()) {
						/*
						 * Just update the target.
						 */
						entry->TargetDollop(target_dollop);
					}
					else {
						/*
						 * Split at this dollop to make a new one!
						 */
						AddDollop(target_dollop->Split(entry->Instruction()->GetTarget()));
					}
				}
				else {
					/*
					 * There is no target dollop. Let's create one!
					 */
					AddNewDollop(entry->Instruction()->GetTarget());
				}
			}
		}
		return;
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
