
#include <libIRDB-cfg.hpp>
#include <algorithm>
#include <irdb-util>
#include <irdb-cfg>
#include <memory>


using namespace std;
using namespace libIRDB;

#define ALLOF(a) begin((a)), end((a))

/*
 * is_in_container - a handle template function returning whether key S is contained in container T.
 */
template <class T, class S>
inline bool is_in_container(const T& container, const S& key)
{
        bool is_in=container.find(key) != container.end();
        return is_in;
}

template <class S>
inline bool is_in_set(const std::set<S>& container, const S& key)
{
        return std::find(container.begin(), container.end(), key) != container.end();
}
/*
 * find_map_object - without modifying the object, return the element
 */
template <class T, class S>
inline S const& find_map_object( const std::map< T , S > &a_map, const T& key)
{
        const auto it=a_map.find(key);
        assert(it!=a_map.end());

        return (*it).second;
}



// constructor 
DominatorGraph_t::DominatorGraph_t(const ControlFlowGraph_t* p_cfg, bool needs_postdoms, bool needs_idoms)
	: cfg(*p_cfg), warn(false)
{
	assert(needs_postdoms==false);
	assert(needs_idoms==false);

	const auto dfs_order = getDFSOrder(const_cast<ControlFlowGraph_t*>(p_cfg));

	pred_func_ptr_t func_get_predecessors=[](const IRDB_SDK::BasicBlock_t* node) -> const IRDB_SDK::BasicBlockSet_t&
	{
		return node->getPredecessors();
	};

	dom_graph  =  Dom_Comp(dfs_order, func_get_predecessors, p_cfg->getEntry());
	idom_graph = Idom_Comp(dfs_order, dom_graph,             p_cfg->getEntry());

	Dominated_Compute();

// a func may have multiple exit nodes.  how do we deal with that?
// psuedo-block?  invoke this for each exit block?
//	pred_func_ptr_t func_get_successors=[](const BasicBlock_t* node) -> const BasicBlockSet_t&
//	{
//		return node->GetSuccessors();
//	};
//	post_dom_graph=Dom_Comp(p_cfg->GetBlocks(), func_get_successors, p_cfg->GetEntry());
//	post_idom_graph=IDom_Comp(p_cfg->GetBlocks(), post_dom_graph, p_cfg->GetEntry());
}




/*

algorithm from advanced compiler design & impelmentation, Mucnick, 2nd edition page 18

procedure Dom_Comp(N,Pred,r) returns Node-> set of Node
	N: in set of Node
	Pred: in Node -> set of Node
	r: in Node
	D, T: set of Node
	n, p: Node
	change := true: boolean
	Domin: Node -> set of Node
	Domin(r) := { r } 

	for each n \in N - {r} do
		Domin(n)={N}
	od
	repeat
		change := false
		for each n \in N - {r} do		

			T := N
			for each p \in Pred(n) do
				T = T intersect Domin(p)
			done
			D = {n} union T
			if D != Domin(n) then
				change := true
				Domin(n) := D
			fi
		done
	until ! change
	return Domin
end || Dom_Comp

*/

DominatorMap_t DominatorGraph_t::Dom_Comp(const IRDB_SDK::BasicBlockVector_t& N, pred_func_ptr_t get_preds,  IRDB_SDK::BasicBlock_t* r)
{
/*
	D, T: set of Node
	n, p: Node
	change := true: boolean
	Domin: Node -> set of Node
	Domin(r) := { r } 
*/
	IRDB_SDK::BasicBlockSet_t D, T;
	bool change=true;
	DominatorMap_t Domin;
	Domin[r].insert(r);

/*
	for each n \in N - {r} do
		Domin(n)={N}
	od
*/
	for( const auto &n : N)
	{
		if(n==r) continue;
		Domin[n]=IRDB_SDK::BasicBlockSet_t(ALLOF(N));
	};

/* 
	repeat
*/	
	do
	{
		change = false;
		/*
		for each n \in N - {r} do		
		*/
		for( const auto &n : N)
		{
			if(n == r) continue;
			/* T := N */
			T = IRDB_SDK::BasicBlockSet_t(ALLOF(N));
/*

			for each p \in Pred(n) do
				T = T intersect Domin(p)
			done
*/
			for(const auto &p : get_preds(n) )
			{
				IRDB_SDK::BasicBlockSet_t tmp;
				set_intersection(T.begin(), T.end(), Domin[p].begin(), Domin[p].end(), inserter(tmp,tmp.begin()));
				T=tmp;
				
			};
			/*
			D = {n} union T
			*/
			D=T;
			D.insert(n);

			/*
			if D != Domin(n) then
				change := true
				Domin(n) := D
			fi
			*/
			if(D != Domin[n])
			{
				change=true;	// keep trying
				Domin[n]=D;
			}
		};
/*
		done
	until ! change
*/
	}
	while (  change );


// log output
#if 0

                for_each(N.begin(), N.end(), [&](const BasicBlock_t* blk)
                {
                        assert(blk);
                        const BasicBlockSet_t& blk_dominates=Domin[blk];
                        Instruction_t* first_insn=*(blk->getInstructions().begin());
                        assert(first_insn);
#if 1
                        cout<<"\tBlock " <<endl<<*blk<<endl;
                        
#endif
                        cout<<"\t Dominated by:"<<endl;
                        for_each(blk_dominates.begin(), blk_dominates.end(), [&](const BasicBlock_t* dom_blk)
                        {
                                cout<<*dom_blk<<endl;
                        });
                });
#endif
	return Domin;
}



