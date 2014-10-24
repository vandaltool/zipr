#include <zipr_all.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

using namespace zipr;
using namespace std;

void MemorySpace_t::SplitFreeRange(RangeAddress_t addr)
{
	list<Range_t>::iterator it=FindFreeRange(addr);
	assert(IsValidRange(it));

	Range_t r=*it;

	if(r.GetStart()==r.GetEnd())
	{
		assert(addr==r.GetEnd());
		free_ranges.erase(it);
	}
	else if(addr==r.GetStart())
	{
		free_ranges.insert(it, Range_t(r.GetStart()+1, r.GetEnd()));
		free_ranges.erase(it);
	}
	else if(addr==r.GetEnd())
	{
		free_ranges.insert(it, Range_t(r.GetStart(), r.GetEnd()-1));
		free_ranges.erase(it);
	}
	else // split range 
	{
		free_ranges.insert(it, Range_t(r.GetStart(), addr-1));
		free_ranges.insert(it, Range_t(addr+1, r.GetEnd()));
		free_ranges.erase(it);
	}
}

void MemorySpace_t::MergeFreeRange(RangeAddress_t addr)
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
	bool merged = false;
	list<Range_t>::iterator it=free_ranges.begin();
	for(;it!=free_ranges.end();++it)
	{
		Range_t r=*it;
		if ((addr+1) == r.GetStart()) {
			/*
			 * Make the beginning of this range
			 * one byte smaller!
			 */
			Range_t nnr(addr, r.GetEnd());
			if(m_opts != NULL && m_opts->GetVerbose())
			{
				printf("Expanded range:\n");
				printf("from: %p to %p\n", (void*)r.GetStart(), (void*)r.GetEnd());
				printf("to: %p to %p\n", (void*)nnr.GetStart(), (void*)nnr.GetEnd());
			}
			nr = nnr;
			free_ranges.erase(it);
			break;
		} else if ((addr-1) == r.GetEnd()) {
			/*
			 * Make the end of this range one byte
			 * bigger
			 */
			Range_t nnr(r.GetStart(), addr);
			if(m_opts != NULL && m_opts->GetVerbose())
			{
				printf("Expanded range:\n");
				printf("from: %p to %p\n", (void*)r.GetStart(), (void*)r.GetEnd());
				printf("to: %p to %p\n", (void*)nnr.GetStart(), (void*)nnr.GetEnd());
			}
			nr = nnr;
			free_ranges.erase(it);
			break;
		}
	}

	/*
	 * Correctness: 
	 * Take a pass through and see if there are
	 * free ranges that can now be merged. This
	 * is important because it's possible that
	 * we added the byte to the end of a range
	 * where it could also have gone at the
	 * beginning of another.
	 */
	for(it=free_ranges.begin();it!=free_ranges.end();++it)
	{
		Range_t r = *it;
		if ((
				/*
				 * <--r-->
				 *    <--nr-->
				 */
				((r.GetEnd()+1) >= nr.GetStart() &&
				r.GetEnd() <= nr.GetEnd()) ||
				/*
				 * <--nr-->
				 *     <--r-->
				 */
				((nr.GetEnd()+1) >= r.GetStart() &&
				nr.GetEnd() <= r.GetEnd())
				) &&
				/*
				 * The ranges themselves are not
				 * identical.
				 */
				(r.GetStart() != nr.GetStart() ||
				r.GetEnd() != nr.GetEnd()))
		{
			/*
			 * merge.
			 */
			Range_t merged_range(std::min(r.GetStart(), nr.GetStart()), std::max(r.GetEnd(), nr.GetEnd()));
			if(m_opts != NULL && m_opts->GetVerbose())
			{
				printf("Merged two ranges:\n");
				printf("r:  %p to %p\n", (void*)r.GetStart(), (void*)r.GetEnd());
				printf("nr: %p to %p\n", (void*)nr.GetStart(), (void*)nr.GetEnd());
				printf("to: %p to %p\n", (void*)merged_range.GetStart(), (void*)merged_range.GetEnd());
			}
			free_ranges.insert(it, merged_range);
			free_ranges.erase(it);
			merged = true;
			break;
		}
	}

	if (!merged)
	{
		free_ranges.push_back(nr);
	}
}

void MemorySpace_t::PrintMemorySpace(std::ostream &out)
{
	for( list<Range_t>::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r = *it;
		out <<"0x"<<std::hex<<r.GetStart()<<" - 0x"<<std::hex<<r.GetEnd()<<endl;
	}
}

std::list<Range_t>::iterator MemorySpace_t::FindFreeRange(RangeAddress_t addr)
{
	for( list<Range_t>::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r=*it;
		if(r.GetStart() <= addr && addr <=r.GetEnd())
			return it;
	}
	return free_ranges.end();
}

bool MemorySpace_t::IsValidRange(std::list<Range_t>::iterator it)
{
	return it!=free_ranges.end();
}

Range_t MemorySpace_t::GetFreeRange(int size)
{
	for( list<Range_t>::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r=*it;
		if(r.GetEnd() - r.GetStart() > size)
			return r;
	}
	assert(0);// assume we find a big enough range.
}

// queries about free areas.
bool MemorySpace_t::AreBytesFree(RangeAddress_t addr, int num_bytes)
{
	for(int i=0;i<num_bytes;i++)
		if(!IsByteFree(addr+i))
			return false;
	return true;
}

bool MemorySpace_t::IsByteFree(RangeAddress_t addr)
{
	if (IsValidRange(FindFreeRange(addr)))
		return true;
	return false;
}

void MemorySpace_t::AddFreeRange(Range_t newRange)
{
	free_ranges.push_back(Range_t(newRange.GetStart(), newRange.GetEnd()));
}
int MemorySpace_t::GetRangeCount()
{
	return free_ranges.size();
}
