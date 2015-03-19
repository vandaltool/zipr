
#include <map>
#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;



void InstructionPredecessors_t::AddPred(const Instruction_t* before, const Instruction_t* after)
{
	assert(before);
	if(!after) return;

	if(getenv("DUMP_PRED_CREATE"))
		cout<<"Found "<<after->GetBaseID()<<":"<<after->getDisassembly() << " follows "<< before->GetBaseID()<<":"<<before->getDisassembly()<<endl;
	pred_map[after].insert((Instruction_t*)before);
	
}

void InstructionPredecessors_t::AddFile(const FileIR_t* firp2)
{
	FileIR_t* firp=(FileIR_t*)firp2; // discarding const qualifier because we know we won't change the set
	for(InstructionSet_t::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		Instruction_t* insn=*it;
		AddPred(insn, insn->GetTarget());
		AddPred(insn, insn->GetFallthrough());
	}

}
