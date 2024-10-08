
#include <map>
#include <set>
#include <memory>
#include <deep.hpp>
#include <loops.hpp>
#include <MEDS_DeadRegAnnotation.hpp>
#include <MEDS_MemoryRangeAnnotation.hpp>
#include <MEDS_SafeFuncAnnotation.hpp>
#include <MEDS_LoopAnnotation.hpp>


using namespace libIRDB;
using namespace std;
using namespace MEDS_Annotation;


StarsDeepAnalysis_t::StarsDeepAnalysis_t(IRDB_SDK::FileIR_t* firp, const vector<string>& options)
	: Transform_t(firp)
{
	for(const auto s : options)
	{
		if(s=="SetDeepLoopAnalyses=true")
			stars_analysis_engine.GetSTARSOptions().SetDeepLoopAnalyses(true);
		else if(s=="SetConstantPropagation=true")
			stars_analysis_engine.GetSTARSOptions().SetConstantPropagation(true);
		else
			throw invalid_argument("Unknown option: "+s);
	}
	stars_analysis_engine.do_STARS(getFileIR());
}

unique_ptr<IRDB_SDK::FunctionSet_t> StarsDeepAnalysis_t::getLeafFunctions()      const 
{
	auto ret=unique_ptr<IRDB_SDK::FunctionSet_t>(new IRDB_SDK::FunctionSet_t);
	auto &ret_map=*ret;
	auto meds_ap=stars_analysis_engine.getAnnotations();

	for(auto func : getFileIR()->getFunctions())
	{
		auto the_range = meds_ap.getFuncAnnotations().equal_range(func->getName());
	        /* for each annotation for this instruction */
       		for (auto it = the_range.first; it != the_range.second; ++it)
		{
		 	const auto p_annotation=dynamic_cast<MEDS_SafeFuncAnnotation*>(it->second);
			if(p_annotation==nullptr)
                                continue;

                        /* bad annotation? */
                        if(!p_annotation->isLeaf())
                                continue;

                        /* record dead reg set */
                        ret_map.insert(func);
		}
        }

	return ret;
}


unique_ptr<IRDB_SDK::DeadRegisterMap_t> StarsDeepAnalysis_t::getDeadRegisters()      const 
{
	auto ret=unique_ptr<IRDB_SDK::DeadRegisterMap_t>(new IRDB_SDK::DeadRegisterMap_t);
	auto &ret_map=*ret;
	auto meds_ap=stars_analysis_engine.getAnnotations();
	for(auto insn : getFileIR()->getInstructions())
	{
		auto the_range = meds_ap.getAnnotations().equal_range(insn->getBaseID());
	        /* for each annotation for this instruction */
       		 for (auto it = the_range.first; it != the_range.second; ++it)
		 {
			 auto p_annotation=dynamic_cast<MEDS_DeadRegAnnotation*>(it->second);
			if(p_annotation==nullptr)
                                continue;

                        /* bad annotation? */
                        if(!p_annotation->isValid())
                                continue;

                        /* record dead reg set */
                        ret_map[insn] =  p_annotation->getRegisterSet();
		 }
        }

	return ret;
}

unique_ptr<IRDB_SDK::StaticGlobalStartMap_t> StarsDeepAnalysis_t::getStaticGlobalRanges() const 
{
	auto ret=unique_ptr<IRDB_SDK::StaticGlobalStartMap_t>(new IRDB_SDK::StaticGlobalStartMap_t());
	auto &ret_map=*ret;
	auto meds_ap=stars_analysis_engine.getAnnotations();
	for(auto insn : getFileIR()->getInstructions())
	{
		auto the_range = meds_ap.getAnnotations().equal_range(insn->getBaseID());
	        /* for each annotation for this instruction */
       		 for (auto it = the_range.first; it != the_range.second; ++it)
		 {
		 	auto p_annotation=dynamic_cast<MEDS_MemoryRangeAnnotation*>(it->second);
			if(p_annotation==nullptr)
                                continue;

                        /* bad annotation? */
                        if(!p_annotation->isValid())
                                continue;

			if(!p_annotation->isStaticGlobalRange())
				continue;

			// record the range minimum. 
                        ret_map[insn] =  p_annotation->getRangeMin();
		 }
        }

	return ret;
}

