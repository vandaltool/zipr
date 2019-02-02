#include <ostream>
namespace libIRDB
{
	using namespace std;

        using  DominatorMap_t    = map<const IRDB_SDK::BasicBlock_t*, IRDB_SDK::BasicBlockSet_t>;
        using  BlockToBlockMap_t = map<const IRDB_SDK::BasicBlock_t*, IRDB_SDK::BasicBlock_t*>;





	class DominatorGraph_t : public IRDB_SDK::DominatorGraph_t
	{
		public:
			DominatorGraph_t(const ControlFlowGraph_t* p_cfg, bool needs_postdoms=false, bool needs_idoms=false);
			virtual ~DominatorGraph_t() { } 


			// get the (post) dominators for a node 
			IRDB_SDK::BasicBlockSet_t& GetDominators(const IRDB_SDK::BasicBlock_t* node) { return dom_graph[node]; }
			IRDB_SDK::BasicBlockSet_t& GetPostDominators(const IRDB_SDK::BasicBlock_t* node) { return post_dom_graph[node]; }

			const IRDB_SDK::BasicBlockSet_t& getDominators(const IRDB_SDK::BasicBlock_t* node) const { return dom_graph.at(node); }
			const IRDB_SDK::BasicBlockSet_t& getPostDominators(const IRDB_SDK::BasicBlock_t* node) const { return post_dom_graph.at(node); }

			bool hasWarnings() const { return warn; }


			// get the immeidate (post) dominators for a node 
			const IRDB_SDK::BasicBlock_t* getImmediateDominator(const IRDB_SDK::BasicBlock_t* node)  const
			{ auto it=idom_graph.find(node); return (it!=idom_graph.end()) ? it->second : NULL; }
			const IRDB_SDK::BasicBlock_t* getImmediatePostDominators(const IRDB_SDK::BasicBlock_t* node)  const
			{ auto it=post_idom_graph.find(node); return (it!=post_idom_graph.end()) ? it->second : NULL; }

			void dump(ostream& os=cout) const; 


		private:

			typedef const IRDB_SDK::BasicBlockSet_t& (*pred_func_ptr_t) (const IRDB_SDK::BasicBlock_t* node);

			DominatorMap_t Dom_Comp(const IRDB_SDK::BasicBlockSet_t& N, pred_func_ptr_t pred_func, IRDB_SDK::BasicBlock_t* r);
			BlockToBlockMap_t Idom_Comp(const IRDB_SDK::BasicBlockSet_t& N, const DominatorMap_t &Domin, IRDB_SDK::BasicBlock_t* r);


			DominatorMap_t dom_graph;
			BlockToBlockMap_t idom_graph;

			DominatorMap_t post_dom_graph;
			BlockToBlockMap_t post_idom_graph;

			const ControlFlowGraph_t& cfg;	// a reference to our cfg.

			bool warn;

	};



}
