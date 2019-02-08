/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <zipr_all.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <algorithm>    // std::random_shuffle
#include <vector>       // std::vector
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand


using namespace zipr;
using namespace std;

ZiprOptionsNamespace_t *ZiprMemorySpace_t::RegisterOptions(ZiprOptionsNamespace_t *global) {
	global->addOption(&m_verbose);
	return nullptr;
}

void ZiprMemorySpace_t::SplitFreeRange(Range_t split_from)
{
	RangeAddress_t counter, end;
	for (counter = split_from.getStart(), end = split_from.getEnd();
	     counter!=end;
	     counter++)
	{
		SplitFreeRange(counter);
	}
}

void ZiprMemorySpace_t::SplitFreeRange(RangeAddress_t addr)
{
	const auto it=FindFreeRange(addr);
	assert(IsValidRange(it));

	const auto r=*it;
	if(r.getStart()==r.getEnd())
	{
		assert(addr==r.getEnd());
		free_ranges.erase(it);
	}
	else if(addr==r.getStart())
	{
		free_ranges.erase(it);
		free_ranges.insert(Range_t(r.getStart()+1, r.getEnd()));
	}
	else if(addr==r.getEnd())
	{
		free_ranges.erase(it);
		free_ranges.insert(Range_t(r.getStart(), r.getEnd()-1));
	}
	else // split range 
	{
		free_ranges.erase(it);
		free_ranges.insert(Range_t(r.getStart(), addr-1));
		free_ranges.insert(Range_t(addr+1, r.getEnd()));
	}
}
void ZiprMemorySpace_t::MergeFreeRange(Range_t range)
{
	RangeAddress_t counter, end;
	for (counter = range.getStart(), end = range.getEnd();
	     counter!=end;
	     counter++)
	{
		MergeFreeRange(counter);
	}
}
void ZiprMemorySpace_t::MergeFreeRange(RangeAddress_t addr)
{
	/*
	 * Make a new range of one byte.
	 * 
	 * Then, look to see whether or not it
	 * can be merged with another range.
	 *
	 * If not, add it as a one byte range.
	 */

	Range_t nr(addr, addr);
	Range_t nrp1(addr+1, addr+1);
	Range_t nrm1(addr-1, addr-1);
	RangeSet_t::iterator itp1;
	RangeSet_t::iterator itm1;

	itp1=free_ranges.find(nrp1);
	itm1=free_ranges.find(nrm1);
	Range_t r;
	if(itp1!=free_ranges.end())
	{
		r=*itp1;
		assert((addr+1) == r.getStart());
		/*
		 * Make the beginning of this range
		 * one byte smaller!
		 */
		Range_t nnr(addr, r.getEnd());
		if (m_verbose)
		{
			printf("Expanded range: ");
			printf("from: (%p - %p) ", (void*)r.getStart(), (void*)r.getEnd());
			printf("to: (%p - %p) \n", (void*)nnr.getStart(), (void*)nnr.getEnd());
		}
		nr = nnr;
		/*
		 * There is the possibility that we just expanded down and we
		 * now abut the end of a smaller free one.
		 */
		if(itm1!=free_ranges.end())
		{
			Range_t r2=*itm1;
			if (m_verbose)
			{
				printf("Expanded range: ");
				printf("from: (%p - %p) ", (void*)nr.getStart(), (void*)nr.getEnd());
				printf("to: (%p - %p) \n", (void*)r2.getStart(), (void*)nnr.getEnd());
			}
			nr.setStart(r2.getStart());	
			free_ranges.erase(r2);
			
		}
		/*
		 * Handling the analagous secondary merge case
		 * does not need to be handled here -- that 
		 * would have been taken care of in the 1st 
		 * conditional (just using opposite terms).
		 */
		free_ranges.erase(r);
	} 
	else if(itm1!=free_ranges.end())	 // not addr+1 is still busy, so we don't merge against it.
	{
		r=*itm1;
	
		assert((addr-1) == r.getEnd()) ;
		/*
		 * Make the end of this range one byte
		 * bigger
		 */
		Range_t nnr(r.getStart(), addr);
		if (m_verbose)
		{
			printf("Expanded range: ");
			printf("from: (%p - %p) ", (void*)r.getStart(), (void*)r.getEnd());
			printf("to: (%p - %p) \n", (void*)nnr.getStart(), (void*)nnr.getEnd());
		}
		nr = nnr;
		free_ranges.erase(itm1);
	}
	else
	{
		// else both p1 and m1 are busy still, so we just insert nr
	}

	// insert NR and be done.
	free_ranges.insert(nr);
	return;

}

