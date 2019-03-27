#include <zipr_all.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <fstream>

namespace zipr
{
#include <pinner/pinner_x86.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

static int ceildiv(int a, int b)
{
        return  (a+b-1)/b;
}

#define ALLOF(a) begin(a),end(a)

ZiprPinnerX86_t::ZiprPinnerX86_t(Zipr_SDK::Zipr_t* p_parent) : 
	m_parent(dynamic_cast<zipr::ZiprImpl_t*>(p_parent)),	 // upcast to ZiprImpl
	memory_space(*p_parent->getMemorySpace()),
	m_dollop_mgr(*p_parent->getDollopManager()),
	m_firp(p_parent->getFileIR()),
	placement_queue(*p_parent->getPlacementQueue()),
	m_verbose(false), // fixme
	m_stats(m_parent->getStats()),
	final_insn_locations(*p_parent->getLocationMap())
{
		
}

void  ZiprPinnerX86_t::doPinning()
{
	// Initial creation of the set of pinned instructions.
	AddPinnedInstructions();

	// Reserve space for pins.
	ReservePinnedInstructions();

	// Emit instruction immediately?

	//TODO: Reenable after option parsing is fixed.
#if 0
	if (m_opts.IsEnabledOptimization(Optimizations_t::OptimizationFallthroughPinned))
	{
		OptimizePinnedFallthroughs();
	}
#endif

	PreReserve2ByteJumpTargets();

	// expand 2-byte pins into 5-byte pins
	ExpandPinnedInstructions();

	while (!two_byte_pins.empty()) 
	{
		/*
		 * Put down the five byte targets
		 * for two byte jumps, if any exist.
		 */
		printf("Going to Fix2BytePinnedInstructions.\n");
		Fix2BytePinnedInstructions();

		/*
		 * If there are still two byte pins, 
		 * try the dance again.
		 */
		if (!two_byte_pins.empty())
		{
			printf("Going to Re PreReserve2ByteJumpTargets.\n");
			PreReserve2ByteJumpTargets();
		}
	}

	// Convert all 5-byte pins into full fragments
	OptimizePinnedInstructions();
}


void ZiprPinnerX86_t::AddPinnedInstructions()
{
	// find the big chunk of free memory in case we need it for unassigned pins.
	VirtualOffset_t next_pin_addr=memory_space.getInfiniteFreeRange().getStart();


	/*
	 * Start out by recording the pinned address into a map
	 * for use by other functions.
	 */
	RecordPinnedInsnAddrs();

	for(auto insn : m_firp->getInstructions())
	{
		assert(insn);

		if(insn->getIndirectBranchTargetAddress()==nullptr)
			continue;

		if(insn->getIndirectBranchTargetAddress()->getVirtualOffset()==0)
                {
                        // Unpinned IBT. Create dollop and add it to placement
                        // queue straight away--there are no pinning considerations.
                        auto newDoll=m_dollop_mgr.addNewDollops(insn);
			placement_queue.insert({newDoll, 0});
			continue;
                }

		// deal with unassigned IBTAs.
		if(insn->getIndirectBranchTargetAddress()->getVirtualOffset()==0)
		{
			insn->getIndirectBranchTargetAddress()->setVirtualOffset(next_pin_addr);
			next_pin_addr+=5;// sizeof pin
		}

		unresolved_pinned_addrs.insert({insn});
	}
}

void ZiprPinnerX86_t::RecordPinnedInsnAddrs()
{
	for(
		set<Instruction_t*>::const_iterator it=m_firp->getInstructions().begin();
		it!=m_firp->getInstructions().end();
		++it
		)
	{
		RangeAddress_t ibta_addr;
		Instruction_t* insn=*it;
		assert(insn);

		if(!insn->getIndirectBranchTargetAddress()
		   || insn->getIndirectBranchTargetAddress()->getVirtualOffset()==0) 
		{
			continue;
		}
		ibta_addr=(RangeAddress_t)insn->
		                          getIndirectBranchTargetAddress()->
		                          getVirtualOffset();
		/*
		* Record the size of thing that we are pinning.
		* We are going to use this information for doing
		* sleds, if we have to.
		*
		* There are two different possibilities: 
		*
		* 1. The instruction is turned into a jump. By default,
		* we assume that turns into a two byte relative jump.
		* This information will *not* be used in the case that
		* this pin is overriden by a patch. In other words, when
		* this pinned address is handled in ExpandPinnedInstructions()
		* then it might be expanded into a five byter. However,
		* when we deal this in the context of putting down a sled,
		* we check for patches before falling back to this method.
		*
		*
		* 2. The instruction cannot be turned into a jump -- it
		* must be pinned immediately. In that case, we want to
		* record the size of the instruction itself.
		*/
		if (ShouldPinImmediately(insn))
			m_InsnSizeAtAddrs[ibta_addr]=std::pair<Instruction_t*, size_t>(insn,insn->getDataBits().length());
		else					 
			m_InsnSizeAtAddrs[ibta_addr]=std::pair<Instruction_t*, size_t>(insn,2);
	}
}


bool ZiprPinnerX86_t::ShouldPinImmediately(Instruction_t *upinsn)
{
	auto d=DecodedInstruction_t::factory (upinsn);
	Instruction_t *pin_at_next_byte = nullptr;
	AddressID_t *upinsn_ibta = nullptr, *ft_ibta = nullptr;

	if(d->isReturn() )
		return true;

	upinsn_ibta=upinsn->getIndirectBranchTargetAddress();
	assert(upinsn_ibta!=nullptr && upinsn_ibta->getVirtualOffset()!=0);

	if (upinsn->getFallthrough() != nullptr)
		ft_ibta=upinsn->getFallthrough()->getIndirectBranchTargetAddress();

	/* careful with 1 byte instructions that have a pinned fallthrough */ 
	const auto len=upinsn->getDataBits().length();
	if(len==1)
	{
		if(upinsn->getFallthrough()==nullptr)
			return true;
		ft_ibta=upinsn->getFallthrough()->getIndirectBranchTargetAddress();
		if((ft_ibta && ft_ibta->getVirtualOffset()!=0) && (upinsn_ibta->getVirtualOffset()+1) == ft_ibta->getVirtualOffset())
			return true;
	}


	// look for a pinned ib 
	if(upinsn->getFallthrough()==nullptr && upinsn->getIBTargets()!=nullptr)
	{
		// check the bytes that follow to make sure we can fit it.
		auto found=false;
		for(auto i = 1u; i < len; i++)
		{

			const auto ibt_at=FindPinnedInsnAtAddr(upinsn_ibta->getVirtualOffset() + i);
			if(ibt_at)
				found=true;
		}

		// if the whole range is clear, we can pin the ib.
		if(!found)
			return true;
	}




	// find the insn pinned at the next byte.
	pin_at_next_byte = FindPinnedInsnAtAddr(upinsn_ibta->getVirtualOffset() + 1);
	if ( pin_at_next_byte && 

	/* upinsn has lock prefix */
		upinsn->getDataBits()[0]==(char)(0xF0) 	&&
	/*
	 * upinsn:  lock cmpxchange op1 op2 [pinned at x]
	 *          x    x+1        x+2 x+3
	 * 
	 * AND pin_at_next_byte (x+1) is:
	 */
		pin_at_next_byte->getDataBits() == upinsn->getDataBits().substr(1,upinsn->getDataBits().length()-1) &&  
	/*
         *               cmpxchange op1 op2 [pinned at x+1]
	 *               x+1        x+2 x+3
	 * AND  pin_at_next_byte->fallthrough() == upinsn->Fallthrough()
	 */
		pin_at_next_byte->getFallthrough() == upinsn->getFallthrough() ) 
	/*
	 * x should become nop, put down immediately
	 * x+1 should become the entire lock command.
	 */
	{
		if (m_verbose)
			cout<<"Using pin_at_next_byte special case, addrs="<<
				upinsn_ibta->getVirtualOffset()<<","<<
				pin_at_next_byte->getAddress()->getVirtualOffset()<<endl;
		/*
		 * Because upinsn is longer than 
		 * 1 byte, we must be somehow
		 * pinned into ourselves. Fix!
		 */

		/*
		 * Make pin_at_next_byte look like upinsn.
		 */
		pin_at_next_byte->setDataBits(upinsn->getDataBits());
		pin_at_next_byte->setComment(upinsn->getComment());
		pin_at_next_byte->setCallback(upinsn->getCallback());
		pin_at_next_byte->setFallthrough(upinsn->getFallthrough());
		pin_at_next_byte->setTarget(upinsn->getTarget());
		/*
		 * Convert upins to nop.
		 */
		string dataBits = upinsn->getDataBits();
		dataBits.resize(1);
		dataBits[0] = 0x90;
		upinsn->setDataBits(dataBits);

		return true;
	}
	return false;
}

void ZiprPinnerX86_t::PreReserve2ByteJumpTargets()
{
	bool repeat = false;

	do
	{
		repeat = false;
		for(set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
			it!=two_byte_pins.end();
			)
		{
			UnresolvedPinned_t up=*it;
			bool found_close_target = false;
			Instruction_t* upinsn=up.getInstrution();

			RangeAddress_t addr;
			
			if (up.HasUpdatedAddress())
			{
				addr = up.GetUpdatedAddress();
			}
			else
			{
				addr=upinsn->getIndirectBranchTargetAddress()->getVirtualOffset();
			}
			
			if (m_AddrInSled[addr])
			{
				/*
				 * There is no need to consider this pin at all! It was
				 * subsumed w/in a sled which has already handled it.
				 * Move along.
				 */
				if (m_verbose)
					cout << "Two byte pin at 0x" 
					     << std::hex << addr 
							 << " is w/in a sled ... skipping and erasing." << endl;
				two_byte_pins.erase(it++);
				continue;
			}

			/*
			 * Check for near branch instructions
			 * by starting far away!
			 * Note: two byte jump range is 127 bytes, 
			 * but that's from the pc after it's been 
			 * inc, etc. complicated goo. 120 is a 
			 * safe estimate of range.
			 */
			for(int size=5;size>0;size-=3) 
			{

				//if (m_verbose)
				//	printf("Looking for %d-byte jump targets to pre-reserve.\n", size);
				for(int i=120;i>=-120;i--)
				{
					if(memory_space.areBytesFree(addr+i,size))
					{
						if (m_verbose)
							printf("Found location for 2-byte->%d-byte conversion "
							"(%p-%p)->(%p-%p) (orig: %p)\n", 
							size,
							(void*)addr,
							(void*)(addr+1),
							(void*)(addr+i),
							(void*)(addr+i+size),
							(upinsn->getIndirectBranchTargetAddress() != nullptr) ?
							(void*)(uintptr_t)upinsn->getIndirectBranchTargetAddress()->getVirtualOffset() : 0x0);

						up.SetRange(Range_t(addr+i, addr+i+size));
						for (auto j = up.GetRange().getStart(); j<up.GetRange().getEnd(); j++)
						{
							memory_space.splitFreeRange(j);
						}

						/*
						 * We add chain entries early, as soon as the patch is 
						 * prereserved. When we use it, we don't have to worry about it
						 * already being written as a patch. However, if we don't use it,
						 * we will need to remove it. See ExpandPinnedInstructions() for
						 * the place where we do the removal.
						 *
						 * addr: place of prereserved memory
						 * size: size of the amount of prereserved memory
						 */
						UnresolvedUnpinned_t uu(up);
						Patch_t patch(up.GetRange().getStart(),
						              UnresolvedType_t::UncondJump_rel32);
						if (size == 2)
							patch.setType(UnresolvedType_t::UncondJump_rel8);
						UnresolvedUnpinnedPatch_t uup(uu, patch);

						if (m_verbose)
							cout << "Adding a chain entry at address "
							     << std::hex << up.GetRange().getStart() << endl;
						m_parent->RecordNewPatch(std::pair<RangeAddress_t, UnresolvedUnpinnedPatch_t>(up.GetRange().getStart(),uup));

						found_close_target = true;
						break;
					}
				}
				if (found_close_target)
					break;
			}

			if (!found_close_target)
			{
				/*
				 * In the case that we did not find a nearby
				 * space for placing a 2/5 jmp, we are going
				 * to fall back to using a sled.
				 */
				
				/*
				 * Algorithm:
				 * 1. Insert the sled at addr and record where it ends (end_addr).
				 * 2. For each pinned instruction, i, between addr and end_addr:
				 *    a. If i exists in two_byte_pins indicating that it will
				 *       be handled, we remove it.
				 *    b. If i has prereserved space associated with it, we
				 *       we clear that space.
				 */
				cout<<"Warning: No location for near jump reserved at 0x"<<hex<<addr<<"."<<endl;

				/*
				 * The first thing that we have to do is to tell
				 * ourselves that this is a chain entry.
				 */
				if (m_verbose)
					cout << "Added emergency patch entry at "
					     << std::hex << addr << endl;
				Patch_t emergency_patch(addr, UnresolvedType_t::UncondJump_rel8);
				UnresolvedUnpinned_t uu(up);
				UnresolvedUnpinnedPatch_t uup(uu, emergency_patch);
				m_parent->RecordNewPatch(std::pair<RangeAddress_t, UnresolvedUnpinnedPatch_t>(addr,uup));

				RangeAddress_t end_of_sled = Do68Sled(addr);

				m_firp->assembleRegistry();
				for (RangeAddress_t i = addr; i<end_of_sled; i++)
				{
					Instruction_t *found_pinned_insn = nullptr;
					found_pinned_insn = FindPinnedInsnAtAddr(i);
					if (found_pinned_insn)
					{
						/*
						 * TODO: Continue from here. Continue implementing the above
						 * algorithm. Don't forget to handle the case that Jason was
						 * explaining where a range of bytes in this sled may actually
						 * contain a two byte jmp that points to the ultimate
						 * five byte jump that ultimately terminates the chain.
						 */
					}
				}
				++it;
			}
			else
			{
				UnresolvedPinned_t new_up = UnresolvedPinned_t(up.getInstrution(), up.GetRange());
				if (up.HasUpdatedAddress())
				{
					new_up.SetUpdatedAddress(up.GetUpdatedAddress());
				}
				two_byte_pins.erase(it++);
				two_byte_pins.insert(new_up);
					
			}
		}
	} while (repeat);
}

void ZiprPinnerX86_t::InsertJumpPoints68SledArea(Sled_t &sled)
{
	for (RangeAddress_t addr = sled.SledRange().getStart();
	     addr < sled.SledRange().getEnd();
			 addr++)
	{
		bool is_pin_point = false, is_patch_point = false;
		/*
		 * There is the possibility that the sled is being put here
		 * because we have a pin that cannot be properly handled.
		 */
		is_pin_point = (nullptr != FindPinnedInsnAtAddr(addr));
		if (m_verbose && is_pin_point)
			cout << "There is a pin at 0x" << std::hex << addr
			     << " inside a sled." << endl;

		/*
		 * There is the possibility that the sled is being put here
		 * because we have a chain entry that cannot properly be 
		 * handled.
		 */
		is_patch_point = FindPatchTargetAtAddr(addr);
		if (is_patch_point)
		{
			
			cout << "There is a patch at 0x"
			     << std::hex << addr << " inside a sled." << endl;
		}

		if (is_pin_point || is_patch_point)
		{
			if (m_verbose)
				cout << "Adding Jump Point at 0x" << std::hex << addr << endl;
			sled.AddJumpPoint(addr);
		}
	}
}

Instruction_t* ZiprPinnerX86_t::Emit68Sled(RangeAddress_t addr, Sled_t sled, Instruction_t* next_sled)
{
	Instruction_t *sled_start_insn = nullptr;
	auto sled_number = addr - sled.SledRange().getStart();
	size_t sled_size = sled.SledRange().getEnd() - sled.SledRange().getStart();

	sled_start_insn = FindPinnedInsnAtAddr(addr);
	if (!sled_start_insn)
		sled_start_insn = FindPatchTargetAtAddr(addr);
	assert(sled_start_insn != nullptr);	

	if (m_verbose)
		cout << "Begin emitting the 68 sled @ " << addr << "." << endl;

	const uint32_t push_lookup[]={0x68686868,
	                              0x90686868,
	                              0x90906868,
	                              0x90909068,
	                              0x90909090};
	const int number_of_pushed_values=ceildiv(sled_size-sled_number, 5);
	vector<uint32_t> pushed_values(number_of_pushed_values, 0x68686868);

	// first pushed value is variable depending on the sled's index
	pushed_values[0] = push_lookup[4-((sled_size-sled_number-1)%5)];

	/* 
	 * Emit something that looks like:
	 * 	if ( *(tos+0*stack_push_size)!=pushed_values[0] )
	 * 			jmp next_sled; //missed
	 * 	if ( *(tos+1*stack_push_size)!=pushed_values[1] )
	 * 			jmp next_sled; //missed
	 * 		...
	 * 	if ( *(tos+number_of_pushed_values*stack_push_size-1)!=pushed_values[number_of_pushed_values-1] )
	 * 			jmp next_sled; //missed
	 * 	lea rsp, [rsp+push_size]
	 * 	jmp dollop's translation	// found
	*/

	string stack_reg="rsp";
	string decoration="qword";
	if(m_firp->getArchitectureBitWidth()!=64)
	{
		decoration="dword";
		stack_reg="esp";
	}
	const int stack_push_size=m_firp->getArchitectureBitWidth()/8;

	string lea_string=string("lea ")+stack_reg+", ["+stack_reg+"+" + to_string(stack_push_size*number_of_pushed_values)+"]"; 
	Instruction_t *lea=addNewAssembly(m_firp, nullptr, lea_string);
	lea->setFallthrough(sled_start_insn);

	Instruction_t *old_cmp=lea;

	for(int i=0;i<number_of_pushed_values;i++)
	{
		string cmp_str="cmp "+decoration+" ["+stack_reg+"+ "+to_string(i*stack_push_size)+"], "+to_string(pushed_values[i]);
		Instruction_t* cmp=addNewAssembly(m_firp, nullptr, cmp_str); 
		Instruction_t *jne=addNewAssembly(m_firp, nullptr, "jne 0"); 
		cmp->setFallthrough(jne);
		jne->setTarget(next_sled);
		jne->setFallthrough(old_cmp);

		cout<<"Adding 68-sled bit:  "+cmp_str+", jne 0 for sled at 0x"<<hex<<addr<<" entry="<<dec<<sled_number<<endl;

		old_cmp=cmp;
	}

	/*
	 * now that all the cmp/jmp's are inserted, we are done with this sled.
	 */
	return old_cmp;
}

Instruction_t* ZiprPinnerX86_t::Emit68Sled(Sled_t sled)// RangeAddress_t addr, int sled_size)
{

	Instruction_t *top_of_sled=addNewAssembly(m_firp, nullptr, "hlt"); 

	for (std::set<RangeAddress_t>::reverse_iterator
	     addr_iter=sled.JumpPointsReverseBegin();
	     addr_iter != sled.JumpPointsReverseEnd();
			 addr_iter++)
	{
		RangeAddress_t addr = *addr_iter;
		if (m_verbose)
			cout << "Specific Emit68Sled(" 
			     << std::hex << addr << ","
			     << sled << ","
			     << std::hex << top_of_sled << ");" << endl;

		top_of_sled=Emit68Sled(addr, sled, top_of_sled);
	}
	return top_of_sled;
}

/*
 * Put the new sled into the existing sled.
 * and do some other things.
 * Note that the clearable sled is just the difference
 * between the new and the old. We do not 
 * need to clear out any of the existing sled 
 * since it is just PUSHs at this point!
 */
void ZiprPinnerX86_t::Update68Sled(Sled_t new_sled, Sled_t &existing_sled)
{
	Range_t clearable_sled_range(new_sled.SledRange().getStart(),
	                             existing_sled.SledRange().getStart());
	Sled_t clearable_sled(memory_space, clearable_sled_range);

	if (m_verbose)
		cout << "Updating sled: " << existing_sled
		     << " with new sled: " << new_sled << endl;

	/*
	 * Put the jump points into the new sled area.
	 */
	InsertJumpPoints68SledArea(new_sled);

	cout << "Clearable sled: " << clearable_sled << endl;
	clearable_sled.MergeSledJumpPoints(new_sled);
	cout << "(Merged) Clearable sled: " << clearable_sled << endl;
	/*
	 * Clear the chains in the new sled!
	 */
	Clear68SledArea(clearable_sled);

	/*
	 * Put in PUSHs in the new_sled.
	 */
	size_t i=0;
	RangeAddress_t addr=new_sled.SledRange().getStart();
	for(;i<existing_sled.SledRange().getStart()-new_sled.SledRange().getStart();
	    i++)
	{
		if (m_verbose)
			cout << "Adding 68 at "
			     << std::hex << addr+i 
					 << " for sled at 0x"
					 << std::hex << addr << endl;
		
		/*
		 * Do not assert that we are writing into a free space.
		 * We may be writing over a PUSH that was there before!
		 */
		assert(memory_space.isByteFree(addr+i) || memory_space[addr+i]==0x68);
		memory_space[addr+i] = 0x68;
		m_AddrInSled[addr+i] = true;
		memory_space.splitFreeRange(addr+i);
	}

	existing_sled.MergeSled(new_sled);

	assert(existing_sled.Disambiguation());

	Instruction_t *sled_disambiguation = Emit68Sled(existing_sled);

	if (m_verbose)
		cout << "Generated sled_disambiguation (in Update68Sled()): " << std::hex << sled_disambiguation << endl;
	/*
	 * Update the disambiguation
	 *
	 * What we are doing is walking through the
	 * expanded or unexpanded pins and seeing 
	 * if they match the end instruction that
	 * we are doing now. We updated the end
	 * instruction in the MergeSled(). Why is it
	 * that this might fail?
	 * 
	 * TODO: This is really bad slow.
	 */
	
	Instruction_t *disambiguations[2] = {existing_sled.Disambiguation(),
	                                     new_sled.Disambiguation()};
	bool disambiguation_updated = false;	
	for (int disambiguation_iter = 0; 
	     disambiguation_iter<2;
			 disambiguation_iter++)
	{
		Instruction_t *disambiguation_to_update =
		               disambiguations[disambiguation_iter];
		/*
		 * The pin pointing to the disambiguation is only ever a 5 byte pin.
		 */
		for(
			std::map<UnresolvedPinned_t,RangeAddress_t>::iterator it=five_byte_pins.begin();
				it!=five_byte_pins.end() /*&& !sled_disambiguation*/;
				it++
		   )
		{
			RangeAddress_t addr=(*it).second;
			UnresolvedPinned_t up=(*it).first;

			cout << std::hex << up.getInstrution() << " 5b vs " << disambiguation_to_update << endl;
			if (up.getInstrution() == disambiguation_to_update)
			{
				five_byte_pins.erase(it);
				UnresolvedPinned_t cup(sled_disambiguation);
				cup.SetUpdatedAddress(up.GetUpdatedAddress());
				five_byte_pins[cup] = addr;

				disambiguation_updated = true;
				break;
			}
		}
	}
	assert(disambiguation_updated);

	existing_sled.Disambiguation(sled_disambiguation);
}

RangeAddress_t ZiprPinnerX86_t::Do68Sled(RangeAddress_t addr)
{
	char jmp_rel32_bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0};
	const size_t nop_overhead=4;	// space for nops.
	const size_t jmp_overhead=sizeof(jmp_rel32_bytes);	// space for nops.
	const size_t sled_overhead = nop_overhead + jmp_overhead;
	const int sled_size=Calc68SledSize(addr, sled_overhead);
	Sled_t sled(memory_space, Range_t(addr,addr+sled_size), m_verbose);
	set<Sled_t>::iterator sled_it;

