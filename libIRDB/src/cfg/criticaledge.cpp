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

#define ALLOF(a) begin(a),end(a)

CriticalEdgeAnalyzer_t::CriticalEdgeAnalyzer_t(const ControlFlowGraph_t* p_cfg, const bool p_conservative) :
	m_cfg(p_cfg),
	m_conservative(p_conservative)
{
	init();
}

/*
*   Critical edge between two nodes is where the source node has multiple successsors,
*   and the target node has multiple predecessors 
*/
void CriticalEdgeAnalyzer_t::init()
{
	for (const auto &src : m_cfg->getBlocks())
	{
		auto num_successors = src->getSuccessors().size();
		if (!m_conservative)
		{
			// in aggressive (non conservative) mode, ignore indirect edges
			// when counting number of successors
			num_successors = count_if
				(
					ALLOF(src->getSuccessors()),
					[&] (const IRDB_SDK::BasicBlock_t* bb_tgt) 
					{
						auto myEdgeType = m_cfg->getEdgeType(src, bb_tgt);
						return myEdgeType.find(IRDB_SDK::cetTargetEdge)!=myEdgeType.end() || 
						       myEdgeType.find(IRDB_SDK::cetFallthroughEdge)!=myEdgeType.end();
					}
				);
		}

		if (num_successors <= 1) continue;

		for (const auto &tgt : src->getSuccessors())
		{
			auto num_predecessors = tgt->getPredecessors().size();
			if (!m_conservative)
			{
				// in aggressive (non conservative) mode, ignore indirect edges
				// when counting number of predecessors
				num_predecessors = count_if
					(
						ALLOF(tgt->getPredecessors()),
						[&] (const IRDB_SDK::BasicBlock_t* bb_pred) 
						{
							auto myEdgeType = m_cfg->getEdgeType(bb_pred, tgt);
							return myEdgeType.find(IRDB_SDK::cetTargetEdge)!=myEdgeType.end() || 
							       myEdgeType.find(IRDB_SDK::cetFallthroughEdge)!=myEdgeType.end();
						}
					);
			}

			if (num_predecessors > 1)
			{
				auto e=IRDB_SDK::BasicBlockEdge_t(src, tgt);
				criticals.insert(e);
			}
		}
	}
}

unique_ptr<IRDB_SDK::CriticalEdges_t> IRDB_SDK::CriticalEdges_t::factory(
	const IRDB_SDK::ControlFlowGraph_t &p_cfg, 
	const bool p_conservative)
{
	auto real_cfg = dynamic_cast<const libIRDB::ControlFlowGraph_t*>(&p_cfg);
	return unique_ptr<IRDB_SDK::CriticalEdges_t>(new libIRDB::CriticalEdgeAnalyzer_t(
				real_cfg, p_conservative));
}
