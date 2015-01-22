
#include <map>
#include <ostream>
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;


Callgraph_t::Callgraph_t()
{
}


static bool IsCallSite(Instruction_t* insn)
{
	DISASM insnd;
	insn->Disassemble(insnd);
	return NULL!=(strstr(insnd.Instruction.Mnemonic,"call"));
}

static bool IsTailJmpSite(Instruction_t* insn)
{
	DISASM insnd;
	insn->Disassemble(insnd);
	if(strstr(insnd.Instruction.Mnemonic,"jmp")==NULL)
		return false;

	if(insn->GetTarget()==NULL)
		return true;

	if(insn->GetFunction() != insn->GetTarget()->GetFunction())
		return true;
	return false;
}

static bool IsPushJmpSite(Instruction_t* insn)
{
	DISASM insnd;
	insn->Disassemble(insnd);
	if(strstr(insnd.Instruction.Mnemonic,"push")==NULL || insn->GetFallthrough()==NULL)
		return false;

	if(insn->GetRelocations().size()==0)
		return false;

	insn->GetFallthrough()->Disassemble(insnd);
	if(strstr(insnd.Instruction.Mnemonic,"jmp")==NULL)
		return false;

	return true;
}


void Callgraph_t::MarkCallSite(Instruction_t* insn)
{

	Function_t* from_func=insn->GetFunction();
	Instruction_t* to_insn=insn->GetTarget();
	Function_t* to_func= to_insn==NULL? NULL : to_insn->GetFunction();

	call_sites[from_func].insert(insn);
	callees[from_func].insert(to_func);
	callers[to_func].insert(from_func);
}

void Callgraph_t::AddFile(libIRDB::FileIR_t *firp)
{

	// for each instruction 
	set<Instruction_t*> &insns=firp->GetInstructions();
	Instruction_t* insn=NULL, *prev=NULL;
	for(set<Instruction_t*>::iterator  it=insns.begin();
			insns.end() != it;
			++it, prev=insn)
	{
		insn=*it;
		if(IsCallSite(insn) || IsTailJmpSite(insn))
			MarkCallSite(insn);
		if(IsPushJmpSite(insn))
			MarkCallSite(insn->GetFallthrough());

		if(insn->GetFunction() && insn->GetFunction()->GetEntryPoint()==insn
			&& insn->GetIndirectBranchTargetAddress())
		{
				callees[NULL].insert(insn->GetFunction());
				callers[insn->GetFunction()].insert(NULL);
		}
	}



//		CGNodeToCGNodeSetMap_t callers;
//		CGNodeToCGNodeSetMap_t callees;
//		NodeToCallSiteSetMap_t call_sites;
}

void Callgraph_t::Dump(std::ostream& fout)
{

	fout<<"Dumping callgraph ..."<<endl;

	fout<<"Mapping one way ..."<<endl;
	for(CGNodeToCGNodeSetMap_t::iterator it=callees.begin();
		callees.end()!=it;
		++it)
	{
		Function_t* f=it->first;

		fout<<"Function "<<GetNodeName(f)<<" calls: ";
		CallGraphNodeSet_t &node_callers=it->second;
		for(CallGraphNodeSet_t::iterator it2=node_callers.begin();
			node_callers.end()!=it2;
			++it2)
		{
			Function_t* the_callee=*it2;
			fout<<GetNodeName(the_callee)<<", ";
		}
		fout<<endl;
		for(CallSiteSet_t::iterator it2=GetCallSites(f).begin();
			GetCallSites(f).end() != it2; 
			++it2)
		{
			CallSite_t the_call_site=*it2;
			fout<<"\t"<<GetCallsiteDisassembly(the_call_site)<<endl;
		}
	}

	fout<<"Mapping the other way ..."<<endl;
	for(CGNodeToCGNodeSetMap_t::iterator it=callers.begin();
		callers.end()!=it;
		++it)
	{
		Function_t* f=it->first;

		fout<<"Function "<<GetNodeName(f)<<" called by: ";
		CallGraphNodeSet_t &node_callees=it->second;
		for(CallGraphNodeSet_t::iterator it2=node_callees.begin();
			node_callees.end()!=it2;
			++it2)
		{
			Function_t* the_caller=*it2;
			fout<<GetNodeName(the_caller)<<", ";

		}
		fout<<endl;
	}


	fout<<"Printing call sites..."<<endl;
	for(NodeToCallSiteSetMap_t::iterator it=call_sites.begin();
			call_sites.end()!=it;
			++it)
	{
		CallGraphNode_t from_func=it->first;
		CallSiteSet_t  call_sites_for_func=it->second;

		fout<<"Call Sites for "<<GetNodeName(from_func)<<": ";

		for(CallSiteSet_t::iterator it2=call_sites_for_func.begin();
			call_sites_for_func.end() != it2; 
			++it2)
		{
			CallSite_t the_call_site=*it2;
			fout<<GetCallsiteDisassembly(the_call_site)<<", ";
		}
		fout<<endl;
	}

	fout<<"Done!"<<endl;
}

void Callgraph_t::_GetAncestors(const CallGraphNode_t& node, CallGraphNodeSet_t &ancestors, CallGraphNodeSet_t &visited, bool skipHellNode)
{
	if (visited.count(node) > 0)
		return;

cerr << "visiting node: " << GetNodeName(node) << " 0x" << hex << node << dec << " visited(size):" << visited.size() << endl;

        // ancestor-traversal(node X)
        //    mark X visited
        //    get parents P of X
        //    if P[i] not visited:
        //         add P[i] to ancestors
        //         ancestor-traversal(P[i])

	visited.insert(node);

	CallGraphNodeSet_t directPredecessors = GetCallersOfNode(node);
	CallGraphNodeSet_t::iterator it;
	for (it = directPredecessors.begin(); it != directPredecessors.end(); ++it)
	{
		if (visited.count(*it) == 0)
		{
			if (*it == NULL && skipHellNode) continue;

cerr << "adding " << GetNodeName(*it) << " to ancestor list " << hex << *it << dec << endl;
			ancestors.insert(*it);
			_GetAncestors(*it, ancestors, visited, skipHellNode);
		}
	}
}

void Callgraph_t::GetAncestors(const CallGraphNode_t& node, CallGraphNodeSet_t &ancestors, bool skipHellNode)
{
	CallGraphNodeSet_t visited;
	_GetAncestors(node, ancestors, visited, skipHellNode);
}

bool Callgraph_t::Reachable(const CallGraphNode_t& from, const CallGraphNode_t &to, bool skipHellNode)
{
	CallGraphNodeSet_t ancestors;
	GetAncestors(to, ancestors, skipHellNode);
	return (ancestors.count(from) > 0);
}
