
#include <libIRDB-cfg.hpp>
#include <algorithm>
#include <utils.hpp>



using namespace std;
using namespace libIRDB;


// constructor 
DominatorGraph_t::DominatorGraph_t(const ControlFlowGraph_t* p_cfg, bool needs_postdoms, bool needs_idoms)
	: cfg(*p_cfg), warn(false)
{


	assert(needs_postdoms==false);
	assert(needs_idoms==false);


//	typedef const BasicBlockSet_t& (*) (const BasicBlock_t* node) pred_func_ptr_t;
	pred_func_ptr_t func_get_predecessors=[](const BasicBlock_t* node) -> const BasicBlockSet_t&
	{
		return node->GetPredecessors();
	};

	dom_graph=Dom_Comp(p_cfg->GetBlocks(), func_get_predecessors, p_cfg->GetEntry());
	idom_graph=Idom_Comp(p_cfg->GetBlocks(), dom_graph, p_cfg->GetEntry());

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

DominatorMap_t DominatorGraph_t::Dom_Comp(const BasicBlockSet_t& N, pred_func_ptr_t get_preds,  BasicBlock_t* r)
{
/*
	D, T: set of Node
	n, p: Node
	change := true: boolean
	Domin: Node -> set of Node
	Domin(r) := { r } 
*/
	BasicBlockSet_t D, T;
	bool change=true;
	DominatorMap_t Domin;
	Domin[r].insert(r);

	BasicBlockSet_t NminusR=N;
	NminusR.erase(r);

/*
	for each n \in N - {r} do
		Domin(n)={N}
	od
*/
	for_each( NminusR.begin(), NminusR.end(), [&](BasicBlock_t* n)
	{
		Domin[n]=N;
	});

/* 
	repeat
*/	
	do
	{
		change = false;
		/*
		for each n \in N - {r} do		
		*/
		for_each(NminusR.begin(), NminusR.end(), [&](BasicBlock_t* n)
		{
			/* T := N */
			T=N;
/*

			for each p \in Pred(n) do
				T = T intersect Domin(p)
			done
*/
			for_each(get_preds(n).begin(), get_preds(n).end(), [&](const BasicBlock_t* p)
			{
				BasicBlockSet_t tmp;
				set_intersection(T.begin(), T.end(), Domin[p].begin(), Domin[p].end(), inserter(tmp,tmp.begin()));
				T=tmp;
				
			});
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
		});
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
                        Instruction_t* first_insn=*(blk->GetInstructions().begin());
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
	


BlockToBlockMap_t DominatorGraph_t::Idom_Comp(const BasicBlockSet_t& N, const DominatorMap_t &Domin, BasicBlock_t* r) 
{
	// n, s, t: Node
	//BasicBlock_t* n=NULL, *s=NULL, *t=NULL;

	// Tmp: Node -> set of Node
	DominatorMap_t Tmp;
	
	// IDom: Node->Node
	BlockToBlockMap_t IDom;


	// calculate this set as we use it several times
	BasicBlockSet_t NminusR = N;
	NminusR.erase(r);


	// for each n \in N do
	for_each( N.begin(), N.end(), [&](BasicBlock_t* n)
	{
		//Tmp(n) := Domin(n) - {n} 
		Tmp[n] = Domin.at(n);
		Tmp[n].erase(n);
	// od
	});


	//for each n \in N - {r} do
	for_each( NminusR.begin(), NminusR.end(), [&]( BasicBlock_t* n)
	{
		// for each s \in Tmp(n) do
		auto Tmp_n=Tmp[n];
		for_each( Tmp_n.begin(), Tmp_n.end(), [&]( BasicBlock_t* s)
		{

			//for each t \in Tmp(n) - {s}  do
			for_each( Tmp_n.begin(), Tmp_n.end(), [&]( BasicBlock_t* t)
			{
				// quickly do Tmp(n)-s 
				if(t != s)   
				{
					//if t \in Tmp(s) then
					if( is_in_container(Tmp[s],t))
					{
						//Tmp(n) -= {t}
						Tmp[n].erase(t);
					}
				}
			});
		});
	});

	//for each n \in N-{r} do
	for_each( NminusR.begin(), NminusR.end(), [&](BasicBlock_t* n)
	{
		//IDom(n) = <only element in>Tmp(n)
		IDom[n]= *(Tmp[n].begin());
		if(Tmp[n].size()!=1)	// should only be one idominator.
			warn=true;
	});
	return IDom;
} // IDom_Comp




ostream& libIRDB::operator<<(ostream& os, const DominatorGraph_t& dg)
{
	for_each(dg.cfg.GetBlocks().begin(), dg.cfg.GetBlocks().end(), [&](const BasicBlock_t* blk)
	{
		assert(blk);
		const BasicBlockSet_t& blk_dominates=dg.GetDominators(blk);
		auto first_insn=*(blk->GetInstructions().begin());
		assert(first_insn);

		os<<"\tBlock entry id:" <<blk->GetInstructions()[0]->getBaseID()<<endl;
		os<<"\t\tDominated by: ";
		for_each(blk_dominates.begin(), blk_dominates.end(), [&os](const BasicBlock_t* dom)
		{
			os<<dom->GetInstructions()[0]->getBaseID()<<", ";
		});
		os<<endl;

		const BasicBlock_t* idom=dg.GetImmediateDominator(blk);
		if(idom)
			os<<"\t\tImmediate Dominator: "<<hex<<idom->GetInstructions()[0]->getBaseID()<<endl;
		else
			os<<"\t\tNo Immed Dominator."<<endl;

	});

	return os;
}
