/*
 * Copyright (c) 2019 - Zephyr Software LLC
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


#include <libIRDB-cfg.hpp>
#include <utils.hpp>

using namespace std;
using namespace libIRDB;

CriticalEdgeAnalyzer_t::CriticalEdgeAnalyzer_t(const ControlFlowGraph_t& p_cfg, const bool p_conservative) :
	m_cfg(p_cfg),
	m_conservative(p_conservative)
{
}

/*
*   Critical edge between two nodes is where the source node has multiple successsors,
*   and the target node has multiple predecessors 
*/
BasicBlockEdgeSet_t CriticalEdgeAnalyzer_t::GetAllCriticalEdges() const
{
	BasicBlockEdgeSet_t criticals; 
	for (const auto &src : m_cfg.GetBlocks())
	{
		auto num_successors = src->GetSuccessors().size();
		auto num_successors_aggressive = num_successors;
		if (!m_conservative)
		{
			// recount but only use fallthrough or target edge
			// ignore indirect edges
			num_successors_aggressive = count_if(
				src->GetSuccessors().begin(), src->GetSuccessors().end(),
				[&] (const BasicBlock_t* bb_tgt) {
					CFG_EdgeType myEdgeType = m_cfg.GetEdgeType(src, bb_tgt);
					return myEdgeType.find(CFG_TargetEdge)!=myEdgeType.end() || 
					       myEdgeType.find(CFG_FallthroughEdge)!=myEdgeType.end();
					});

			cout << "aggressive mode: num_successors: " << num_successors << " num_aggressives: " << num_successors_aggressive << endl;
			num_successors = num_successors_aggressive;
		}

		if (num_successors <= 1) continue;

		for (const auto &tgt : src->GetSuccessors())
		{
			auto num_predecessors = tgt->GetPredecessors().size();
			auto num_predecessors_aggressive = num_predecessors;
			if (!m_conservative)
			{
				num_predecessors_aggressive = count_if(
					tgt->GetPredecessors().begin(), tgt->GetPredecessors().end(),
					[&] (const BasicBlock_t* bb_pred) {
						CFG_EdgeType myEdgeType = m_cfg.GetEdgeType(bb_pred, tgt);
						return myEdgeType.find(CFG_TargetEdge)!=myEdgeType.end() || 
					           myEdgeType.find(CFG_FallthroughEdge)!=myEdgeType.end();
						});

				cout << "aggressive mode: num_predecessors: " << num_predecessors << " num_aggressive: " << num_predecessors_aggressive << endl;
				num_predecessors = num_predecessors_aggressive;
			}

			if (num_predecessors > 1)
			{
				BasicBlockEdge_t e(src, tgt);
				criticals.insert(e);
			}
		}
	}
	return criticals;
}



