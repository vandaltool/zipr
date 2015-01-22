#ifndef irdb_cfg_callgraph_h
#define irdb_cfg_callgraph_h



typedef libIRDB::Function_t* CallGraphNode_t;
typedef std::pair<CallGraphNode_t,CallGraphNode_t> CallGraphEdge_t;
typedef std::set<CallGraphNode_t> CallGraphNodeSet_t;
typedef libIRDB::Instruction_t* CallSite_t;
typedef std::set<CallSite_t> CallSiteSet_t;



class Callgraph_t
{
	public:
		Callgraph_t();
		void AddFile(libIRDB::FileIR_t *firp);

		bool EdgeExists(const CallGraphEdge_t& edge)
			{
				CallGraphNode_t from=edge.first;
				CallGraphNode_t to=edge.second;
				
				// note that this callers[from] may allocate an empty set -- probably can do better.
				const CallGraphNodeSet_t &calleeSet=callers[from];	
				return calleeSet.find(to)!=calleeSet.end();
			}
		bool EdgeExists(const CallGraphNode_t& n1, const CallGraphNode_t& n2) 
			{ return EdgeExists(CallGraphEdge_t(n1,n2));}

		CallGraphNodeSet_t& GetCallersOfNode(const CallGraphNode_t& node)
			{ return callers[node]; }
		CallGraphNodeSet_t& GetCalleesOfNode(const CallGraphNode_t& node)
			{ return callees[node]; }
		
		void GetAncestors(const CallGraphNode_t& node, CallGraphNodeSet_t& ancestors, bool skipHellNode = false);

		CallSiteSet_t& GetCallSites(const CallGraphNode_t& n1)
			{ return call_sites[n1]; }

		void Dump(std::ostream& fout);

		std::string GetNodeName(const CallGraphNode_t& n1) const
			{ return n1 ? n1->GetName() : "HELLNODE"; }

		std::string GetCallsiteDisassembly(const CallSite_t &c) const
			{ return c ? c->getDisassembly() : "NOFROMFUNC"; } 

		bool Reachable(const CallGraphNode_t& from, const CallGraphNode_t &to, bool skipHellNode = false);

	private:
		// mark the given insn as a call site.
		void MarkCallSite(Instruction_t* insn);

		// traverse graph to retrieve all ancestors
		void _GetAncestors(const CallGraphNode_t& node, CallGraphNodeSet_t &ancestors, CallGraphNodeSet_t &visited, bool skipHellNode);

		typedef	std::map<CallGraphNode_t, CallGraphNodeSet_t > CGNodeToCGNodeSetMap_t;
		typedef std::map<CallGraphNode_t, CallSiteSet_t > NodeToCallSiteSetMap_t;

		CGNodeToCGNodeSetMap_t callers; // map a callee to its callees
		CGNodeToCGNodeSetMap_t callees; // map a caller to its callers
		NodeToCallSiteSetMap_t call_sites;
};

#endif
