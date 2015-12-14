#include <zipr_all.h>
#include <iostream>

namespace Zipr_SDK {
	using namespace libIRDB;
	using namespace zipr;
	Dollop_t::Dollop_t(Instruction_t *start)
	{
		Instruction_t *loop = NULL;

		m_size = 0;
		m_fallthrough_dollop = NULL;

		if (start == NULL)
			return;

		loop = start;
		do {
			push_back(new DollopEntry_t(loop));
		} while (NULL != (loop = loop->GetFallthrough()));

		m_size = CalculateWorstCaseSize();
	}

	size_t Dollop_t::CalculateWorstCaseSize()
	{
		size_t dollop_size = 0;
		Instruction_t *cur_insn = NULL;
		if (front())
			cur_insn = front()->Instruction();

		while (cur_insn != NULL)
		{
			/*
			 * TODO: Take into consideration the dollop might
			 * be MUCH shorter if it is going to jump to a 
			 * previously placed dollop at this insn.
			 */
	#if 0
			if ((to_addr=final_insn_locations[cur_insn]) != 0)
			{
				if (!m_replop)
				{
					if (m_verbose)
						printf("Fallthrough loop detected. ");
					break;
				}
			}
	#endif
			dollop_size += Utils::DetermineWorstCaseInsnSize(cur_insn);
			cur_insn=cur_insn->GetFallthrough();
		}
		return dollop_size;
	}

	Dollop_t *Dollop_t::Split(libIRDB::Instruction_t *split_point) {
		/*
		 * 1. Find the matching dollop entry.
		 */
		DollopEntry_t query(split_point);
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

		new_dollop->Fallthrough(m_fallthrough_dollop);
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

			new_dollop->push_back(to_move);
			erase(moved_it);
		}
		/*
		 * 4. Return the new one
		 */
		return new_dollop;
	}

	DollopEntry_t::DollopEntry_t(libIRDB::Instruction_t *insn) {
		/*
		 * NB: This does not link if the insn has a target.
		 */
		m_instruction = insn;
		m_target_dollop = NULL;
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
		if (fallthrough = d.Fallthrough())
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