	if (m_verbose)
		cout << "Adding 68-sled at 0x" << std::hex << addr 
		     << " size="<< std::dec << sled_size << endl;
	
	for (sled_it = m_sleds.begin();
	     sled_it != m_sleds.end();
			 sled_it++)
	{
		Sled_t sled_i = *sled_it;
		if (sled_i.Overlaps(sled))
		{
			if (m_verbose)
				cout << "Found overlapping sled: " << sled_i << " and " << sled << endl;

			m_sleds.erase(sled_it);
			Update68Sled(sled, sled_i);
			m_sleds.insert(sled_i);
			/*
			 * Return the final address of the updated sled.
			 */
			return sled_i.SledRange().getEnd();
		}
	}


	InsertJumpPoints68SledArea(sled);

	/*
	 * It's possible that the sled that we are going to put here
	 * is actually going to overwrite some pins and chain entries.
	 * So, we have to make sure to unreserve that space.
	 */
	Clear68SledArea(sled);

	/* Now, let's (speculatively) clear out the overhead space.
	 */
	if (m_verbose)
		cout << "Clearing overhead space at " << std::hex
		     << "(" << addr+sled_size << "."
		     << addr+sled_size+sled_overhead << ")." << endl;
	for (size_t i=0;i<sled_overhead;i++)
		if (!memory_space.isByteFree(addr+sled_size+i))
			memory_space.mergeFreeRange(addr+sled_size+i);

