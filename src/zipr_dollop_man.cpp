#include <zipr_all.h>
#include <iostream>

using namespace zipr;
using namespace std;
using namespace Zipr_SDK;
using namespace libIRDB;

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
	 * Populate the instruction-to-dollop map.
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
