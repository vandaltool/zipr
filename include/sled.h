/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information. 
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *      
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#ifndef sled_h
#define sled_h

class Sled_t
{
	public:
		Sled_t(const Zipr_SDK::MemorySpace_t &memory_space,
		       Range_t range,
		       bool verbose=true)
			: m_memory_space(memory_space),
			  m_range(range),
			  m_verbose(verbose)
		{
		}

		Zipr_SDK::Range_t SledRange(void) const
		{
			return m_range;
		}
		void SledRange(Zipr_SDK::Range_t range)
		{
			m_range = range;
		}

		bool Overlaps(Sled_t candidate)
		{
			if (m_verbose)
				std::cout << "Comparing " << *this << " with candidate " << candidate << std::endl;
			/*
			 * candidate sled: candidate
			 * existing  sled: this
			 *
			 * <---------------->
			 *     candidate
			 *        <--------------->
			 *             existing
			 */
			return candidate.SledRange().getStart() <= m_range.getEnd() &&
			       candidate.SledRange().getEnd() >= m_range.getStart();
		}
		void MergeSledJumpPoints(Sled_t merging)
		{
			for (auto jmp_pnt : merging.m_jmp_pts)
			{
				if (m_verbose)
					std::cout << "Merging jump point: " 
					          << std::hex << jmp_pnt << std::endl;

				m_jmp_pts.insert(jmp_pnt);
			}
		}

		/*
		 * NB: We can only merge in sleds that are before
		 * this sled.
		 */
		void MergeSled(Sled_t merging)
		{
			/*
			 * merging  sled: sled
			 * existing sled: this
			 *
			 * <---------<------>-------->
			 *      merging
			 *        <--------------->
			 *             existing
			 */
			assert(merging.SledRange().getStart() <= m_range.getEnd() &&
			       merging.SledRange().getEnd() >= m_range.getStart());

			/*
			 * First, extend our range.
			 */
			m_range.SetStart(std::min(merging.SledRange().getStart(),
			                          m_range.getStart()));

			m_range.SetEnd(std::max(merging.SledRange().getEnd(),
			                          m_range.getEnd()));

			/*
			 * Second, merge in the jump points to this one.
			 */
			MergeSledJumpPoints(merging);
		}

		IRDB_SDK::Instruction_t *Disambiguation(void) const
		{
			return m_disambiguation;
		}
		void Disambiguation(IRDB_SDK::Instruction_t *disambiguation)
		{
			m_disambiguation = disambiguation;
		}
		void AddJumpPoint(RangeAddress_t jmp_point)
		{
			m_jmp_pts.insert(jmp_point);
		}

		std::set<RangeAddress_t>::iterator JumpPointsBegin()
		{
			return m_jmp_pts.begin();
		}
		std::set<RangeAddress_t>::iterator JumpPointsEnd()
		{
			return m_jmp_pts.end();
		}
		std::set<RangeAddress_t>::reverse_iterator JumpPointsReverseBegin()
		{
			return m_jmp_pts.rbegin();
		}
		std::set<RangeAddress_t>::reverse_iterator JumpPointsReverseEnd()
		{
			return m_jmp_pts.rend();
		}
	friend bool operator<(const Sled_t &lhs, const Sled_t &rhs);
	friend std::ostream &operator<<(std::ostream &out, const Sled_t &sled);

	private:
		const Zipr_SDK::MemorySpace_t &m_memory_space;
		IRDB_SDK::Instruction_t *m_disambiguation;
		std::set<RangeAddress_t> m_jmp_pts;
		Range_t m_range;
		bool m_verbose;
};

inline std::ostream &operator<<(std::ostream &out, const Sled_t &sled)
{
	return out << "Sled (" 
	           << std::hex << sled.m_range.getStart()
	           << std::hex << " - " << sled.m_range.getEnd()
	           << ") with disambiguation "
	           << std::hex << sled.Disambiguation();
}
inline bool operator<(const Sled_t &lhs, const Sled_t &rhs)
{
	return lhs.SledRange().getStart() < rhs.SledRange().getStart();
}

#endif //sled_h