	/*
	 * Put down the sled.
	 */
	for(auto i=0;i<sled_size;i++)
	{
		if (m_verbose)
			cout << "Adding 68 at "
			     << std::hex << addr+i 
					 << " for sled at 0x"
					 << std::hex << addr << endl;
		assert(memory_space.isByteFree(addr+i));
		memory_space[addr+i]=0x68;
		m_AddrInSled[addr+i] = true;
		memory_space.splitFreeRange(addr+i);
	}
	/*
	 * Put down the NOPs
	 */
	for(size_t i=0;i<nop_overhead;i++)
	{
		if (m_verbose)
			cout << "Adding 90 at "
			     << std::hex << addr+sled_size+i 
					 << " for sled at 0x"
					 << std::hex << addr << endl;

		assert(memory_space.isByteFree(addr+sled_size+i));
		memory_space[addr+sled_size+i] = 0x90;
		m_AddrInSled[addr+sled_size+i] = true;
		memory_space.splitFreeRange(addr+sled_size+i);
	}

	/*
	 * That brings us to the part that actually, you know, does
	 * the jump to the proper target depending upon where we
	 * landed.
	 */
	Instruction_t* sled_disambiguation=Emit68Sled(sled);

	if (m_verbose)
		cout << "Generated sled_disambiguation (in Do68Sled()): " << std::hex << sled_disambiguation << endl;

