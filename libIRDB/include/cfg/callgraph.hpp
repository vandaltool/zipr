#ifndef irdb_cfg_callgraph_h
#define irdb_cfg_callgraph_h

#include <sstream>

// only one type of hell node for now, but leave
// open the possibilty for multiple hell nodes
typedef enum { DEFAULT_HELL_NODE = 0 } HellNodeType; 

class CallGraphNode_t
{
	public:
		CallGraphNode_t(libIRDB::Function_t* f = NULL) {
			if (f) {
				SetFunction(f);
			} else {
				m_isHellNode = true;
				m_node.hellNodeType = DEFAULT_HELL_NODE;
			}
		}

		bool IsHellNode() const { return m_isHellNode; }

		libIRDB::Function_t* GetFunction() const {
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

	private:
		void SetFunction(Function_t* const f) {
			assert(f);
			m_node.func = f;
			m_isHellNode = false;
		}

	private:
		bool m_isHellNode;
		union {
			libIRDB::Function_t* func;
			HellNodeType hellNodeType;
		} m_node;
};

typedef std::pair<CallGraphNode_t*,CallGraphNode_t*> CallGraphEdge_t;
typedef std::set<CallGraphNode_t*> CallGraphNodeSet_t;
typedef libIRDB::Instruction_t* CallSite_t;
typedef std::set<CallSite_t> CallSiteSet_t;

class Callgraph_t
{
	public:
		Callgraph_t();
		~Callgraph_t();
		void AddFile(libIRDB::FileIR_t* const firp);

		bool EdgeExists(const CallGraphEdge_t& edge)
		{
			CallGraphNode_t *from=edge.first;
			CallGraphNode_t *to=edge.second;
				
			const CallGraphNodeSet_t& calleeSet = callers[from];
			return calleeSet.find(to)!=calleeSet.end();
		}
		bool EdgeExists(CallGraphNode_t& n1, CallGraphNode_t& n2) 
			{ return EdgeExists(CallGraphEdge_t(&n1,&n2));}

		CallGraphNodeSet_t& GetCallersOfNode(CallGraphNode_t* const node)
			{ return callers[node]; }
		CallGraphNodeSet_t& GetCalleesOfNode(CallGraphNode_t* const node)
			{ return callees[node]; }
		
		void GetAncestors(Function_t* const fn, CallGraphNodeSet_t& ancestors, bool skipHellNode = false);
		void GetAncestors(CallGraphNode_t* const node, CallGraphNodeSet_t& ancestors, bool skipHellNode = false);

		CallSiteSet_t& GetCallSites(CallGraphNode_t* const n1)
			{ return call_sites[n1]; }

		void Dump(std::ostream& fout);

		std::string GetNodeName(const CallGraphNode_t* const n1) const {
			assert(n1);
			if (n1->IsHellNode()) {
				std::ostringstream s;
				s << "HELLNODE" << n1->GetHellNodeType();
				return s.str();
			} else {
				assert(n1->GetFunction());
				return n1->GetFunction()->GetName(); 
			}
		}

		std::string GetCallsiteDisassembly(const CallSite_t &c) const
			{ return c ? c->getDisassembly() : "NOFROMFUNC"; } 

		bool Reachable(CallGraphNode_t* const from, CallGraphNode_t* const to, bool skipHellNode = false);

		CallGraphNode_t& GetDefaultHellNode() { return default_hellnode; }

		CallGraphNode_t* FindNode(Function_t* const fn);

	private:
		// create nodes from functions
		void CreateNodes(libIRDB::FileIR_t *firp);

		// mark the given insn as a call site.
		void MarkCallSite(Instruction_t* const insn);

		// traverse graph to retrieve all ancestors
		void _GetAncestors(CallGraphNode_t* const node, CallGraphNodeSet_t &ancestors, CallGraphNodeSet_t &visited, bool skipHellNode);

		typedef	std::map<CallGraphNode_t*, CallGraphNodeSet_t > CGNodeToCGNodeSetMap_t;
		typedef std::map<CallGraphNode_t*, CallSiteSet_t > NodeToCallSiteSetMap_t;
		typedef std::map<Function_t*, CallGraphNode_t*> FunctionToCGNodeMap_t;

		CGNodeToCGNodeSetMap_t callers;     // map a callee to its callees
		CGNodeToCGNodeSetMap_t callees;     // map a caller to its callers
		NodeToCallSiteSetMap_t call_sites;
		CallGraphNode_t default_hellnode;   // default hell node
		FunctionToCGNodeMap_t nodes;        // maps functions to call graph nodes
};

#endif
