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
        
        bool isIndJmp = IndBranchAsm.isUnconditionalBranch() && !IndBranchAsm.getOperand(0).isConstant();
        bool isIndCall = IndBranchAsm.isCall() && !IndBranchAsm.getOperand(0).isConstant();
        bool isRet = IndBranchAsm.isReturn();
        
        // Set the provenance info of targets depending on the type of IB
        IB_Type this_IB_type;
        
        if(isIndJmp)
        {
                this_IB_type = IB_Type::IndJmp;
        }
        else if(isIndCall)
        {
                this_IB_type = IB_Type::IndCall;
        }
        else if(isRet)
        {
                this_IB_type = IB_Type::Ret;
        }
        else
        {
                assert(0);
        }
        
	for(auto insn : afterset)
	{
                prov_map[insn].set((size_t) this_IB_type);
	}
}

void IBTProvenance_t::AddFile(const FileIR_t* firp2)
{
	FileIR_t* firp=(FileIR_t*)firp2; // discarding const qualifier because we know we won't change the set
        firp->AssembleRegistry(); // Takes time but I'm paranoid
	for(auto insn : firp->GetInstructions())
	{
		// If insn is an IB, add the type of IB to the targets' provenance info
	        if(insn->GetIBTargets())
        	      	AddProvs(insn, *insn->GetIBTargets());		
	}
}