	if (m_verbose)
		cout << "Pin for 68-sled  at 0x"
		     << std::hex << addr <<" is "
				 << std::hex << (addr+sled_size+nop_overhead) << endl;

	/*
	 * Reserve the bytes for the jump at the end of the sled that
	 * will take us to the (above) disambiguator.
	 */
	for(size_t i=0;i<jmp_overhead;i++)
	{
		assert(memory_space.isByteFree(addr+sled_size+nop_overhead+i));
		memory_space[addr+sled_size+nop_overhead+i]=jmp_rel32_bytes[i];
		memory_space.splitFreeRange(addr+sled_size+nop_overhead+i);
		//m_AddrInSled[addr+sled_size+nop_overhead+i] = true;
	}

	/*
	 * We know that the jmp we just put down is a two byte jump.
	 * We want it to point to the sled_disambiguation so we
	 * put a two byte pin down. This will affect the work of the
	 * loop above where we are putting down two byte pins and 
	 * attempting to expand them through chaining.
	 */
	UnresolvedPinned_t cup(sled_disambiguation);
	cup.SetUpdatedAddress(addr+sled_size+nop_overhead);
	five_byte_pins[cup] = addr+sled_size+nop_overhead;
	if (m_verbose)
		cout << "Put in a five byte jmp to the disambiguation " << std::hex << sled_disambiguation << " at " << std::hex << addr+sled_size+nop_overhead << endl;

