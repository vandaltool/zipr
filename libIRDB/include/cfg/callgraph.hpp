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
				
				// note that this callees[from] may allocate an empty set -- probably can do better.
				const CallGraphNodeSet_t &calleeSet=callees[from];	
				return calleeSet.find(to)!=calleeSet.end();
			}
		bool EdgeExists(const CallGraphNode_t& n1, const CallGraphNode_t& n2) 
			{ return EdgeExists(CallGraphEdge_t(n1,n2));}

		CallGraphNodeSet_t& GetCalleesOfCaller(const CallGraphNode_t& caller)
			{ return callees[caller]; }
		CallGraphNodeSet_t& GetCallersOfCallee(const CallGraphNode_t& callee)
			{ return callers[callee]; }

		CallSiteSet_t& GetCallSites(const CallGraphNode_t& n1)
			{ return call_sites[n1]; }

		void Dump(std::ostream& fout);

	private:
		// mark the given insn as a call site.
		void MarkCallSite(Instruction_t* insn);

		typedef	std::map<CallGraphNode_t, CallGraphNodeSet_t > CGNodeToCGNodeSetMap_t;
		typedef std::map<CallGraphNode_t, CallSiteSet_t > NodeToCallSiteSetMap_t;

		CGNodeToCGNodeSetMap_t callees; // map a callee to it's callers.
		CGNodeToCGNodeSetMap_t callers; // map a caller to it's callees
		NodeToCallSiteSetMap_t call_sites;
};

#endif
