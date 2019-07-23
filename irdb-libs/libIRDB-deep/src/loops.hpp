/*
   Copyright 2018-2019 Zephyr Software, LLC.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include <irdb-core>
#include <irdb-cfg>
#include <irdb-deep>
#include <set>
#include <map>

namespace libIRDB
{
	using namespace std;
	
	class Loop_t : public IRDB_SDK::Loop_t 
	{
		public:
			Loop_t() = delete;
			Loop_t(IRDB_SDK::BasicBlock_t* header_blk) 
				:
				preheader(nullptr)
				{ 
					header = header_blk; 
					assert(header != nullptr);
				}
			Loop_t(const Loop_t& copy) = delete;
			virtual ~Loop_t() {} 

			virtual IRDB_SDK::BasicBlock_t*   getPreheader()   const override { assert(0); }
			virtual IRDB_SDK::BasicBlock_t*   getHeader()      const override { return header; }
			virtual IRDB_SDK::BasicBlockSet_t getAllBlocks()   const override { assert(0); }
			virtual IRDB_SDK::BasicBlockSet_t getOuterBlocks() const override { assert(0); }
			virtual IRDB_SDK::LoopSet_t       getInnerLoops()  const override { assert(0); }

			virtual bool isBlockInLoop       (const IRDB_SDK::BasicBlock_t* blk)  const override { assert(0); }
			virtual bool isBlockInInnerLoop  (const IRDB_SDK::BasicBlock_t* blk)  const override { assert(0); }
			virtual bool isBlockOuterLoopOnly(const IRDB_SDK::BasicBlock_t* blk)  const override { assert(0); }
		private:
			IRDB_SDK::BasicBlock_t*   preheader;
			IRDB_SDK::BasicBlock_t*   header;
			IRDB_SDK::BasicBlockSet_t blocks;
			IRDB_SDK::LoopSet_t       inner_loops;
	};

	class LoopNest_t : public IRDB_SDK::LoopNest_t
	{
		public:
			LoopNest_t(IRDB_SDK::ControlFlowGraph_t* p_cfg) 
				: 
					cfg(p_cfg) 
				{}

			LoopNest_t(const LoopNest_t& copy) = delete;
			virtual ~LoopNest_t() {}

			virtual IRDB_SDK::LoopSet_t getAllLoops()   const override ;
			virtual IRDB_SDK::LoopSet_t getOuterLoops() const override { assert(0); }

			virtual IRDB_SDK::Function_t*         getFunction()   const override { return cfg->getFunction(); }
			virtual IRDB_SDK::ControlFlowGraph_t* getCFG()        const override { return cfg;  };


			void addLoop(const uint64_t loop_id, unique_ptr<IRDB_SDK::Loop_t> loop_ptr) { loops[loop_id]=move(loop_ptr); } 
			void saveCFG(unique_ptr<IRDB_SDK::ControlFlowGraph_t> cfg_ptr             ) { saved_cfg = move(cfg_ptr); }
		private:
			unique_ptr<IRDB_SDK::ControlFlowGraph_t> saved_cfg;
			IRDB_SDK::ControlFlowGraph_t*            cfg;
			map<uint64_t,unique_ptr<IRDB_SDK::Loop_t> >        loops;


	};
}
