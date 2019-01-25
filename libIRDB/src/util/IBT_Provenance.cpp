#include <map>
#include <bitset>
#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;


Provenance_t IBTProvenance_t::empty;


void IBTProvenance_t::AddFile(const IRDB_SDK::FileIR_t* firp)
{

        using ICFSProvMap_t =  std::map<const IRDB_SDK::ICFS_t*, Provenance_t>;

	auto icfs_prov_map = ICFSProvMap_t(); 

	// collect before info for each icfs into icfs_prov_map
	for(auto insn : firp->getInstructions())
	{
		const auto &ibTargets=insn->getIBTargets();
		if(!ibTargets)
			continue;

		auto this_prov=Provenance_t();
		const auto p_IndBranchAsm=DecodedInstruction_t::factory(insn);
		const auto &IndBranchAsm=*p_IndBranchAsm;
		const auto isIndJmp = IndBranchAsm.isUnconditionalBranch() && !IndBranchAsm.getOperand(0)->isConstant();
		const auto isIndCall = IndBranchAsm.isCall() && !IndBranchAsm.getOperand(0)->isConstant();
		const auto isRet = IndBranchAsm.isReturn();

		if(isIndJmp)
		{
			this_prov.addIndirectJump();
		}
		else if(isIndCall)
		{
			this_prov.addIndirectCall();
		}
		else if(isRet)
		{
			this_prov.addReturn();
		}
		else
		{
			assert(0);
		}

		icfs_prov_map[ibTargets].addProv(this_prov);
	}

	// deploy info for each target of the icfs
	for(const auto &icfs : firp->getAllICFS())
	{
		assert(icfs);
		for(const auto &insn : *icfs)
		{
			prov_map[insn].addProv(icfs_prov_map[icfs]);
		}
	}
}

