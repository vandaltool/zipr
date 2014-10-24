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

#ifndef memory_space_h
#define memory_space_h

class Options_t;

class MemorySpace_t
{
	public:
		MemorySpace_t():m_opts(NULL),m_is_sorted(false),free_ranges_ptrs(NULL) { }
		MemorySpace_t(Options_t *opts):m_opts(opts),m_is_sorted(false),free_ranges_ptrs(NULL) { }

		// range operatations
		void SplitFreeRange(RangeAddress_t addr);
		void MergeFreeRange(RangeAddress_t addr);
		std::list<Range_t>::iterator FindFreeRange(RangeAddress_t addr);
		Range_t GetFreeRange(int size);
		void AddFreeRange(Range_t newRange);

		// queries about free areas.
		bool AreBytesFree(RangeAddress_t addr, int num_bytes);
		bool IsByteFree(RangeAddress_t addr);
		bool IsValidRange(std::list<Range_t>::iterator it);

		int GetRangeCount();

		void Sort();

		void PrintMemorySpace(std::ostream &out);
	protected:
		std::list<Range_t> free_ranges;   // keep ordered
		Options_t *m_opts;
		bool m_is_sorted;
	private:
		std::list<Range_t>::iterator FindFreeRangeB(int startIndex, int stopIndex,
			RangeAddress_t addr);
		std::list<Range_t>::iterator **free_ranges_ptrs;
		int free_ranges_ptrs_size;
};

#endif
