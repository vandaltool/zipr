/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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

#ifndef irdb_cfg_callgraph_h
#define irdb_cfg_callgraph_h

namespace libIRDB
{

	using namespace std;

	// only one type of hell node for now, but leave
	// open the possibilty for multiple hell nodes
	typedef enum { DEFAULT_HELL_NODE = 0 } HellNodeType; 

	class CallGraphNode_t : public IRDB_SDK::CallGraphNode_t
	{
		public:
			virtual ~CallGraphNode_t(){ }
			CallGraphNode_t(IRDB_SDK::Function_t* f = NULL) {
				if (f) {
					SetFunction(f);
				} else {
					m_isHellNode = true;
					m_node.hellNodeType = DEFAULT_HELL_NODE;
				}
			}

			bool isHellnode() const { return m_isHellNode; }

			IRDB_SDK::Function_t* getFunction() const {
				assert(!m_isHellNode);
				if (m_isHellNode) 
					return NULL;					
				else
					return m_node.func;
			}

			HellNodeType GetHellNodeType() const {
				assert(m_isHellNode);
				return m_node.hellNodeType;
			}

			bool operator==(const CallGraphNode_t &other) const {
				if (this == &other) return true;
				if (m_isHellNode)
				{
					return m_node.hellNodeType == other.m_node.hellNodeType;
				}
				else
					return m_node.func == other.m_node.func;
			}
			void dump(ostream& os=cout) const;

		private:
			void SetFunction(IRDB_SDK::Function_t* const f) {
				assert(f);
				m_node.func = f;
				m_isHellNode = false;
			}

		private:
			bool m_isHellNode;
			union {
				IRDB_SDK::Function_t* func;
				HellNodeType hellNodeType;
			} m_node;
	};


	class Callgraph_t : public IRDB_SDK::CallGraph_t
	{
		public:
			Callgraph_t();
			virtual ~Callgraph_t();
			void addFile(IRDB_SDK::FileIR_t* const firp);

			bool edgeExists(const IRDB_SDK::CallGraphEdge_t& edge) const
			{
				const auto from = dynamic_cast<CallGraphNode_t*>(edge.first);
				const auto to   = dynamic_cast<CallGraphNode_t*>(edge.second);
					
				const auto caller_it  = callers.find(from);
				if(caller_it==callers.end())
					return false;

				const auto &calleeSet = caller_it->second;
				return calleeSet.find(to)!=calleeSet.end();
			}
			bool edgeExists(IRDB_SDK::CallGraphNode_t& n1, IRDB_SDK::CallGraphNode_t& n2)  const
				{ return edgeExists(IRDB_SDK::CallGraphEdge_t(&n1,&n2));}

			IRDB_SDK::CallGraphNodeSet_t& GetCallersOfNode(IRDB_SDK::CallGraphNode_t* const node)
				{ return callers[node]; }
			IRDB_SDK::CallGraphNodeSet_t& GetCalleesOfNode(IRDB_SDK::CallGraphNode_t* const node)
				{ return callees[node]; }

			const IRDB_SDK::CallGraphNodeSet_t& getCallersOfNode(IRDB_SDK::CallGraphNode_t* const node) const
			{ 
				static const auto empty = IRDB_SDK::CallGraphNodeSet_t();
				const auto it=callers.find(node);
				return (it==callers.end()) ?  empty : it->second;
			}
			const IRDB_SDK::CallGraphNodeSet_t& getCalleesOfNode(IRDB_SDK::CallGraphNode_t* const node) const
			{ 
				static const auto empty = IRDB_SDK::CallGraphNodeSet_t();
				const auto it=callees.find(node);
				return (it==callees.end()) ?  empty : it->second;
			}
			
			void GetAncestors(IRDB_SDK::Function_t* const fn, IRDB_SDK::CallGraphNodeSet_t& ancestors, bool skipHellNode = false) const;
			void GetAncestors(IRDB_SDK::CallGraphNode_t* const node, IRDB_SDK::CallGraphNodeSet_t& ancestors, bool skipHellNode = false) const;

			const IRDB_SDK::CallSiteSet_t& GetCallSites(IRDB_SDK::CallGraphNode_t* const n1) const
			{ 
				static const auto empty = IRDB_SDK::CallSiteSet_t();
				const auto it=call_sites.find(n1);
				return (it==call_sites.end()) ?  empty : it->second;
			}
			

			IRDB_SDK::CallSiteSet_t& getCallSites(IRDB_SDK::CallGraphNode_t* const n1)
				{ return call_sites[n1]; }

			void dump(ostream& fout) const;

			string GetNodeName(const IRDB_SDK::CallGraphNode_t* const p_n1) const {
				auto n1=dynamic_cast<CallGraphNode_t const*>(p_n1);
				assert(n1);
				if (n1->isHellnode()) {
					ostringstream s;
					s << "HELLNODE" << n1->GetHellNodeType();
					return s.str();
				} else {
					assert(n1->getFunction());
					return n1->getFunction()->getName(); 
				}
			}

			string GetCallsiteDisassembly(const IRDB_SDK::CallSite_t &c) const
				{ return c ? c->getDisassembly() : "NOFROMFUNC"; } 

			bool isReachable(IRDB_SDK::CallGraphNode_t* const from, IRDB_SDK::CallGraphNode_t* const to, bool skipHellNode = false) const;

			CallGraphNode_t& GetDefaultHellNode() { return default_hellnode; }
			const CallGraphNode_t& getDefaultHellNode() const { return default_hellnode; }

			IRDB_SDK::CallGraphNode_t* findNode(IRDB_SDK::Function_t* const fn) const;

		private:
			// create nodes from functions
			void CreateNodes(IRDB_SDK::FileIR_t *firp);

			// mark the given insn as a call site.
			void MarkCallSite(IRDB_SDK::Instruction_t* const insn);

			// traverse graph to retrieve all ancestors
			void _GetAncestors(IRDB_SDK::CallGraphNode_t* const node, IRDB_SDK::CallGraphNodeSet_t &ancestors, IRDB_SDK::CallGraphNodeSet_t &visited, bool skipHellNode) const;

			using CGNodeToCGNodeSetMap_t = map<IRDB_SDK::CallGraphNode_t*, IRDB_SDK::CallGraphNodeSet_t > ;
			using NodeToCallSiteSetMap_t = map<IRDB_SDK::CallGraphNode_t*, IRDB_SDK::CallSiteSet_t      > ;
			using FunctionToCGNodeMap_t  = map<IRDB_SDK::Function_t*     , IRDB_SDK::CallGraphNode_t*   > ;
;
			CGNodeToCGNodeSetMap_t callers;     // map a callee to its callees
			CGNodeToCGNodeSetMap_t callees;     // map a caller to its callers
			NodeToCallSiteSetMap_t call_sites;
			CallGraphNode_t default_hellnode;   // default hell node
			FunctionToCGNodeMap_t nodes;        // maps functions to call graph nodes
	};

}
#endif
