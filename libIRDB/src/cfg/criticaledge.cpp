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

CriticalEdgeAnalyzer_t::CriticalEdgeAnalyzer_t(const ControlFlowGraph_t& p_cfg) :
	m_cfg(p_cfg)
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
		for (const auto &tgt : src->GetSuccessors())
		{
			if (src->GetSuccessors().size() > 1 && tgt->GetPredecessors().size() > 1)
			{
				BasicBlockEdge_t e(src, tgt);
				criticals.insert(e);
			}
		}
	}
	return criticals;
}



