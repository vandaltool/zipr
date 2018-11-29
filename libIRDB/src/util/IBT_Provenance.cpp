#include <map>
#include <bitset>
#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;



void IBTProvenance_t::AddProvs(const Instruction_t* before, const InstructionSet_t& afterset)
{
        // Determine type of IB
        const auto IndBranchAsm=DecodedInstruction_t(before);
        const auto isIndJmp = IndBranchAsm.isUnconditionalBranch() && !IndBranchAsm.getOperand(0).isConstant();
        const auto isIndCall = IndBranchAsm.isCall() && !IndBranchAsm.getOperand(0).isConstant();
        const auto isRet = IndBranchAsm.isReturn();
        
        // Set the provenance info of targets depending on the type of IB 
	for(auto insn : afterset)
	{
		if(isIndJmp)
		{
			prov_map[insn].addIndirectJump();
		}
		else if(isIndCall)
		{
			prov_map[insn].addIndirectCall();
		}
		else if(isRet)
		{
			prov_map[insn].addReturn();
		}
		else
		{
			assert(0);
		}
	}
}

void IBTProvenance_t::AddFile(const FileIR_t* firp2)
{
	using TypeTuple_t=tuple<bool,bool,bool>;
	using Seen_t = pair<TypeTuple_t,ICFS_t*>;
	using SeenSet_t=set<Seen_t>;
	auto already_seen=SeenSet_t();
	FileIR_t* firp=(FileIR_t*)firp2; // discarding const qualifier because we know we won't change the set
        firp->AssembleRegistry(); // Takes time but I'm paranoid
	for(auto insn : firp->GetInstructions())
	{
		const auto &ibTargets=insn->GetIBTargets();
		if(!ibTargets)
			continue;

		const auto IndBranchAsm=DecodedInstruction_t(insn);
		const auto isIndJmp = IndBranchAsm.isUnconditionalBranch() && !IndBranchAsm.getOperand(0).isConstant();
		const auto isIndCall = IndBranchAsm.isCall() && !IndBranchAsm.getOperand(0).isConstant();
		const auto isRet = IndBranchAsm.isReturn();
		const auto the_tup=TypeTuple_t(isIndJmp,isIndCall,isRet);
		const auto to_be_seen=Seen_t(the_tup,ibTargets);
		

		// If insn is an IB, add the type of IB to the targets' provenance info

		const auto find_it=already_seen.find(to_be_seen);
	        if(find_it==already_seen.end()) 
        	      	AddProvs(insn, *ibTargets);
		already_seen.insert(to_be_seen);
	}
}