/* algorith for constructing immediate dominators from Muchnick, page 184.

procedure Idom_Comp(N,Domin,r) returns Node -> Node
	N: in set of Node
	Domin: in Node -> set of Node
	r: in Node
begin
	n, s, t: Node
	Tmp: Node -> set of Node
	IDom: Node->Node
	for each n \in N do
		Tmp(n) := Domin(n) - {n} 
	od

	for each n \in N - {r} do
		for each s \in Tmp(n) do
			for each t \in Tmp(n) - {s}  do
				if t \in Tmp(s) then
					Tmp(n) -= {t}
				fi
			od
		od
	od

	for each n \in N-{r} do
		IDom(n) = <only element in>Tmp(n)
	od
	return IDdom
end || IDom_Comp
*/
	


BlockToBlockMap_t DominatorGraph_t::Idom_Comp(const IRDB_SDK::BasicBlockVector_t& N, const DominatorMap_t &Domin, IRDB_SDK::BasicBlock_t* r) 
{
	// n, s, t: Node
	const auto verbose=false;

	// Tmp: Node -> set of Node
	auto Tmp = DominatorMap_t();
	
	// IDom: Node->Node
	auto IDom = BlockToBlockMap_t() ;

	auto tmp_n_total_size = 0u;
	auto inner_total      = 0u;
	auto inner_checks     = 0u;


	// for each n \in N do
	for(auto n : N)
	{
		//Tmp(n) := Domin(n) - {n} 
		Tmp[n] = Domin.at(n);
		Tmp[n].erase(n);
		tmp_n_total_size += Tmp[n].size();
	// od
	}

	if(verbose)
		cout << "Average size of Tmp[n] = " << tmp_n_total_size / Tmp.size() << endl;


	//for each n \in N - {r} do
	for(auto n : N)
	{
		if( n == r) continue;

		// for each s \in Tmp(n) do
		auto Tmp_n=Tmp[n];

		// for_each( Tmp_n.begin(), Tmp_n.end(), [&]( BasicBlock_t* s)
		for (auto s : Tmp_n)
		{

			//for each t \in Tmp(n) - {s}  do
			// for_each( Tmp_n.begin(), Tmp_n.end(), [&]( BasicBlock_t* t)
			for (auto t : Tmp_n)
			{
				// quickly do Tmp(n)-s 
				if(t != s)   
				{
					//if t \in Tmp(s) then
					inner_checks ++;
					if( is_in_container(Tmp[s],t))
					{
						//Tmp(n) -= {t}
						Tmp[n].erase(t); 
						inner_total ++;
					}
				}
			};
		};
	};

	if(verbose)
	{
		cout << "Inner checks = " << inner_checks << endl;
		cout << "Inner total = "  << inner_total  << endl;
	}

	//for each n \in N-{r} do
	for (auto n : N)
	{
		if(n == r) continue;

		//IDom(n) = <only element in>Tmp(n)
		IDom[n]= *(Tmp[n].begin());
		if(Tmp[n].size()!=1)	// should only be one idominator.
			warn=true;
	};
	return IDom;
} // IDom_Comp


void DominatorGraph_t::Dominated_Compute()
{
	for(auto p : dom_graph)
	{
		auto dominates     = p.first;
		auto dominated_set = p.second;

		for(auto dominated : dominated_set)
			dominated_graph[dominated].insert(dominates);
	}
	for(auto p : idom_graph)
	{
		auto dominates = p.first;
		auto dominated = p.second;
		idominated_graph[dominated].insert(dominates);
	}

}




ostream& IRDB_SDK::operator<<(ostream& os, const DominatorGraph_t& dg)
{
	dg.dump(os);
	return os;
}

void DominatorGraph_t::dump(ostream& os) const
{
	for(auto blk : cfg.getBlocks())
	{
		assert(blk);
		const auto& blk_dominators=getDominators(blk);
		auto first_insn=*(blk->getInstructions().begin());
		assert(first_insn);

		os<<"\tBlock entry id:" <<dec<<blk->getInstructions()[0]->getBaseID()<<endl;
		os<<"\t\tBlocks that (eventually) dominate me: ";
		for(auto blk : blk_dominators)
		{
			os<<blk->getInstructions()[0]->getBaseID()<<", ";
		};
		os<<endl;

		const IRDB_SDK::BasicBlock_t* idom=getImmediateDominator(blk);
		if(idom)
			os<<"\t\tMy Immediate Dominator: "<<dec<<idom->getInstructions()[0]->getBaseID()<<endl;
		else
			os<<"\t\tNo Immed Dominator."<<endl;


		os<<"\t\tBlocks I (eventually) Dominate: ";
		const auto& dominated_blocks=getDominated(blk);
		for(auto blk : dominated_blocks)
		{
			os<<blk->getInstructions()[0]->getBaseID()<<", ";
		};
		os<<endl;

		os<<"\t\tBlocks I Immediately Dominate: ";
		const auto& imm_dominated_blocks=getImmediateDominated(blk);
		for(auto blk : imm_dominated_blocks)
		{
			os<<blk->getInstructions()[0]->getBaseID()<<", ";
		};
		os<<endl;
	};

}

// constructor 
unique_ptr<IRDB_SDK::DominatorGraph_t>
IRDB_SDK::DominatorGraph_t::factory(const ControlFlowGraph_t* p_cfg, const bool needs_postdoms, const bool needs_idoms)
{
	const auto real_cfg=dynamic_cast<const libIRDB::ControlFlowGraph_t*>(p_cfg);
	return unique_ptr<IRDB_SDK::DominatorGraph_t>(new libIRDB::DominatorGraph_t(real_cfg, needs_postdoms, needs_idoms));
}





