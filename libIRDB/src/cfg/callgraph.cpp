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


#include <map>
#include <ostream>
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;


Callgraph_t::Callgraph_t() : 
	default_hellnode(NULL)
{
}

Callgraph_t::~Callgraph_t()
{
	for (FunctionToCGNodeMap_t::iterator it = nodes.begin(); it != nodes.end(); ++it)
	{
		CallGraphNode_t* n = it->second;
		delete(n);
	}
	nodes.clear();
}

static bool IsCallSite(Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);
	return d->isCall();
}

static bool IsTailJmpSite(Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);
	if(d->isBranch())
		return false;

	if(insn->getTarget()==NULL)
		return true;

	if(insn->getFunction() != insn->getTarget()->getFunction())
		return true;
	return false;
}

static bool IsPushJmpSite(Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);
	if(d->getMnemonic()!="push" || insn->getFallthrough()==NULL)
		return false;

	if(insn->getRelocations().size()==0)
		return false;

	const auto d2=DecodedInstruction_t::factory(insn->getFallthrough());
	if(!d2->isBranch())
		return false;

	return true;
}

void Callgraph_t::MarkCallSite(Instruction_t* insn)
{
	auto from_func=insn->getFunction();
	auto to_insn=insn->getTarget();
	auto to_func= to_insn==NULL? NULL : to_insn->getFunction();

	auto from_node = FindNode(from_func);
	auto to_node = FindNode(to_func);

	if (!from_node)
		from_node = &GetDefaultHellNode();

	if (!to_node)
		to_node = &GetDefaultHellNode();

	call_sites[from_node].insert(insn);
	callees[from_node].insert(to_node);
	callers[to_node].insert(from_node);
}

void Callgraph_t::CreateNodes(IRDB_SDK::FileIR_t *firp)
{
	const FunctionSet_t &fns=firp->getFunctions();
	for(auto it=fns.begin(); fns.end() != it; ++it)
	{
		auto fn=*it;
		if (fn) {
			auto newnode = new CallGraphNode_t(fn);
			nodes[fn] = newnode;
//			cout << "Added CGNode: " << GetNodeName(newnode) << endl;
		}
	}
}

void Callgraph_t::AddFile(IRDB_SDK::FileIR_t* const firp)
{
	// Create CG Nodes from functions
	CreateNodes(firp);

	// for each instruction 
	auto &insns=firp->getInstructions();

	for(auto it=insns.begin(); insns.end() != it; ++it)
	{
		auto insn=dynamic_cast<Instruction_t*>(*it);
		if(IsCallSite(insn) || IsTailJmpSite(insn))
			MarkCallSite(insn);
		if(IsPushJmpSite(insn))
			MarkCallSite(dynamic_cast<Instruction_t*>(insn->getFallthrough()));

		if(insn->getFunction() && insn->getFunction()->getEntryPoint()==insn
			&& insn->getIndirectBranchTargetAddress())
		{
				auto node = FindNode(insn->getFunction());
				assert(node);
				callees[&GetDefaultHellNode()].insert(node);
				callers[node].insert(&GetDefaultHellNode());
		}
	}
}

void Callgraph_t::Dump(std::ostream& fout)
{

	fout<<"Dumping callgraph ..."<<endl;

	fout<<"Mapping one way ..."<<endl;
	for(auto it=callees.begin(); callees.end()!=it; ++it)
	{
		CallGraphNode_t* node = it->first;

		fout<<"Function "<<GetNodeName(node)<<" calls: ";
		CallGraphNodeSet_t &node_callers=it->second;
		for(auto it2=node_callers.begin(); node_callers.end()!=it2; ++it2)
		{
			CallGraphNode_t* the_callee=*it2;
			fout<<GetNodeName(the_callee)<<", ";
		}
		fout<<endl;
		for(auto it2=GetCallSites(node).begin(); GetCallSites(node).end() != it2; ++it2)
		{
			CallSite_t the_call_site=*it2;
			fout<<"\t"<<GetCallsiteDisassembly(the_call_site)<<endl;
		}
	}

	fout<<"Mapping the other way ..."<<endl;
	for(auto it=callers.begin(); callers.end()!=it; ++it)
	{
		CallGraphNode_t* n=it->first;

		fout<<"Function "<<GetNodeName(n)<<" called by: ";
		CallGraphNodeSet_t &node_callees=it->second;
		for(CallGraphNodeSet_t::iterator it2=node_callees.begin();
			node_callees.end()!=it2;
			++it2)
		{
			CallGraphNode_t* the_caller=*it2;
			fout<<GetNodeName(the_caller)<<", ";

		}
		fout<<endl;
	}


	fout<<"Printing call sites..."<<endl;
	for(auto it=call_sites.begin(); call_sites.end()!=it; ++it)
	{
		auto from_node=it->first;
		auto &call_sites_for_func=it->second;

		fout<<"Call Sites for "<<GetNodeName(from_node)<<": ";

		for(auto it2=call_sites_for_func.begin(); call_sites_for_func.end() != it2; ++it2)
		{
			auto the_call_site=*it2;
			fout<<GetCallsiteDisassembly(the_call_site)<<", ";
		}
		fout<<endl;
	}

	fout<<"Done!"<<endl;
}

void Callgraph_t::_GetAncestors(CallGraphNode_t* const node, CallGraphNodeSet_t &ancestors, CallGraphNodeSet_t &visited, bool skipHellNode)
{
	if (!node || visited.count(node) > 0)
		return;

cerr << "visiting node: " << GetNodeName(node) << " visited(size):" << visited.size() << endl;

        // ancestor-traversal(node X)
        //    mark X visited
        //    get parents P of X
        //    if P[i] not visited:
        //         add P[i] to ancestors
        //         ancestor-traversal(P[i])

	visited.insert(node);

	auto directPredecessors = GetCallersOfNode(node);
	for (auto it = directPredecessors.begin(); it != directPredecessors.end(); ++it)
	{
		if (visited.count(*it) == 0)
		{
			assert(*it);
			if ((*it)->IsHellnode() && skipHellNode) continue;

cerr << "adding " << GetNodeName(*it) << " to ancestor list " << hex << *it << dec << endl;
			ancestors.insert(*it);
			_GetAncestors(*it, ancestors, visited, skipHellNode);
		}
	}
}

CallGraphNode_t* Callgraph_t::FindNode(IRDB_SDK::Function_t* const fn)
{
	return nodes[fn];
}

void Callgraph_t::GetAncestors(IRDB_SDK::Function_t* const fn, CallGraphNodeSet_t &ancestors, bool skipHellNode)
{
	auto node = FindNode(fn);
	if (node)
		GetAncestors(node, ancestors, skipHellNode);
}

void Callgraph_t::GetAncestors(CallGraphNode_t* const node, CallGraphNodeSet_t &ancestors, bool skipHellNode)
{
	auto visited=CallGraphNodeSet_t();
	_GetAncestors(node, ancestors, visited, skipHellNode);
}

bool Callgraph_t::Reachable(CallGraphNode_t* const from, CallGraphNode_t* const to, bool skipHellNode)
{
	auto ancestors=CallGraphNodeSet_t();
	GetAncestors(to, ancestors, skipHellNode);
	return (ancestors.count(from) > 0);
}
