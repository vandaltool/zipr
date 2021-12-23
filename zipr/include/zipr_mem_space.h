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

class FreeRanges_t 
{

	public:
		FreeRanges_t()
			: by_size(32)
		{

		}
		RangeSet_t::iterator find( const Range_t& key )
		{
			return by_address.find(key);
		}
		RangeSet_t::const_iterator find( const Range_t& key ) const
		{
			return by_address.find(key);
		}
		pair<RangeSet_t::iterator,bool> insert( const Range_t& value )
		{
			const auto size   = value.getSize();
			assert(size>=1);
			const auto bucket = integerlog2floor(size);
			by_size[bucket].insert(value);
			return by_address.insert(value);
		}
		RangeSet_t::iterator erase( RangeSet_t::iterator pos )
		{
			assert(pos!=by_address.end());
			const auto theRange=*pos;
			const auto size   = theRange.getSize();
			assert(size>=1);
			const auto bucket = integerlog2floor(size);
			by_size[bucket].erase(theRange);
			return by_address.erase(pos);
		}
		RangeSet_t::iterator erase( const Range_t& range)
		{
			return erase(find(range));
		}
		const RangeSet_t& byAddress() const { return by_address; }
		const RangeSet_t& bySize(const size_t size) const
		{ 
			assert(size>=1);
			const auto bucket = integerlog2ceil(size);
			return by_size[bucket]; 
		}
		size_t size() const { return by_address.size(); }

	private:
		static unsigned integerlog2ceil (unsigned int index)
		{
		    auto result = 0u;
		    --index;
		    while (index > 0) {
			++result;
			index >>= 1;
		    }

		    return result;
		}
		static unsigned integerlog2floor(unsigned int index)
		{
			assert(index>=0);
			int targetlevel = 0;
			while (index >>= 1) 
				++targetlevel;
			return  targetlevel;
		}

		RangeSet_t  by_address; // keep ordered
		vector< RangeSet_t > by_size;
};

// a memory space _is_ a map of range addres to char, with additional functionality.
class ZiprMemorySpace_t : public MemorySpace_t
{
	public:
		ZiprMemorySpace_t() :
			free_ranges(),
			original_free_ranges(),
			max_plopped(0),
			min_plopped(-1)
		{ 
		}

		void registerOptions(ZiprOptions_t* opt_man); 

		// range operatations
		void splitFreeRange(RangeAddress_t addr) { return SplitFreeRange(addr); } 
		void splitFreeRange(Range_t range)       { return SplitFreeRange(range); } 
		void mergeFreeRange(RangeAddress_t addr) { return MergeFreeRange(addr); } 
		void mergeFreeRange(Range_t range)       { return MergeFreeRange(range); } 
		RangeSet_t::iterator findFreeRange(RangeAddress_t addr) { return FindFreeRange(addr); } 
		void addFreeRange(Range_t newRange)                { return AddFreeRange(newRange); } 
		void addFreeRange(Range_t newRange, bool original) { return AddFreeRange(newRange, original); } 
		void removeFreeRange(Range_t newRange) {return RemoveFreeRange(newRange); } 
		Range_t getLargeRange(void) { return GetLargeRange(); } 
		bool areBytesFree(RangeAddress_t addr, int num_bytes) { return AreBytesFree(addr,num_bytes); } 
		bool isByteFree(RangeAddress_t addr) { return IsByteFree(addr); } 
		bool isValidRange(RangeSet_t::iterator it) { return IsValidRange(it); } 
		void printMemorySpace(std::ostream &out) { return PrintMemorySpace(out); } 
		void plopBytes(RangeAddress_t addr, const char the_byte[], int num) { return PlopBytes(addr,the_byte,num); } 
		void plopByte(RangeAddress_t addr, char the_byte) { return PlopByte(addr,the_byte); } 
		void plopJump(RangeAddress_t addr) { return PlopJump(addr); } 
		RangeAddress_t getMinPlopped() const { return min_plopped; }
		RangeAddress_t getMaxPlopped() const { return max_plopped; }
		int getRangeCount() { return GetRangeCount(); } 



		// range operations
		void SplitFreeRange(RangeAddress_t addr);
		void SplitFreeRange(Range_t split_from);
		void MergeFreeRange(Range_t range);
		void MergeFreeRange(RangeAddress_t addr);
		RangeSet_t::iterator FindFreeRange(RangeAddress_t addr);
		Range_t getFreeRange(int size);
		Range_t getInfiniteFreeRange();
		std::list<Range_t> getFreeRanges(size_t size = 0);
		std::pair<RangeSet_t::const_iterator,RangeSet_t::const_iterator>
			getNearbyFreeRanges(const RangeAddress_t hint, size_t count = 0);
		void AddFreeRange(Range_t newRange);
		void AddFreeRange(Range_t newRange, bool original);
		void RemoveFreeRange(Range_t newRange);
		Range_t GetLargeRange(void);

		// queries about free areas.
		bool AreBytesFree(RangeAddress_t addr, int num_bytes);
		bool IsByteFree(RangeAddress_t addr);
		bool IsValidRange(RangeSet_t::iterator it);

		int GetRangeCount();
		RangeSet_t getOriginalFreeRanges() const { return original_free_ranges; }

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

			const auto is_free_addr = IsValidRange(FindFreeRange(addr));
			if(is_free_addr) /* and, the range is free. */
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
		FreeRanges_t free_ranges;
		RangeSet_t  original_free_ranges; // keep ordered

	private:
		RangeAddress_t max_plopped;
		RangeAddress_t min_plopped;
		Zipr_SDK::ZiprBooleanOption_t* m_verbose;
		static bool SortRangeBySize(const Range_t &a, const Range_t &b);
};

#endif
