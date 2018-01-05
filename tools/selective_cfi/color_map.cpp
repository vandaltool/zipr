
#include "color_map.hpp"

using namespace std;
using namespace libIRDB;


bool ColoredInstructionNonces_t::create()
{
	UniqueICFSSet_t unique_icfs;

	assert(firp);
	const ICFSSet_t& all_icfs=firp->GetAllICFS();
	for(ICFSSet_t::iterator it=all_icfs.begin(); it!=all_icfs.end(); ++it)
	{
		ICFS_t* p=*it;
		assert(p);
		unique_icfs.insert( *p );
	}

	ColoredSlotValue_t v;
	for(UniqueICFSSet_t::iterator it=unique_icfs.begin(); it!=unique_icfs.end(); ++it)
	{
		const ICFS_t& the_icfs=*it;

		for(int slot_no=0; /* loop until break */ ; slot_no++)
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
			for(ICFS_t::iterator it2=the_icfs.begin(); it2!=the_icfs.end(); ++it2)
			{
				Instruction_t* target=*it2;
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
				    << " for "<<hex<<target->GetBaseID()<<":"<<target->getDisassembly()
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
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin(); it!=firp->GetInstructions().end(); ++it)
	{
		Instruction_t* insn=*it;
		if(insn->GetIBTargets())
		{
			v=GetColorOfIB(insn);
			cout<<"IB assigned " <<"slot[-"<<v.GetPosition()<<"]=color["<<hex<<v.GetNonceValue()<<dec<<"]"
			    << " for "<<insn->GetBaseID()<<":"<<insn->getDisassembly() << endl;

			used_icfs.insert(*insn->GetIBTargets());
	
		}
	}

	cout<<"# ATTRIBUTE scfi::Unique_Used_ICFS_size="<<dec<<used_icfs.size()<<endl;
	cout<<"# ATTRIBUTE scfi::Unique_ICFS_size="<<dec<<unique_icfs.size()<<endl;
#endif

	// output stats
	cout<<"# ATTRIBUTE scfi::slots_used="<<slots_used.size()<<endl;
	int total_slots = 0;
	for(int slot_no=0; slot_no<slots_used.size(); slot_no++)
	{
		cout<<"# scfi::ATTRIBUTE used_slot"<<slot_no<<"="<<slots_used[slot_no].SlotsUsed()<<endl;
		total_slots += slots_used[slot_no].SlotsUsed();
	}
	cout<<"# ATTRIBUTE scfi::total_slots="<<total_slots<<endl;
	cout<<"# ATTRIBUTE scfi::pct_slots_used="<<std::fixed<<((float)slots_used.size()/(float)total_slots)*100.00<<"%"<<endl;

	return true;
}

