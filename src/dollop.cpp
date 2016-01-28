#include <zipr_all.h>
#include <iostream>

namespace Zipr_SDK {
	using namespace libIRDB;
	using namespace zipr;
	Dollop_t::Dollop_t(Instruction_t *start) :
		m_size(0),
		m_fallthrough_dollop(NULL),
		m_fallthrough_patched(false),
		m_coalesced(false),
		m_was_truncated(false)
	{
		Instruction_t *loop = NULL;

		if (start == NULL)
			return;

		loop = start;
		do {
			push_back(new DollopEntry_t(loop, this));
		} while ((NULL != (loop = loop->GetFallthrough())) &&
			/*
			 * If this is a pinned instruction, we want to stop!
			 */
		         (NULL == loop->GetIndirectBranchTargetAddress())
						);

		m_size = CalculateWorstCaseSize();
	}

	size_t Dollop_t::CalculateWorstCaseSize()
	{
		size_t dollop_size = 0;
		list<DollopEntry_t*>::const_iterator it, it_end;

		for (it = begin(), it_end = end();
		     it != it_end;
		     it++)
		{
			Instruction_t *cur_insn = (*it)->Instruction();
			dollop_size += Utils::DetermineWorstCaseInsnSize(cur_insn, false);
		}

		if ((m_fallthrough_dollop || (back() && 
		                             back()->Instruction() && 
																 back()->Instruction()->GetFallthrough())) &&
		    (!m_fallthrough_patched && !m_coalesced))
			dollop_size += Utils::TRAMPOLINE_SIZE;
		return dollop_size;
	}

	DollopEntry_t *Dollop_t::FallthroughDollopEntry(DollopEntry_t *entry) const
	{
		list<DollopEntry_t *>::const_iterator found_entry;

		found_entry = std::find(begin(), end(), entry);
		if (found_entry != end() && std::next(found_entry) != end())
			return *(std::next(found_entry));
		else
			return NULL;
	}

	void Dollop_t::WasCoalesced(bool coalesced)
	{
		m_coalesced = coalesced;
		m_size = CalculateWorstCaseSize();
	}

	void Dollop_t::FallthroughPatched(bool patched)
	{
		m_fallthrough_patched = patched;
		m_size = CalculateWorstCaseSize();
	}

	Dollop_t *Dollop_t::Split(libIRDB::Instruction_t *split_point) {
		/*
		 * 1. Find the matching dollop entry.
		 */
		DollopEntry_t query(split_point, NULL);
		std::list<DollopEntry_t *>::iterator de_split_point, de_it;
		Dollop_t *new_dollop = NULL;

		de_split_point = find_if(begin(),end(),
			[&query](const DollopEntry_t *p) {
				std::cout << "Checking "
				          << std::hex << query.Instruction() << " ?= "
									<< std::hex << p->Instruction() << "." << std::endl;
				return query.Instruction() == p->Instruction();
			});
		/*
		 * No matching split point. Just return NULL.
		 */
		if (de_split_point == end())
			return NULL;

		new_dollop = new Dollop_t();

		new_dollop->FallthroughDollop(m_fallthrough_dollop);
		m_fallthrough_dollop = new_dollop;

		/*
		 * 2. Remove them from this one
		 */
		de_it = de_split_point;
		while (de_it != end()) {
		/*
		 * 3. Move them to a new one
		 */
			DollopEntry_t *to_move = *de_it;
			std::list<DollopEntry_t*>::iterator moved_it = de_it;

			de_it++;

			to_move->MemberOfDollop(new_dollop);
			new_dollop->push_back(to_move);
			erase(moved_it);
		}
		new_dollop->m_size = new_dollop->CalculateWorstCaseSize();
		m_size = CalculateWorstCaseSize();

		/*
		 * 4. Return the new one
		 */
		return new_dollop;
	}

	void Dollop_t::RemoveDollopEntries(
		std::list<DollopEntry_t*>::iterator first_to_remove, 
		std::list<DollopEntry_t*>::iterator last_to_remove) {
		erase(first_to_remove, last_to_remove);
		m_size = CalculateWorstCaseSize();
	}

	DollopEntry_t::DollopEntry_t(libIRDB::Instruction_t *insn,Dollop_t *member_of)
	{
		/*
		 * NB: This does not link if the insn has a target.
		 */
		m_instruction = insn;
		m_target_dollop = NULL;
		m_member_of_dollop = member_of;
	}

	bool DollopEntry_t::operator==(const DollopEntry_t &comp) {
		std::cout << "operator==s being invoked "
		          << "(" << std::hex << m_instruction
		          << ", " << std::hex << comp.m_instruction
							<< ") "
		          << "(" << std::hex << m_target_dollop
		          << ", " << std::hex << comp.m_target_dollop
							<< ") "
							<< std::endl;
		return comp.m_instruction == m_instruction &&
		       comp.m_target_dollop == m_target_dollop;
	}
	bool DollopEntry_t::operator!=(const DollopEntry_t &comp) {
		return !operator==(comp);
	}
	Dollop_t *Dollop_t::CreateNewDollop(libIRDB::Instruction_t *start) {
		return new Zipr_SDK::Dollop_t(start);
	}
	std::ostream &operator<<(std::ostream &out, const Dollop_t &d) {
		std::list<DollopEntry_t*>::const_iterator it, it_end;
		Dollop_t *fallthrough = NULL;

		for (it = d.begin(), it_end = d.end();
		     it != it_end;
				 it++) {
			out << std::hex << *(*it) << std::endl;
		}
		if (fallthrough = d.FallthroughDollop())
			out << "Fallthrough: " << std::hex << fallthrough << std::endl;
		return out;
	}

	std::ostream &operator<<(std::ostream &out, const DollopPatch_t &p) {
		out << std::hex << &p << ":" << std::hex << p.Target();
		return out;
	}
	std::ostream &operator<<(std::ostream &out, const DollopEntry_t &p) {
		out << "Instruction: " << std::hex << p.Instruction() << std::endl;
		out << "Target Dollop: " << std::hex << p.TargetDollop() << std::endl;
		return out;
	}
}