void ZiprMemorySpace_t::PrintMemorySpace(std::ostream &out)
{
	for(auto r : free_ranges)
	{
		out <<"0x"<<std::hex<<r.getStart()<<" - 0x"<<std::hex<<r.getEnd()<<endl;
	}
}

RangeSet_t::iterator ZiprMemorySpace_t::FindFreeRange(RangeAddress_t addr)
{
	auto freer = free_ranges.find(Range_t(addr, addr)); 
	return freer;
}

bool ZiprMemorySpace_t::IsValidRange(RangeSet_t::iterator it)
{
	return it!=free_ranges.end();
}

std::pair<RangeSet_t::const_iterator,RangeSet_t::const_iterator>
	ZiprMemorySpace_t::getNearbyFreeRanges(const RangeAddress_t hint,size_t count)
{
	const auto search=Range_t(hint, hint+1);
	const auto result = free_ranges.lower_bound(search);
	/*
	 * TODO: Not quite sure what to make of this.
	 */
	return std::pair<RangeSet_t::const_iterator,RangeSet_t::const_iterator>(
		result, free_ranges.end());
}

Range_t ZiprMemorySpace_t::GetLargeRange(void)
{
	for(auto r : free_ranges)
	{
		if(r.getEnd()==(RangeAddress_t)-1)
			return r;
	}
	return Range_t(0,0);
}

bool ZiprMemorySpace_t::SortRangeBySize(const Range_t &a, const Range_t &b)
{
	return (a.getEnd() - a.getStart()) <= (b.getEnd() - b.getStart());
}

std::list<Range_t> ZiprMemorySpace_t::getFreeRanges(size_t size)
{
	auto result=list<Range_t>();
	for(auto r : free_ranges)
	{
		if(r.getEnd() - r.getStart() >= (unsigned) size)
			result.push_back(r);
	}
	result.sort(SortRangeBySize);
	return result;
}

Range_t ZiprMemorySpace_t::getInfiniteFreeRange()
{
	for(auto r : free_ranges)
	{
		if(r.getEnd()==(RangeAddress_t)-1)
			return r;
	}
	assert(false);
	return Range_t(0,0);
}

Range_t ZiprMemorySpace_t::getFreeRange(int size)
{
	vector<Range_t> v;
	Range_t big_range;
	for(auto r : free_ranges)
	{
		if(r.getEnd()==(RangeAddress_t)-1)
			big_range=r;
		else if(r.getEnd() - r.getStart() >= (unsigned) size)
			v.push_back(r);

		// that's enough randomization
		if(v.size() > 100)
			break;
	}
	if(v.size()==0)
		return big_range;	

	// choose random value to return.
	int index=std::rand() % v.size();
	return v[index];
}

// queries about free areas.
bool ZiprMemorySpace_t::AreBytesFree(RangeAddress_t addr, int num_bytes)
{
	for(int i=0;i<num_bytes;i++)
		if(!IsByteFree(addr+i))
			return false;
	return true;
}

bool ZiprMemorySpace_t::IsByteFree(RangeAddress_t addr)
{
	if (IsValidRange(FindFreeRange(addr)))
		return true;
	return false;
}

void ZiprMemorySpace_t::AddFreeRange(Range_t newRange, bool original)
{
	if (original)
	{
		original_free_ranges.insert(Range_t(newRange.getStart(),newRange.getEnd()));
	}
	AddFreeRange(newRange);
}

void ZiprMemorySpace_t::AddFreeRange(Range_t newRange)
{
	free_ranges.insert(Range_t(newRange.getStart(), newRange.getEnd()));
}
void ZiprMemorySpace_t::RemoveFreeRange(Range_t oldRange)
{
	free_ranges.erase(Range_t(oldRange.getStart(), oldRange.getEnd()));
}

int ZiprMemorySpace_t::GetRangeCount()
{
	return free_ranges.size();
}
