#include <zipr_all.h>
#include <iostream>

#define ALLOF(a) std::begin((a)),std::end((a))

namespace Zipr_SDK {
	using namespace IRDB_SDK;
	using namespace zipr;
	using namespace std;
	Dollop_t::Dollop_t(Instruction_t *start, Zipr_SDK::DollopManager_t *mgr) :
		m_size(0),
		m_fallthrough_dollop(nullptr),
		m_fallback_dollop(nullptr),
		m_fallthrough_patched(false),
		m_coalesced(false),
		m_was_truncated(false),
		m_dollop_mgr(mgr)
	{
		Instruction_t *loop = nullptr;

		if (start == nullptr)
			return;

		loop = start;
		do {
			push_back(new DollopEntry_t(loop, this));
		} while ((nullptr != (loop = loop->getFallthrough())) &&
			/*
			 * If this is a pinned instruction (or unpinned IBT), we want to stop!
			 */
		         (nullptr == loop->getIndirectBranchTargetAddress())
						);

		m_size = CalculateSize();
	}

	void Dollop_t::ReCalculateSize()
	{
		m_size = CalculateSize();
	}

	size_t Dollop_t::CalculateSize()
	{
		// calculate the total, worst-case size of each dollop entry
		auto total_dollop_entry_size = (size_t)0;
		assert(m_dollop_mgr);
		for (auto cur_de : *this )
			total_dollop_entry_size += m_dollop_mgr->DetermineDollopEntrySize(cur_de);

		// now determine if we need to add space for a trampoline.

		// no need for a trampoline if patched and/or coalesced.
		if(m_fallthrough_patched || m_coalesced)
			return total_dollop_entry_size;

		// no need for a trampoline if there is no fallthrough
		const auto has_fallthrough = m_fallthrough_dollop || 
		                             (back() && back()->Instruction() && back()->Instruction()->getFallthrough())
					     ;
		if (!has_fallthrough)
			return total_dollop_entry_size;

		// save space for a trampoline.
		assert(m_dollop_mgr);
		const auto l_dollop_mgr=dynamic_cast<ZiprDollopManager_t*>(m_dollop_mgr);
		const auto l_zipr=l_dollop_mgr->GetZipr();
		const auto l_zipr_impl=dynamic_cast<ZiprImpl_t*>(l_zipr);
		assert(l_zipr_impl);
		return 	total_dollop_entry_size + l_zipr_impl->getSizer()->TRAMPOLINE_SIZE;
	}
	DollopEntry_t *Dollop_t::FallthroughDollopEntry(DollopEntry_t *entry) const
	{
		const auto found_entry = find(ALLOF(*this), entry);
		if (found_entry == end())
		       return nullptr;
		const auto next_entry=next(found_entry);
		return next_entry == end() ? nullptr : *next_entry ;
	}

	void Dollop_t::WasCoalesced(bool coalesced)
	{
		m_coalesced = coalesced;
		m_size = CalculateSize();
	}

	void Dollop_t::FallthroughPatched(bool patched)
	{
		m_fallthrough_patched = patched;
		m_size = CalculateSize();
	}

	Dollop_t *Dollop_t::Split(IRDB_SDK::Instruction_t *split_point) {
		/*
		 * 1. Find the matching dollop entry.
		 */
		DollopEntry_t query(split_point, nullptr);
		std::list<DollopEntry_t *>::iterator de_split_point, de_it;
		Dollop_t *new_dollop = nullptr;

		de_split_point = find_if(begin(),end(),
			[&query](const DollopEntry_t *p) {
//				std::cout << "Checking "
//				          << std::hex << query.Instruction() << " ?= "
//									<< std::hex << p->Instruction() << "." << std::endl;
				return query.Instruction() == p->Instruction();
			});
		/*
		 * No matching split point. Just return nullptr.
		 */
		if (de_split_point == end())
			return nullptr;

		new_dollop = new Dollop_t();

		new_dollop->setDollopManager(m_dollop_mgr);

		/*
		 * Set fallthrough and fallback dollop pointers.
		 *    ----- ----
		 *    |   | |   |
		 * this - new - fallthrough
		 *              |
		 *         |-----
		 */
		if (m_fallthrough_dollop)
			m_fallthrough_dollop->FallbackDollop(new_dollop);
		new_dollop->FallbackDollop(this);

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
		new_dollop->m_size = new_dollop->CalculateSize();
		m_size = CalculateSize();

		/*
		 * 4. Return the new one
		 */
		return new_dollop;
	}

	void Dollop_t::RemoveDollopEntries(
		std::list<DollopEntry_t*>::iterator first_to_remove, 
		std::list<DollopEntry_t*>::iterator last_to_remove) {
		erase(first_to_remove, last_to_remove);
		m_size = CalculateSize();
	}

	DollopEntry_t::DollopEntry_t(IRDB_SDK::Instruction_t *insn,Dollop_t *member_of)
	{
		/*
		 * NB: This does not link if the insn has a target.
		 */
		m_instruction = insn;
		m_target_dollop = nullptr;
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

	Dollop_t *Dollop_t::CreateNewDollop(IRDB_SDK::Instruction_t *start,
	                                    Zipr_SDK::DollopManager_t *mgr) {
		return new Zipr_SDK::Dollop_t(start, mgr);
	}

	std::ostream &operator<<(std::ostream &out, const Dollop_t &d) {
		std::list<DollopEntry_t*>::const_iterator it, it_end;
		Dollop_t *fallthrough = nullptr, *fallback = nullptr;

		for (it = d.begin(), it_end = d.end();
		     it != it_end;
				 it++) {
			out << std::hex << *(*it) << std::endl;
		}
		if ((fallback = d.FallbackDollop()) != nullptr)
			out << "Fallback: " << std::hex << fallback << std::endl;
		if ((fallthrough = d.FallthroughDollop()) != nullptr)
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