	sled.Disambiguation(sled_disambiguation);

	if (m_verbose)
		cout << "Inserting sled: " << sled << endl;
	m_sleds.insert(sled);

	return addr+sled_size+nop_overhead+jmp_overhead;
}


void ZiprPinnerX86_t::Clear68SledArea(Sled_t sled)
{
	for (std::set<RangeAddress_t>::iterator addr_iter = sled.JumpPointsBegin();
	     addr_iter != sled.JumpPointsEnd();
			 addr_iter++)
	{
		RangeAddress_t addr = *addr_iter;
		size_t clear_size = 0;
		if (m_verbose)
			cout << "Testing " << std::hex << addr << endl;
		if (!(sled.SledRange().getStart() <= addr && addr <= sled.SledRange().getEnd()))
		{	
			if (m_verbose)
				cout << std::hex << addr << " outside sled range." << endl;
			continue;
		}
		if (FindPatchTargetAtAddr(addr))
		{
			UnresolvedUnpinnedPatch_t uup = m_parent->FindPatch(addr);
			clear_size = uup.second.getSize();

			if (m_verbose)
				cout << "Need to clear a " << std::dec << clear_size
				     << " byte chain entry at " << std::hex << (addr) << endl;
		}
		else if (FindPinnedInsnAtAddr(addr))
		{
			std::map<RangeAddress_t, std::pair<IRDB_SDK::Instruction_t*, size_t> >
			   ::iterator pinned_it = m_InsnSizeAtAddrs.find(addr);

			assert(pinned_it != m_InsnSizeAtAddrs.end());


			clear_size = pinned_it->second.second;

			if (m_verbose)
				cout << "Need to clear a " << std::dec << clear_size
				     << " byte pin at " << std::hex << (addr) << endl;
		}
		else
			assert(false);

		clear_size = std::min(sled.SledRange().getEnd(), addr+clear_size) - addr;
		if (m_verbose)
			cout << "Need to clear " << std::dec << clear_size << " bytes." << endl;

		if (clear_size>0)
		{
			/*
			 * We do want to free this space, but only if it
			 * is already in use.
			 */
			if (!memory_space.isByteFree(addr))
				memory_space.mergeFreeRange(Range_t(addr, addr+clear_size));
			assert(memory_space.isByteFree(addr));
		}
	}
}

int ZiprPinnerX86_t::Calc68SledSize(RangeAddress_t addr, size_t sled_overhead)
{
	int sled_size=0;
	while(true)
	{
		auto i=(size_t)0;
		for(i=0;i<sled_overhead;i++)
		{
			if (FindPinnedInsnAtAddr(addr+sled_size+i))
			{
				if (m_verbose)
					cout << "Sled free space broken up by pin at " 
					     << std::hex << (addr+sled_size+i) << endl;
				break;
			}
			else if (FindPatchTargetAtAddr(addr+sled_size+i))
			{
				if (m_verbose)
					cout << "Sled free space broken up by chain entry at " 
					     << std::hex << (addr+sled_size+i) << endl;
				break;
			}
			else
			{
				if (m_verbose)
					cout << "Sled free space at " << std::hex << (addr+sled_size+i) << endl;
			}
		}
		// if i==sled_overhead, that means that we found 6 bytes in a row free
		// in the previous loop.  Thus, we can end the 68 sled.
		// if i<sled_overhead, we found a in-use byte, and the sled must continue.
		if(i==sled_overhead)
		{
			assert(sled_size>2);
			return sled_size;
		}

		// try a sled that's 1 bigger.
		sled_size+=(i+1);
	}

	// cannot reach here?
	assert(0);

}

