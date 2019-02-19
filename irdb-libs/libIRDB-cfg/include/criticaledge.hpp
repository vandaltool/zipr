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

namespace libIRDB
{
	using namespace std;

	class CriticalEdgeAnalyzer_t : public IRDB_SDK::CriticalEdges_t
	{
		public:
			CriticalEdgeAnalyzer_t(const ControlFlowGraph_t *p_cfg, const bool p_conservative=true);
			virtual ~CriticalEdgeAnalyzer_t() { }
			const IRDB_SDK::BasicBlockEdgeSet_t& getAllCriticalEdges() const { return criticals; }

		private:
			void init();
			const IRDB_SDK::ControlFlowGraph_t *m_cfg;
			const bool m_conservative;
		        IRDB_SDK::BasicBlockEdgeSet_t criticals;

	};
}
