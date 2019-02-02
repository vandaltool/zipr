
#include "color_map.hpp"

using namespace std;
using namespace IRDB_SDK;


bool ColoredInstructionNonces_t::create()
{
	UniqueICFSSet_t unique_icfs;

	assert(firp);
	const ICFSSet_t& all_icfs=firp->getAllICFS();
	for(ICFSSet_t::iterator it=all_icfs.begin(); it!=all_icfs.end(); ++it)
	{
		ICFS_t* p=*it;
		assert(p);
		unique_icfs.insert( *p );
	}

	ColoredSlotValue_t v;
	for(auto the_icfs : unique_icfs)
	{
		// const ICFS_t& the_icfs=*it;
		// const auto the_icfs=*it;

		for(auto slot_no=0U; /* loop until break */ ; slot_no++)
		{
			// check if we need to allocate a new slot
			if(slot_no<slots_used.size())
			{
				// skip any slots that are full.
				if(!slots_used[slot_no].CanReserve())
					goto next_slot;
			}
			else
			{
				// allocate slot
				slots_used.push_back(ColoredSlotAllocator_t(slot_no,slot_values));
			}

			// check if any of the targets for this branch already have a slot filled.
			for(auto target : the_icfs)
			{
				if(color_assignments[target][slot_no].IsValid())
					goto next_slot;
			}

			// if we get here, the slot is valid for all targets, we can reserve it.
			v=slots_used[slot_no].Reserve();

			// and record the reservation for each target.
			slot_assignments[the_icfs]=v;
			for(ICFS_t::iterator it2=the_icfs.begin(); it2!=the_icfs.end(); ++it2)
			{
				Instruction_t* target=*it2;
				color_assignments[target][slot_no]=v;
				cout<<"Setting slot[-"<<hex<<v.GetPosition()<<"]=color["<<hex<<v.GetNonceValue()<<dec<<"]"
				    << " for "<<hex<<target->getBaseID()<<":"<<target->getDisassembly()
				    << endl;
			}

			// and we're done with this ICFS
			break;


			// inc the slot no and try again.
			next_slot:
				// try next;
				; // neded statement for compiler to accept 
		}

	}

#if 1 /* debug code */
	UniqueICFSSet_t used_icfs;
	for(auto insn : firp->getInstructions())
	{
		if(insn->getIBTargets())
		{
			v=GetColorOfIB(insn);
			cout<<"IB assigned " <<"slot[-"<<v.GetPosition()<<"]=color["<<hex<<v.GetNonceValue()<<dec<<"]"
			    << " for "<<insn->getBaseID()<<":"<<insn->getDisassembly() << endl;

			used_icfs.insert(*insn->getIBTargets());
	
		}
	}

	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::Unique_Used_ICFS_size="<<dec<<used_icfs.size()<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::Unique_ICFS_size="<<dec<<unique_icfs.size()<<endl;
#endif

	// output stats
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::slots_used="<<slots_used.size()<<endl;
	int total_slots = 0;
	for(auto slot_no=0U; slot_no<slots_used.size(); slot_no++)
	{
		cout<<"# Selective_Control_Flow_Integrity::ATTRIBUTE used_slot"<<slot_no<<"="<<slots_used[slot_no].SlotsUsed()<<endl;
		total_slots += slots_used[slot_no].SlotsUsed();
	}
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::total_slots="<<total_slots<<endl;
	cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::pct_slots_used="<<std::fixed<<((float)slots_used.size()/(float)total_slots)*100.00<<"%"<<endl;

	return true;
}