bool ZiprPinnerX86_t::IsPinFreeZone(RangeAddress_t addr, int size)
{
	for(int i=0;i<size;i++)
		if(FindPinnedInsnAtAddr(addr+i)!=nullptr)
			return false;
	return true;
}



void ZiprPinnerX86_t::ReservePinnedInstructions()
{
	set<UnresolvedPinned_t> reserved_pins;


	/* first, for each pinned instruction, try to 
	 * put down a jump for the pinned instruction
 	 */
	for( auto it=unresolved_pinned_addrs.begin(); it!=unresolved_pinned_addrs.end(); ++it)
	{
		char bytes[]={(char)0xeb,(char)0}; // jmp rel8
		UnresolvedPinned_t up=*it;
		const auto upinsn=up.getInstrution();
		auto addr=upinsn->getIndirectBranchTargetAddress()->getVirtualOffset();

		if(upinsn->getIndirectBranchTargetAddress()->getFileID() == BaseObj_t::NOT_IN_DATABASE)
			continue;

		/* sometimes, we need can't just put down a 2-byte jump into the old slot
	   	 * we may need to do alter our technique if there are two consecutive pinned addresses (e.g. 800 and 801).
		 * That case is tricky, as we can't put even a 2-byte jump instruction down. 
		 * so, we attempt to pin any 1-byte instructions with no fallthrough (returns are most common) immediately.
		 * We may also attempt to pin any 1-byte insn that falls through to the next pinned address (nops are common).
		 * We may also pin multi-byte instructions that don't fall through.
		 */
		if(ShouldPinImmediately(upinsn))
		{
			if (m_verbose)
				cout << "Final pinning " << hex << addr << "-" << (addr+upinsn->getDataBits().size()-1)  << endl;

			for(auto i=0u;i<upinsn->getDataBits().size();i++)
			{
				memory_space[addr+i]=upinsn->getDataBits()[i];
				memory_space.splitFreeRange(addr+i);
				m_stats->total_other_space++;
			}

			// record the final location of this instruction.
			final_insn_locations[upinsn] = addr;

			// create a dollop for this instruction and place it.
			auto dollop=m_dollop_mgr.addNewDollops(upinsn);
			assert(dollop);
			dollop->Place(addr);

			auto place_addr=addr;
			for(auto de : *dollop)
			{
				de->Place(place_addr);
				place_addr+=sizeof(de->getInstruction()->getDataBits().size());
			}

			continue;
		}

		if (m_verbose) 
		{
			printf("Working two byte pinning decision at %p for: ", (void*)addr);
			printf("%s\n", upinsn->getComment().c_str());
		}


		// if the byte at x+1 is free, we can try a 2-byte jump (which may end up being converted to a 5-byte jump later).
		if (FindPinnedInsnAtAddr(addr+1)==nullptr)
		{
			/* so common it's not worth printing 
			if (m_verbose)
			{
				printf("Can fit two-byte pin (%p-%p).  fid=%d\n", 
					(void*)addr,
					(void*)(addr+sizeof(bytes)-1),
					upinsn->getAddress()->getFileID());
			}
			*/
		
			/*
			 * Assert that the space is free.  We already checked that it should be 
			 * with the FindPinnedInsnAtAddr, but just to be safe.
			 */
			for(auto i=0u;i<sizeof(bytes);i++)
			{
				assert(memory_space.find(addr+i) == memory_space.end() );
				memory_space[addr+i]=bytes[i];
				memory_space.splitFreeRange(addr+i);
			}
			// insert the 2-byte pin to be patched later.
			up.SetRange(Range_t(addr, addr+2));
			two_byte_pins.insert(up);
		}
		// this is the case where there are two+ pinned bytes in a row start.
		// check and implement the 2-in-a-row test
		// The way this work is to put down this instruction:
		// 68  --opcode for push 4-byte immed (addr+0)
		// ww				      (addr+1)
		// xx				      (addr+2)
		// yy				      (addr+3)
		// zz				      (addr+4)
		// jmp L1		              (addr+5 to addr+6)
		// ...
		// L1: lea rsp, [rsp+8]
		//     jmp dollop(addr)
		// where ww,xx are un-specified here (later, they will become a 2-byte jump for the pin at addr+1, which will 
		// be handled in other parts of the code.)  However, at a minimum, the bytes for the jmp l1 need to be free
		// and there is little flexibility on the 68 byte, which specifies that ww-zz are an operand to the push.
		// Thus, the jump is at a fixed location.   So, bytes addr+5 and addr+6 must be free.  Also, for the sake of simplicity,
		// we will check that xx, yy and zz are free so that later handling of addr+1 is uncomplicated.
		// This technique is refered to as a "push disambiguator" or sometimes a "push sled" for short.
		else if (IsPinFreeZone(addr+2,5)) 
		{
			if (m_verbose)
				printf("Cannot fit two byte pin; Using 2-in-a-row workaround.\n");
			/*
			 * The whole workaround pattern is:
			 * 0x68 0xXX 0xXX 0xXX 0xXX (push imm)
			 * lea rsp, rsp-8
			 * 0xeb 0xXX (jmp)
			 * 
			 * We put the lea into the irdb and then
			 * put down a pin with that as the target.
			 * We put the original instruction as 
			 * the fallthrough for the lea.
			 */
			char push_bytes[]={(char)0x68,(char)0x00, /* We do not actually write */
					   (char)0x00,(char)0x00, /* all these bytes but they */
					   (char)0x00};           /* make counting easier (see*/
					   		          /* below). */
			Instruction_t *lea_insn = nullptr;

			if(m_firp->getArchitectureBitWidth()==64)
				lea_insn = addNewAssembly(m_firp, nullptr, "lea rsp, [rsp+8]");
			else
				lea_insn = addNewAssembly(m_firp, nullptr, "lea esp, [esp+4]");

			m_firp->assembleRegistry();
			lea_insn->setFallthrough(upinsn);

			/*
			 * Write the push opcode.
			 * Do NOT reserve any of the bytes in the imm value
			 * since those are going to contain the two byte pin
			 * to the adjacent pinned address.
			 */
			memory_space[addr] = push_bytes[0];
			memory_space.splitFreeRange(addr);

			addr += sizeof(push_bytes);

			// reserve the bytes for the jump at the end of the push.
			for(auto i=0u;i<sizeof(bytes);i++)
			{
				assert(memory_space.find(addr+i) == memory_space.end() );
				memory_space[addr+i]=bytes[i];
				memory_space.splitFreeRange(addr+i);
			}

			if (m_verbose)
				printf("Advanced addr to %p\n", (void*)addr);

			/*
			 * Insert a new UnresolvePinned_t that tells future
			 * loops that we are going to use an updated address
			 * to place this instruction.
			 * 
			 * This is a type of fiction because these won't really
			 * be pins in the strict sense. But, it's close enough.
			 */
			UnresolvedPinned_t cup(lea_insn);
			cup.SetUpdatedAddress(addr);
			two_byte_pins.insert(cup);
		} 
		// If, those bytes aren't free, we will default to a "68 sled".
		// the main concept for a 68 sled is that all bytes will be 68 until we get to an opening where we can "nop out" of 
		// the sled, re-sync the instruction stream, and inspect the stack to see what happened.  Here is an example with 7 pins in a row.
		// 0x8000: 68
		// 0x8001: 68
		// 0x8002: 68
		// 0x8003: 68
		// 0x8004: 68
		// 0x8005: 68
		// 0x8006: 68
		// 0x8007: 90
		// 0x8008: 90
		// 0x8009: 90
		// 0x800a: 90
		// <resync stream>:  at this point regardless of where (between 0x8000-0x8006 the program transfered control,
		// execution will resynchronize.  For example, if we jump to 0x8000, the stream will be 
		//	push 68686868
		//	push 68909090
		//	nop
		//	<resync>
		// But if we jump to  0x8006, our stream will be:
		// 	push 90909090
		// 	<resync>
		// Note that the top of stack will contain 68909090,68686868 if we jumped to 0x8000, but 0x90909090 if we jumped to 0x8006
		// After we resync, we have to inspect the TOS elements to see which instruction we jumped to.
		else if (FindPinnedInsnAtAddr(addr+1))
		{
			const auto end_of_sled=Do68Sled(addr);

			// skip over some entries until we get passed the sled.
			while (true)
			{
				// get this entry
				const auto up=*it;
				const auto upinsn=up.getInstrution();
				auto addr=upinsn->getIndirectBranchTargetAddress() ->getVirtualOffset();

				// is the entry within the sled?
				if(addr>=end_of_sled)
					// nope, skip out of this while loop
					break;
				// inc the iterator so the for loop will continue at the right place.
				++it;

				/*
				 * It's possible that this pin is going to be the last 
				 * one in the program. TODO: If this instruction abuts the 
				 * end of the program's address space then there 
				 * could be a problem. As of now, we assume that 
				 * this is not the case.
				 */
				if (it==unresolved_pinned_addrs.end())
					break;
			}
			// back up one, because the last one still needs to be processed.
			--it;

			// resolve any new instructions added for the sled.
			m_firp->assembleRegistry();
		}
		else
			assert(0); // impossible to reach, right?
	}
}

