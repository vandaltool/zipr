


typedef std::map<const BasicBlock_t*, BasicBlockSet_t> DominatorMap_t;
typedef std::map<const BasicBlock_t*, BasicBlock_t*> BlockToBlockMap_t;

class DominatorGraph_t
{
	public:
		DominatorGraph_t(const ControlFlowGraph_t* p_cfg, bool needs_postdoms=false, bool needs_idoms=false);


		// get the (post) dominators for a node 
		BasicBlockSet_t& GetDominators(const BasicBlock_t* node) { return dom_graph[node]; }
		BasicBlockSet_t& GetPostDominators(const BasicBlock_t* node) { return post_dom_graph[node]; }

		const BasicBlockSet_t& GetDominators(const BasicBlock_t* node) const { return dom_graph.at(node); }
		const BasicBlockSet_t& GetPostDominators(const BasicBlock_t* node) const { return post_dom_graph.at(node); }

		bool HasWarnings() const { return warn; }


		// get the immeidate (post) dominators for a node 
		const BasicBlock_t* GetImmediateDominator(const BasicBlock_t* node)  const
		{ auto it=idom_graph.find(node); return (it!=idom_graph.end()) ? it->second : NULL; }
		const BasicBlock_t* GetImmediatePostDominators(const BasicBlock_t* node)  const
		{ auto it=post_idom_graph.find(node); return (it!=post_idom_graph.end()) ? it->second : NULL; }


	private:

        	typedef const BasicBlockSet_t& (*pred_func_ptr_t) (const BasicBlock_t* node);

		DominatorMap_t Dom_Comp(const BasicBlockSet_t& N, pred_func_ptr_t pred_func, BasicBlock_t* r);
		BlockToBlockMap_t Idom_Comp(const BasicBlockSet_t& N, const DominatorMap_t &Domin, BasicBlock_t* r);


		DominatorMap_t dom_graph;
		BlockToBlockMap_t idom_graph;

		DominatorMap_t post_dom_graph;
		BlockToBlockMap_t post_idom_graph;

		const ControlFlowGraph_t& cfg;	// a reference to our cfg.

		bool warn;

                friend std::ostream& operator<<(std::ostream& os, const DominatorGraph_t& cfg);
	
};

std::ostream& operator<<(std::ostream& os, const DominatorGraph_t& cfg);


