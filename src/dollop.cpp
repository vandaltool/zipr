#include <zipr_all.h>
#include <iostream>

#define ALLOF(a) std::begin((a)),std::end((a))

namespace Zipr_SDK
{
	bool DollopEntry_t::operator==(const  DollopEntry_t &comp) 
	{
		return 
			make_tuple(     getInstruction(),      getTargetDollop()) == 
			make_tuple(comp.getInstruction(), comp.getTargetDollop()) ;
	}

	bool DollopEntry_t::operator!=(const Zipr_SDK::DollopEntry_t &comp) 
	{
		return !operator==(comp);
	}

	ostream &operator<<(ostream &out, const Zipr_SDK::Dollop_t &d) 
	{

		//for (it = d.begin(), it_end = d.end(); it != it_end; it++) 
		for (auto entry : d) 
		{
			out << hex << *(entry) << endl;
		}
		auto fallback = d.getFallbackDollop();
		if ( fallback != nullptr)
			out << "Fallback: " << hex << fallback << endl;
		auto fallthrough = d.getFallthroughDollop();
		if ( fallthrough != nullptr)
			out << "Fallthrough: " << hex << fallthrough << endl;
		return out;
	}
#if 0
	ostream &operator<<(ostream &out, const DollopPatch_t &p) {
		out << hex << &p << ":" << hex << p.getTarget();
		return out;
	}
#endif

	ostream &operator<<(ostream &out, const DollopEntry_t &p) {
		out << "Instruction: " << hex << p.getInstruction() << endl;
		out << "Target Dollop: " << hex << p.getTargetDollop() << endl;
		return out;
	}
}

namespace zipr 
{
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

	void Dollop_t::reCalculateSize()
	{
		m_size = CalculateSize();
	}

	size_t Dollop_t::CalculateSize()
	{
		// calculate the total, worst-case size of each dollop entry
		auto total_dollop_entry_size = (size_t)0;
		assert(m_dollop_mgr);
		for (auto cur_de : *this )
			total_dollop_entry_size += m_dollop_mgr->determineDollopEntrySize(cur_de);

		// now determine if we need to add space for a trampoline.

		// no need for a trampoline if patched and/or coalesced.
		if(m_fallthrough_patched || m_coalesced)
			return total_dollop_entry_size;

		// no need for a trampoline if there is no fallthrough
		const auto has_fallthrough = m_fallthrough_dollop || 
		                             (back() && back()->getInstruction() && back()->getInstruction()->getFallthrough())
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
	Zipr_SDK::DollopEntry_t *Dollop_t::getFallthroughDollopEntry(Zipr_SDK::DollopEntry_t *entry) const
	{
		const auto found_entry = find(ALLOF(*this), entry);
		if (found_entry == end())
		       return nullptr;
		const auto next_entry=next(found_entry);
		return next_entry == end() ? nullptr : *next_entry ;
	}

	void Dollop_t::setCoalesced(bool coalesced)
	{
		m_coalesced = coalesced;
		m_size = CalculateSize();
	}

	void Dollop_t::setFallthroughPatched(bool patched)
	{
		m_fallthrough_patched = patched;
		m_size = CalculateSize();
	}

	Zipr_SDK::Dollop_t *Dollop_t::split(IRDB_SDK::Instruction_t *split_point) 
	{
		/*
		 * 1. Find the matching dollop entry.
		 */
		DollopEntry_t query(split_point, nullptr);
		Dollop_t *new_dollop = nullptr;

		auto de_split_point = find_if(begin(),end(),
			[&query](const Zipr_SDK::DollopEntry_t *p) {
//				cout << "Checking "
//				          << hex << query.Instruction() << " ?= "
//									<< hex << p->Instruction() << "." << endl;
				return query.getInstruction() == p->getInstruction();
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
			m_fallthrough_dollop->setFallbackDollop(new_dollop);
		new_dollop->setFallbackDollop(this);

		new_dollop->setFallthroughDollop(m_fallthrough_dollop);
		m_fallthrough_dollop = new_dollop;

		/*
		 * 2. Remove them from this one
		 */
		auto de_it = de_split_point;
		while (de_it != end()) {
		/*
		 * 3. Move them to a new one
		 */
			auto to_move = *de_it;
			auto moved_it = de_it;

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

	void Dollop_t::removeDollopEntries(
		Zipr_SDK::DollopEntryList_t::iterator first_to_remove, 
		Zipr_SDK::DollopEntryList_t::iterator last_to_remove) 
	{
		erase(first_to_remove, last_to_remove);
		m_size = CalculateSize();
	}

	DollopEntry_t::DollopEntry_t(IRDB_SDK::Instruction_t *insn, Zipr_SDK::Dollop_t *member_of)
	{
		/*
		 * NB: This does not link if the insn has a target.
		 */
		m_instruction = insn;
		m_target_dollop = nullptr;
		m_member_of_dollop = member_of;
	}

	bool DollopEntry_t::operator==(const  DollopEntry_t &comp) {
		cout << "operator==s being invoked "
		          << "(" << hex << m_instruction
		          << ", " << hex << comp.m_instruction
							<< ") "
		          << "(" << hex << m_target_dollop
		          << ", " << hex << comp.m_target_dollop
							<< ") "
							<< endl;
		return comp.m_instruction == m_instruction &&
		       comp.m_target_dollop == m_target_dollop;
	}

	bool DollopEntry_t::operator!=(const DollopEntry_t &comp) {
		return !operator==(comp);
	}



	ostream &operator<<(ostream &out, const Dollop_t &d) 
	{

		//for (it = d.begin(), it_end = d.end(); it != it_end; it++) 
		for (auto entry : d) 
		{
			out << hex << *(entry) << endl;
		}
		auto fallback = d.getFallbackDollop();
		if ( fallback != nullptr)
			out << "Fallback: " << hex << fallback << endl;
		auto fallthrough = d.getFallthroughDollop();
		if ( fallthrough != nullptr)
			out << "Fallthrough: " << hex << fallthrough << endl;
		return out;
	}

	ostream &operator<<(ostream &out, const DollopPatch_t &p) {
		out << hex << &p << ":" << hex << p.getTarget();
		return out;
	}

	ostream &operator<<(ostream &out, const DollopEntry_t &p) {
		out << "Instruction: " << hex << p.getInstruction() << endl;
		out << "Target Dollop: " << hex << p.getTargetDollop() << endl;
		return out;
	}

	Zipr_SDK::Dollop_t* Dollop_t::createNewDollop(IRDB_SDK::Instruction_t *start, Zipr_SDK::DollopManager_t *mgr) 
	{
		return new zipr::Dollop_t(start, mgr);
	}
}