void ZiprPinnerX86_t::ExpandPinnedInstructions()
{
	/* now, all insns have 2-byte pins.  See which ones we can make 5-byte pins */
	
	for(
		set<UnresolvedPinned_t>::iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.getInstrution();
		RangeAddress_t addr=0;

		/*
		 * This is possible if we moved the address
		 * forward because we had consecutive pinned
		 * instructions and had to apply the workaround.
		 */
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr = upinsn->getIndirectBranchTargetAddress()->getVirtualOffset();
		}

		if (m_AddrInSled[addr])
		{
			/*
			 * There is no need to consider this pin at all! It was
			 * subsumed w/in a sled which has already handled it.
			 * Move along.
			 */
			if (m_verbose)
				cout << "Two byte pin at 0x" 
				     << std::hex << addr 
						 << " is w/in a sled ... skipping and erasing." << endl;
			two_byte_pins.erase(it++);
			continue;
		}

		char bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0}; // jmp rel8
		bool can_update=memory_space.areBytesFree(addr+2,sizeof(bytes)-2);
		if (m_verbose && can_update)
			printf("Found %p can be updated to 5-byte jmp\n", (void*)addr);

		can_update &= !m_AddrInSled[up.GetRange().getStart()];
		if (m_verbose && can_update && m_AddrInSled[up.GetRange().getStart()])
			printf("%p was already fixed into a sled. Cannot update.\n", (void*)addr);
		if(can_update)
		{
			memory_space.plopJump(addr);

			/*
			 * Unreserve those bytes that we reserved before!
			 */
			for (auto j = up.GetRange().getStart(); j<up.GetRange().getEnd(); j++)
			{
				if (!m_AddrInSled[j])
					memory_space.mergeFreeRange(j);
			}
		
			/*
			 * We have a chain entry prereserved and we want to get
			 * rid of it now!
			 */
			if (m_verbose)
				cout << "Erasing chain entry at 0x" 
				     << std::hex << up.GetRange().getStart() << endl;
			m_parent->RemovePatch(up.GetRange().getStart());


			up.SetRange(Range_t(0,0));
			five_byte_pins[up]=addr;
			m_InsnSizeAtAddrs[addr] = std::pair<Instruction_t*, size_t>(
			                          m_InsnSizeAtAddrs[addr].first, 5);
			two_byte_pins.erase(it++);
			m_stats->total_5byte_pins++;
			m_stats->total_trampolines++;
		}
		else
		{
			++it;
			if (m_verbose)
				printf("Found %p can NOT be updated to 5-byte jmp\n", (void*)addr);
			m_stats->total_2byte_pins++;
			m_stats->total_trampolines++;
			m_stats->total_tramp_space+=2;
		}
	}

	printf("Totals:  2-byters=%d, 5-byters=%d\n", (int)two_byte_pins.size(), (int)five_byte_pins.size());
}


