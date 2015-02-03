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

struct Range_tCompare
{
	bool operator()(const Range_t first, const Range_t second)
	{
		return first.GetEnd() < second.GetStart();
	}
};


// a memory space _is_ a map of range addres to char, with additional functionality.
class MemorySpace_t : public std::map<RangeAddress_t,char>
{
	public:
		MemorySpace_t(Options_t *opts) :
			m_opts(opts)
		{ 
		}

		// range operatations
		void SplitFreeRange(RangeAddress_t addr);
		void MergeFreeRange(RangeAddress_t addr);
		std::set<Range_t>::iterator FindFreeRange(RangeAddress_t addr);
		Range_t GetFreeRange(int size);
		void AddFreeRange(Range_t newRange);

		// queries about free areas.
		bool AreBytesFree(RangeAddress_t addr, int num_bytes);
		bool IsByteFree(RangeAddress_t addr);
		bool IsValidRange(std::set<Range_t>::iterator it);

		int GetRangeCount();

		void PrintMemorySpace(std::ostream &out);

		void PlopBytes(RangeAddress_t addr, const char the_byte[], int num)
		{
        		for(int i=0;i<num;i++)
        		{
                		PlopByte(addr+i,the_byte[i]);
        		}
		}
		
		void PlopByte(RangeAddress_t addr, char the_byte)
		{
			min_plopped=std::min(addr,min_plopped);
			max_plopped=std::max(addr,max_plopped);
        		if(this->find(addr) == this->end() )
                		this->SplitFreeRange(addr);
        		(*this)[addr]=the_byte;
		}
		void PlopJump(RangeAddress_t addr)
		{
        		char bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0}; // jmp rel8
                	this->PlopBytes(addr,bytes,sizeof(bytes));
		}
		RangeAddress_t GetMinPlopped() const { return min_plopped; }
		RangeAddress_t GetMaxPlopped() const { return max_plopped; }

	protected:
		std::set<Range_t, Range_tCompare> free_ranges;   // keep ordered
		Options_t *m_opts;

	private:
		RangeAddress_t min_plopped;
		RangeAddress_t max_plopped;
};

#endif