unique_ptr<IRDB_SDK::RangeSentinelSet_t> StarsDeepAnalysis_t::getRangeSentinels()      const 
{
	auto ret=unique_ptr<IRDB_SDK::RangeSentinelSet_t>(new IRDB_SDK::RangeSentinelSet_t);
	auto meds_ap=stars_analysis_engine.getAnnotations();
	for(auto insn : getFileIR()->getInstructions())
	{
		auto the_range = meds_ap.getAnnotations().equal_range(insn->getBaseID());
	        /* for each annotation for this instruction */
       		 for (auto it = the_range.first; it != the_range.second; ++it)
		 {
		 	auto p_annotation=dynamic_cast<MEDS_MemoryRangeAnnotation*>(it->second);
			if(p_annotation==nullptr)
                                continue;

                        /* bad annotation? */
                        if(!p_annotation->isValid())
                                continue;

			if(!p_annotation->isSentinel())
				continue;

			/* record sentinal marker. */
			ret->insert(insn);
		 }
        }
	return ret;
}

unique_ptr<IRDB_SDK::LoopNest_t> StarsDeepAnalysis_t::getLoops(IRDB_SDK::Function_t* f) const
{
	auto cfg = IRDB_SDK::ControlFlowGraph_t::factory(f);
	auto ret = getLoops(cfg.get());
	auto real_nest = dynamic_cast<libIRDB::LoopNest_t*>(ret.get()); 
	real_nest->saveCFG(move(cfg));

	return ret;
}

unique_ptr<IRDB_SDK::LoopNest_t> StarsDeepAnalysis_t::getLoops(IRDB_SDK::ControlFlowGraph_t* cfg) const 
{
	auto func = cfg->getFunction();
	auto id_to_block_map = map<IRDB_SDK::DatabaseID_t, IRDB_SDK::BasicBlock_t*>();
	for(auto blk : cfg->getBlocks())
		id_to_block_map[blk->getInstructions()[0]->getBaseID()]=blk;

	auto meds_ap=stars_analysis_engine.getAnnotations();
	const auto the_range = meds_ap.getAnnotations().equal_range(func->getBaseID());

	auto ret = unique_ptr<IRDB_SDK::LoopNest_t>(new LoopNest_t(cfg));
	for (auto it = the_range.first; it != the_range.second; ++it)
	{
		auto p_annotation=dynamic_cast<MEDS_LoopAnnotation*>(it->second);
		if(p_annotation==nullptr)    continue;
		if(!p_annotation->isValid()) continue;

		const auto header_id = p_annotation -> getHeaderID();
		const auto loop_id   = p_annotation -> getLoopID();
		IRDB_SDK::BasicBlockSet_t loop_blocks;
		for (auto block_id : p_annotation->getBlockIDs()) {
			loop_blocks.insert(id_to_block_map[block_id]);
		}
		auto the_loop = unique_ptr<Loop_t>(new Loop_t(id_to_block_map[header_id], loop_blocks));
		dynamic_cast<libIRDB::LoopNest_t*>(ret.get())->addLoop(loop_id,move(the_loop));
	}

	return move(ret);
}

unique_ptr<IRDB_SDK::DeepAnalysis_t> IRDB_SDK::DeepAnalysis_t::factory(FileIR_t* firp, const AnalysisEngine_t& ae, const vector<string>& options)
{
	auto ret=unique_ptr<IRDB_SDK::DeepAnalysis_t>();
		
	switch(ae)
	{
		case IRDB_SDK::aeSTARS:
		{
			ret.reset(new libIRDB::StarsDeepAnalysis_t(firp,options));
			break;
		}
		default:
		{
			throw std::invalid_argument("AnalysisEngine_t value not supported");
		}

	}
	return ret;
}