void ZiprPinnerX86_t::Fix2BytePinnedInstructions()
{
	for(
		set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.getInstrution();
		RangeAddress_t addr;
		
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr=upinsn->getIndirectBranchTargetAddress()->getVirtualOffset();
		}

		/*
		 * This might have already been handled in a sled.
		 */
		if (m_AddrInSled[addr])
		{
			if (m_verbose)
				cout << "Skipping two byte pin at " 
				     << std::hex << addr << " because it is in a sled." << endl;
			/*
			 * If there are some reserved bytes for this then
			 * we want to unreserve it (but only if it is not
			 * in a sled itself since it would not be good to
			 * imply that space is now open).
			 */
			if (up.HasRange())
			{
				for (auto j = up.GetRange().getStart(); j<up.GetRange().getEnd(); j++)
				{
					if (!m_AddrInSled[j])
						memory_space.mergeFreeRange(j);
				}
			}
			two_byte_pins.erase(it++);
			continue;
		}

		if (up.HasRange())
		{
			/*
			 * Always clear out the previously reserved space.
			 * Do this here because some/most of the algorithms
			 * that we use below assume that it is unreserved.
			 */
			for (auto j = up.GetRange().getStart(); j<up.GetRange().getEnd(); j++)
			{
				if (!m_AddrInSled[j])
					memory_space.mergeFreeRange(j);
			}

			if (m_AddrInSled[up.GetRange().getStart()])
			{
				if (m_verbose)
					printf("Using previously reserved spot of 2-byte->x-byte conversion "
					"(%p-%p)->(%p-%p) (orig: %p) because it was subsumed under sled\n", 
					(void*)addr,
					(void*)(addr+1),
					(void*)(up.GetRange().getStart()),
					(void*)(up.GetRange().getEnd()),
					(upinsn->getIndirectBranchTargetAddress() != nullptr) ?
					(void*)(uintptr_t)upinsn->getIndirectBranchTargetAddress()->getVirtualOffset() : 0x0);

				/*
				 * We simply patch the jump to this target and do not add
				 * it to any list for further processing. We are done.
				 */
				PatchJump(addr, up.GetRange().getStart());
				two_byte_pins.erase(it++);
			}
			else if (up.GetRange().is5ByteRange()) 
			{
				if (m_verbose)
					printf("Using previously reserved spot of 2-byte->5-byte conversion "
					"(%p-%p)->(%p-%p) (orig: %p)\n", 
					(void*)addr,
					(void*)(addr+1),
					(void*)(up.GetRange().getStart()),
					(void*)(up.GetRange().getEnd()),
					(upinsn->getIndirectBranchTargetAddress() != nullptr) ?
					(void*)(uintptr_t)upinsn->getIndirectBranchTargetAddress()->getVirtualOffset() : 0x0);

				five_byte_pins[up] = up.GetRange().getStart();
				memory_space.plopJump(up.GetRange().getStart());
				PatchJump(addr, up.GetRange().getStart());

				two_byte_pins.erase(it++);
			}
			else if (up.HasRange() && up.GetRange().is2ByteRange()) 
			{
				/*
				 * Add jump to the reserved space.
				 * Make an updated up that has a new
				 * "addr" so that addr is handled 
				 * correctly the next time through.
				 *
				 * Ie tell two_byte_pins list that
				 * the instruction is now at the jump
				 * target location.
				 */
				UnresolvedPinned_t new_up = 
					UnresolvedPinned_t(up.getInstrution());
				new_up.SetUpdatedAddress(up.GetRange().getStart());
				new_up.SetRange(up.GetRange());

				char bytes[]={(char)0xeb,(char)0}; // jmp rel8
				for(unsigned int i=0;i<sizeof(bytes);i++)
				{
					assert(memory_space.find(up.GetRange().getStart()+i) == memory_space.end() );
					memory_space[up.GetRange().getStart()+i]=bytes[i];
					memory_space.splitFreeRange(up.GetRange().getStart()+i);
					assert(!memory_space.isByteFree(up.GetRange().getStart()+i));
				}

				if (m_verbose)
					printf("Patching 2 byte to 2 byte: %p to %p (orig: %p)\n", 
					(void*)addr,
					(void*)up.GetRange().getStart(),
					(void*)(uintptr_t)upinsn->getIndirectBranchTargetAddress()->getVirtualOffset());

				PatchJump(addr, up.GetRange().getStart());

				two_byte_pins.erase(it++);
				two_byte_pins.insert(new_up);
			}
		}
		else
		{
			printf("FATAL: Two byte pin without reserved range: %p\n", (void*)addr);
			assert(false);
			it++;
		}
	}
}

void ZiprPinnerX86_t::OptimizePinnedInstructions()
{

	// should only be 5-byte pins by now.

	assert(two_byte_pins.size()==0);


	for(
		std::map<UnresolvedPinned_t,RangeAddress_t>::iterator it=five_byte_pins.begin();
			it!=five_byte_pins.end();
	   )
	{
		RangeAddress_t addr=(*it).second;
		UnresolvedPinned_t up=(*it).first;

		// ideally, we'll try to fill out the pinned 5-byte jump instructions with actual instructions
		// from the program.  That's an optimization.  At the moment, let's just create a patch for each one.

		if (m_AddrInSled[addr])
		{
			if (m_verbose)
				cout << "Skipping five byte pin at " 
				     << std::hex << addr << " because it is in a sled." << endl;
			it++;
			continue;
		}

		UnresolvedUnpinned_t uu(up.getInstrution());
		Patch_t	thepatch(addr,UncondJump_rel32);
		m_parent->AddPatch(uu,thepatch);
		memory_space.plopJump(addr);


		bool can_optimize=false; // fixme
		if(can_optimize)
		{
			//fixme
		}
		else
		{
			if (m_verbose)
			{
				//DISASM d;
				//Disassemble(uu.getInstrution(),d);
				auto d=DecodedInstruction_t::factory(uu.getInstrution());
				printf("Converting 5-byte pinned jump at %p-%p to patch to %d:%s\n", 
				       (void*)addr,(void*)(addr+4), uu.getInstrution()->getBaseID(), d->getDisassembly().c_str()/*.CompleteInstr*/);
			}
			m_stats->total_tramp_space+=5;
		}

		// remove and move to next pin
		five_byte_pins.erase(it++);
	}
		
}



Instruction_t *ZiprPinnerX86_t::FindPinnedInsnAtAddr(RangeAddress_t addr)
{
        std::map<RangeAddress_t,std::pair<IRDB_SDK::Instruction_t*, size_t> >::iterator it=m_InsnSizeAtAddrs.find(addr);
        if(it!=m_InsnSizeAtAddrs.end())
                return it->second.first;
        return nullptr;
}






