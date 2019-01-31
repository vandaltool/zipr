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

unique_ptr<IRDB_SDK::CallGraph_t>  IRDB_SDK::CallGraph_t::factory(FileIR_t* const firp)
{
	auto ret=unique_ptr<IRDB_SDK::CallGraph_t>(new libIRDB::Callgraph_t());
	if(firp != nullptr)
		ret->addFile(firp);
	return ret;
}


Callgraph_t::~Callgraph_t()
{
	for (auto p : nodes)
	{
		delete p.second;
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

void Callgraph_t::MarkCallSite(IRDB_SDK::Instruction_t* insn)
{
	auto from_func=insn->getFunction();
	auto to_insn=insn->getTarget();
	auto to_func= to_insn==NULL? NULL : to_insn->getFunction();

	auto from_node = findNode(from_func);
	auto to_node = findNode(to_func);

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

void Callgraph_t::addFile(IRDB_SDK::FileIR_t* const firp)
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
				auto node = findNode(insn->getFunction());
				assert(node);
				callees[&GetDefaultHellNode()].insert(node);
				callers[node].insert(&GetDefaultHellNode());
		}
	}
}


void Callgraph_t::_GetAncestors(IRDB_SDK::CallGraphNode_t* const node, IRDB_SDK::CallGraphNodeSet_t &ancestors, IRDB_SDK::CallGraphNodeSet_t &visited, bool skipHellNode) const
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

	const auto &directPredecessors = getCallersOfNode(node);
	for (const auto pred : directPredecessors ) // it = directPredecessors.begin(); it != directPredecessors.end(); ++it)
	{
		if (visited.count(pred) == 0)
		{
			assert(pred);
			if (pred->isHellnode() && skipHellNode) continue;

			cerr << "adding " << GetNodeName(pred) << " to ancestor list " << hex << pred << dec << endl;
			ancestors.insert(pred);
			_GetAncestors(pred, ancestors, visited, skipHellNode);
		}
	}
}

IRDB_SDK::CallGraphNode_t* Callgraph_t::findNode(IRDB_SDK::Function_t* const fn) const
{
	auto node_it=nodes.find(fn);
	return (node_it==nodes.end()) ? nullptr : node_it->second;
}

void Callgraph_t::GetAncestors(IRDB_SDK::Function_t* const fn, IRDB_SDK::CallGraphNodeSet_t &ancestors, bool skipHellNode) const
{
	auto node = findNode(fn);
	if (node)
		GetAncestors(node, ancestors, skipHellNode);
}

void Callgraph_t::GetAncestors(IRDB_SDK::CallGraphNode_t* const node, IRDB_SDK::CallGraphNodeSet_t &ancestors, bool skipHellNode) const
{
	auto visited=IRDB_SDK::CallGraphNodeSet_t();
	_GetAncestors(node, ancestors, visited, skipHellNode);
}

bool Callgraph_t::isReachable(IRDB_SDK::CallGraphNode_t* const from, IRDB_SDK::CallGraphNode_t* const to, bool skipHellNode) const
{
	auto ancestors=IRDB_SDK::CallGraphNodeSet_t();
	GetAncestors(to, ancestors, skipHellNode);
	return (ancestors.count(from) > 0);
}

void Callgraph_t::dump(std::ostream& fout) const
{

	fout<<"Dumping callgraph ..."<<endl;

	fout<<"Mapping one way ..."<<endl;
	for(const auto p : callees) // it=callees.begin(); callees.end()!=it; ++it)
	{
		// CallGraphNode_t* node = it->first;
		// CallGraphNodeSet_t &node_callers=it->second;
		const auto  node        =p.first;
		const auto &node_callers=p.second;

		fout<<"Function "<<GetNodeName(node)<<" calls: ";
		for(const auto &the_callee : node_callers) // auto it2=node_callers.begin(); node_callers.end()!=it2; ++it2)
		{
			// CallGraphNode_t* the_callee=*it2;
			fout<<GetNodeName(the_callee)<<", ";
		}
		fout<<endl;
		for(const auto the_call_site : GetCallSites(node)) // auto it2=GetCallSites(node).begin(); GetCallSites(node).end() != it2; ++it2)
		{
			// CallSite_t the_call_site=*it2;
			fout<<"\t"<<GetCallsiteDisassembly(the_call_site)<<endl;
		}
	}

	fout<<"Mapping the other way ..."<<endl;
	for(const auto p : callers) // auto it=callers.begin(); callers.end()!=it; ++it)
	{
		// CallGraphNode_t* n=it->first;
		const auto  n           =p.first;
		const auto &node_callees=p.second;

		fout<<"Function "<<GetNodeName(n)<<" called by: ";
		for(auto the_caller : node_callees) // CallGraphNodeSet_t::iterator it2=node_callees.begin(); node_callees.end()!=it2; ++it2)
		{
			// CallGraphNode_t* the_caller=*it2;
			fout<<GetNodeName(the_caller)<<", ";

		}
		fout<<endl;
	}


	fout<<"Printing call sites..."<<endl;
	for(const auto p : call_sites) // auto it=call_sites.begin(); call_sites.end()!=it; ++it)
	{
		const auto from_node=p.first;
		const auto &call_sites_for_func=p.second;

		fout<<"Call Sites for "<<GetNodeName(from_node)<<": ";

		for(const auto the_call_site : call_sites_for_func) // auto it2=call_sites_for_func.begin(); call_sites_for_func.end() != it2; ++it2)
		{
			// auto the_call_site=*it2;
			fout<<GetCallsiteDisassembly(the_call_site)<<", ";
		}
		fout<<endl;
	}

	fout<<"Done!"<<endl;
}

void CallGraphNode_t::dump(std::ostream& fout) const
{
	if (isHellnode()) 
	{
		fout << "HELLNODE" << GetHellNodeType();
	} 
	else 
	{
		assert(getFunction());
		fout << getFunction()->getName();
	}
}

ostream& IRDB_SDK::operator<<(ostream& os, const IRDB_SDK::CallGraph_t& cg)
{
	cg.dump(os);
	return os;
}
ostream& IRDB_SDK::operator<<(ostream& os, const IRDB_SDK::CallGraphNode_t& cgn)
{
	cgn.dump(os);
	return os;
}

