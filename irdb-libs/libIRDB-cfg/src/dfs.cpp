

#include <irdb-core>
#include <irdb-cfg>
#include <vector>

using namespace IRDB_SDK;

using VisitedMap_t = map<BasicBlock_t*,bool> ;

static void doDFS(BasicBlockVector_t &order, VisitedMap_t &visited, BasicBlock_t* node)
{
	// visit this node
	visited[node]=true;
	order.push_back(node);

	// visit each successor
	for(auto successor : node->getSuccessors())
	{
		if(!visited[successor])
			doDFS(order,visited,successor);
	}

}

BasicBlockVector_t IRDB_SDK::getDFSOrder(ControlFlowGraph_t* cfg)
{
	assert(cfg!=nullptr);

	auto ret     = BasicBlockVector_t();
	ret.reserve(cfg->getBlocks().size());
	auto visited = VisitedMap_t();

	// explicitly fill the map with falses.
	for(auto& node : cfg->getBlocks())
		visited[node]=false;

	// make sure the function entry goes first
	doDFS(ret,visited,cfg->getEntry());

	for (auto it = visited.begin();it!=visited.end();it++) {
		if (!it->second && it->first->getPredecessors().size() == 0) {
			doDFS(ret,visited,it->first);
		}
	}

	// everything should be visited now, but just to be sure
	// for each node in the map
	for(auto &p : visited)
		// if it's not visited
		if(p.second == false)
			// that means it appeared unreachable from the entry block
			// add it to the dfs end.
			ret.push_back(p.first);

	return ret;
}
