#include <set>
#include <iostream>

#include "cgclibc.hpp"

using namespace std;

// WARNING: DO NOT CHANGE FORMAT OF OUTPUT
void CGC_libc::emitFunctionInfo(Function_t *p_fn)
{
	if (p_fn && p_fn->GetEntryPoint() && p_fn->GetEntryPoint()->GetAddress())
	{
		cout << "function " << p_fn->GetName() << " 0x" << hex << p_fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() << dec << endl;
	}
}

// format of file
//    <test> positive <libc_function> <candidate_function>
//
// post: m_mallocUniverse will contains the relevant candidate functions
void CGC_libc::setPositiveInferences(std::string p_positiveFile)
{
	ifstream pin(p_positiveFile.c_str(), std::ifstream::in);
	while (!pin.eof())
	{
		char buf[2024];
		char libcFn[2024];
		char candidateFn[2024];

		if (pin.getline(buf, 2000))
		{
			sscanf(buf,"%*s %*s %s %s", libcFn, candidateFn); 
			cout << "fn: " << libcFn << endl;

			if (strcmp(libcFn,"malloc")==0)
			{
				set<Function_t*>::iterator it;
				for (it = m_firp->GetFunctions().begin(); it != m_firp->GetFunctions().end(); ++it)
				{
					Function_t *fn = *it;
					if (fn && fn->GetName() == string(candidateFn))
					{
						m_mallocUniverse.insert(m_cg.FindNode(fn));
					}
				}
			} 
		}
	}

	pin.close();
}

// format of file
//    <test> negative <libc_function> <candidate_function>
//
// post: m_mallocNegativeUniverse set
void CGC_libc::setNegativeInferences(std::string p_negativeFile)
{
	ifstream pin(p_negativeFile.c_str(), std::ifstream::in);
	while (!pin.eof())
	{
		char buf[2024];
		char libcFn[2024];
		char candidateFn[2024];

		if (pin.getline(buf, 2000))
		{
			sscanf(buf,"%*s %*s %s %s", libcFn, candidateFn); 
			cout << "fn: " << libcFn << endl;

			if (strcmp(libcFn,"malloc")==0)
			{
				set<Function_t*>::iterator it;
				for (it = m_firp->GetFunctions().begin(); it != m_firp->GetFunctions().end(); ++it)
				{
					Function_t *fn = *it;
					if (fn && fn->GetName() == string(candidateFn))
					{
						cout << fn->GetName() << " cannot be malloc()" << endl;
						m_mallocNegativeUniverse.insert(m_cg.FindNode(fn));
					}
				}
			} 
		}
	}

	pin.close();
}

void CGC_libc::displayAllFunctions()
{
	FunctionSet_t &functions = m_firp->GetFunctions();
	for (set<Function_t*>::iterator i = functions.begin(); i != functions.end(); ++i)
	{
		Function_t *fn = *i;
		if (!fn) continue;
		emitFunctionInfo(fn);
	}
}

static void emitCandidate(std::string str, Function_t *p_fn)
{
	if (p_fn && p_fn->GetEntryPoint() && p_fn->GetEntryPoint()->GetAddress())
	{
		cout << "static positive " << str << " " << p_fn->GetName() << " 0x" << hex << p_fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() << dec << " " << endl;
	}
}

static void displayFinalInference(Callgraph_t &p_cg, CallGraphNodeSet_t &p_maybes, string p_funcName)
{
	for (CallGraphNodeSet_t::iterator i = p_maybes.begin(); i != p_maybes.end(); ++i)
	{
		CallGraphNode_t *node = *i;
		if (node->IsHellnode()) continue;

		Function_t *fn = node->GetFunction();
		if (!fn) continue;
		emitCandidate(p_funcName, fn);
	}
}

static void displayMaybes(Callgraph_t &p_cg, CallGraphNodeSet_t& p_maybes, string p_funcName)
{
	for (CallGraphNodeSet_t::iterator i = p_maybes.begin(); i != p_maybes.end(); ++i)
	{
		CallGraphNode_t *n = *i;
		if (n->IsHellnode()) continue;
		Function_t *fn = n->GetFunction();
		if (fn && fn->GetEntryPoint() && fn->GetEntryPoint()->GetAddress())
		{
			cout << "maybe " << p_funcName << " 0x" << hex << fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() << dec << " " << fn->GetName() << " args: " << fn->GetNumArguments() << endl;
		}
	}
}

static void displayMaybes(Callgraph_t &p_cg, set<Function_t*> &p_maybes, string p_funcName)
{
		// obsolete?
	for (set<Function_t*>::iterator i = p_maybes.begin(); i != p_maybes.end(); ++i)
	{
		Function_t *fn = *i;
		if (fn && fn->GetEntryPoint() && fn->GetEntryPoint()->GetAddress())
		{
			cout << "maybe " << p_funcName << " 0x" << hex << fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() << dec << " " << fn->GetName() << endl;
		}
	}
}

CGC_libc::CGC_libc(FileIR_t *p_firp) :
	m_syscalls(p_firp)
{
	m_skipHellNode = false;
	m_clustering = false;
	m_dominance = false;

	m_firp = p_firp;
	m__terminateWrapper = NULL;
	m_transmitWrapper = NULL;
	m_receiveWrapper = NULL;
	m_fdwaitWrapper = NULL;
	m_allocateWrapper = NULL;
	m_deallocateWrapper = NULL;
	m_randomWrapper = NULL;

	m_cg.AddFile(m_firp);
	m_cg.Dump(cout);

	int elfoid=m_firp->GetFile()->GetELFOID();
	pqxx::largeobject lo(elfoid);
	libIRDB::pqxxDB_t *interface=dynamic_cast<libIRDB::pqxxDB_t*>(libIRDB::BaseObj_t::GetInterface());
	assert(interface);
	lo.to_file(interface->GetTransaction(),"tmp.exe");

	m_elfiop=new ELFIO::elfio;
	m_elfiop->load("tmp.exe");
}

void CGC_libc::findSyscallWrappers()
{
	SyscallSiteSet_t sites = m_syscalls.GetSyscalls();

	FunctionSet_t f_terminate;
	FunctionSet_t f_transmit;
	FunctionSet_t f_receive;
	FunctionSet_t f_fdwait;
	FunctionSet_t f_allocate;
	FunctionSet_t f_deallocate;
	FunctionSet_t f_random;
	
	set<SyscallSite_t>::iterator it;
	for (it = sites.begin(); it != sites.end(); ++it)
	{
		SyscallSite_t site = *it;
		Function_t* fn = site.GetSite()->GetFunction();

		if (!fn) continue;

		switch (site.GetSyscallNumber()) 
		{
			case SNT_terminate:
				f_terminate.insert(fn);
				break;
			case SNT_transmit:
				f_transmit.insert(fn);
				break;
			case SNT_receive:
				f_receive.insert(fn);
				break;
			case SNT_fdwait:
				f_fdwait.insert(fn);
				break;
			case SNT_allocate:
				f_allocate.insert(fn);
				break;
			case SNT_deallocate:
				f_deallocate.insert(fn);
				break;
			case SNT_random:
				f_random.insert(fn);
				break;
		}
	}

	if (f_terminate.size() == 1)
	{
		FunctionSet_t::iterator fi = f_terminate.begin();
		if (*fi)
			m__terminateWrapper = m_cg.FindNode(*fi);
	}

	if (f_transmit.size() == 1)
	{
		FunctionSet_t::iterator fi = f_transmit.begin();
		if (*fi)
			m_transmitWrapper = m_cg.FindNode(*fi);
	}

	if (f_receive.size() == 1)
	{
		FunctionSet_t::iterator fi = f_receive.begin();
		if (*fi)
			m_receiveWrapper = m_cg.FindNode(*fi);
	}

	if (f_fdwait.size() == 1)
	{
		FunctionSet_t::iterator fi = f_fdwait.begin();
		if (*fi)
			m_fdwaitWrapper = m_cg.FindNode(*fi);
	}

	if (f_random.size() == 1)
	{
		FunctionSet_t::iterator fi = f_random.begin();
		if (*fi)
			m_randomWrapper = m_cg.FindNode(*fi);
	}

	if (f_allocate.size() == 1)
	{
		FunctionSet_t::iterator fi = f_allocate.begin();
		if (*fi)
		{
			m_allocateWrapper = m_cg.FindNode(*fi);
			m_cg.GetAncestors(m_allocateWrapper, m_maybeMallocs, m_skipHellNode);
		}
	}

	if (f_deallocate.size() == 1)
	{
		FunctionSet_t::iterator fi = f_deallocate.begin();
		if (*fi)
		{
			m_deallocateWrapper = m_cg.FindNode(*fi);
			m_cg.GetAncestors(m_deallocateWrapper, m_maybeFrees, m_skipHellNode);
		}
	}
}

bool CGC_libc::potentialMallocFunctionPrototype(Function_t *p_fn)
{
	// conservatively rely only on the number of arguments
	return (p_fn && p_fn->GetNumArguments() == 1) ? true : false;

/*
	if (p_fn && p_fn->GetNumArguments() == 1)
	{
		FuncType_t* ftype = p_fn->GetType();
		if (!ftype) return true;

		AggregateType_t* aggtype = ftype->GetArgumentsType();
		if (!aggtype) return true;

		Type_t *t = aggtype->GetAggregatedType(0);
		if (!t) return true;

		cout << " type id: " << t->GetTypeID() << " / " << t->GetName() << endl;
		if (t->IsUnknownType() || t->IsNumericType() || t->IsBasicType())
			return true;
		else
			return false;
	}

	return false; 
*/
}

bool CGC_libc::potentialFreeFunctionPrototype(Function_t *p_fn)
{
	// conservatively rely only on the number of arguments
	return (p_fn && p_fn->GetNumArguments() == 1) ? true : false;

/*
	if (p_fn && p_fn->GetNumArguments() == 1)
	{
		FuncType_t* ftype = p_fn->GetType();
		if (!ftype) return true;

		AggregateType_t* aggtype = ftype->GetArgumentsType();
		if (!aggtype) return true;

		Type_t *t = aggtype->GetAggregatedType(0);
		if (!t) return true;

		cout << " type id: " << t->GetTypeID() << " / " << t->GetName() << endl;
		if (t->IsUnknownType() || t->IsPointerType())
			return true;
		else
			return false;
	}

	return false; 
*/
}

void CGC_libc::pruneMallocs()
{
	// remove functions that cannot be malloc()
	//    malloc() won't call transmit, receive, fdwait, even indirectly
	//    should have the right function prototype: POINTER malloc(NUMERIC)

	// make a copy
	CallGraphNodeSet_t t = m_maybeMallocs;

	for (CallGraphNodeSet_t::iterator i = t.begin(); i != t.end(); ++i)
	{
		CallGraphNode_t* node = *i;
		if (node->IsHellnode())
		{
			m_maybeMallocs.erase(node);
			continue;
		}

		Function_t *fn = node->GetFunction();

		if (fn == NULL) {
			continue;
		}

cout << "Looking at function: 0x" << hex << fn << dec << endl;
cout << "Function name: " << fn->GetName() << endl;

		// m_mallocUniverse has candidate set of all mallocs
		//   (determined dynamically)
		if (m_mallocUniverse.size() > 0 && m_mallocUniverse.count(node) == 0)
		{
			m_maybeMallocs.erase(node); 
			continue;
		}

		if (m_mallocNegativeUniverse.count(node) > 0)
		{
			m_maybeMallocs.erase(node); 
			continue;
		}

		if (m_cg.GetCalleesOfNode(m_transmitWrapper).count(node) > 0)
		{
			cout << "calls transmit, remove" << endl;
			m_maybeMallocs.erase(node);
			continue;
		}

		if (m_cg.GetCalleesOfNode(m_receiveWrapper).count(node) > 0)
		{
			cout << "calls receive, remove" << endl;
			m_maybeMallocs.erase(node);
			continue;
		}

		if (m_cg.GetCalleesOfNode(m_fdwaitWrapper).count(node) > 0)
		{
			cout << "calls fdwait, remove" << endl;
			m_maybeMallocs.erase(node);
			continue;
		}

		cout << fn->GetName() << " has " << fn->GetNumArguments() << " arguments" << endl;
		if (!potentialMallocFunctionPrototype(fn))
		{
			m_maybeMallocs.erase(node);
			continue;
		}
	}
}

void CGC_libc::pruneFrees()
{
	// (2) remove functions that cannot be free()
	// free() won't call transmit, receive, fdwait, even indirectly
	// should have the right function prototype: void free(POINTER)
	CallGraphNodeSet_t t = m_maybeFrees; 
	CallGraphNodeSet_t::iterator i;

	for (i = t.begin(); i != t.end(); ++i)
	{
		CallGraphNode_t *node = *i;

		if (node->IsHellnode())
		{
			m_maybeFrees.erase(node);
			continue;
		}

		Function_t *fn = node->GetFunction();

		if (!fn) continue;

cout << "Looking at function: " << fn->GetName() << endl;

		if (m_cg.GetCalleesOfNode(m_transmitWrapper).count(node) > 0)
		{
			cout << "calls transmit, remove" << endl;
			m_maybeFrees.erase(node);
			continue;
		}

		if (m_cg.GetCalleesOfNode(m_receiveWrapper).count(node) > 0)
		{
			cout << "calls receive, remove" << endl;
			m_maybeFrees.erase(node);
			continue;
		}

		if (m_cg.GetCalleesOfNode(m_fdwaitWrapper).count(node) > 0)
		{
			cout << "calls fdwait, remove" << endl;
			m_maybeFrees.erase(node);
			continue;
		}

		if (m_cg.GetCalleesOfNode(m_allocateWrapper).count(node) > 0)
		{
			cout << "calls allocate, remove" << endl;
			m_maybeFrees.erase(node);
			continue;
		}

		if (!potentialFreeFunctionPrototype(fn))
		{
			m_maybeFrees.erase(node);
			continue;
		}

	}
}

#ifdef UNUSED
void CGC_libc::findUnreachableNodes()
{
	FunctionSet_t &functions = m_firp->GetFunctions();
	
	for (FunctionSet_t::iterator it = functions.begin(); 
		it != functions.end(); ++it)
	{
		if (m_cg.GetCallersOfNode(*it).size() == 0)
		{
			cout << "Unreachable function detected: " << m_cg.GetNodeName(*it) << endl;
		}
	}
}
#endif


void CGC_libc::clusterFreeMalloc()
{
	if (m_maybeMallocs.size() == 0)
		return;

	cout << "Do Malloc()" << endl;

	// m1 -> 20, 15
	// m2 -> 20, 12
	// m3 -> 20, 10
	std::map<CallGraphNode_t*, std::set<int> > mallocs;

	// f1 -> 8
	// f2 -> 10
	std::map<CallGraphNode_t*, std::set<int> > frees;

	for (CallGraphNodeSet_t::iterator n = m_maybeMallocs.begin(); n != m_maybeMallocs.end(); ++n)
	{
		CallGraphNode_t *node = *n;
		if (!node || node->IsHellnode()) continue;

		Function_t *fn = node->GetFunction();
		if (!fn) continue;

		for(set<Instruction_t*>::const_iterator i=fn->GetInstructions().begin(); i!=fn->GetInstructions().end(); ++i)
		{
			Instruction_t *insn = *i;
			if (!insn) continue;

			DISASM d;
			insn->Disassemble(d);
			
			if (d.Argument1.ArgType == MEMORY_TYPE)
			{
				int displacement = d.Argument1.Memory.Displacement;
				if (isGlobalData(displacement))
				{
					cout << "Function: " << fn->GetName() << "  global data found at displacement: " << displacement << endl;
					mallocs[node].insert(displacement);
				}
			}

			if (d.Argument2.ArgType == MEMORY_TYPE)
			{
				int displacement = d.Argument2.Memory.Displacement;
				if (isGlobalData(displacement))
				{
					cout << "Function: " << fn->GetName() << "  global data found at displacement: " << displacement << endl;
					mallocs[node].insert(displacement);
				}
			}

			if (d.Argument3.ArgType == MEMORY_TYPE)
			{
				int displacement = d.Argument3.Memory.Displacement;
				if (isGlobalData(displacement))
				{
					cout << "Function: " << fn->GetName() << "  global data found at displacement: " << displacement << endl;
					mallocs[node].insert(displacement);
				}
			}
		}
	}

	cout << "Do Free()" << endl;
	for (CallGraphNodeSet_t::iterator n = m_maybeFrees.begin(); n != m_maybeFrees.end(); ++n)
	{
		CallGraphNode_t *node = *n;
		if (!node || node->IsHellnode()) continue;

		Function_t *fn = node->GetFunction();
		if (!fn) continue;

		for(set<Instruction_t*>::const_iterator i=fn->GetInstructions().begin(); i!=fn->GetInstructions().end(); ++i)
		{
			Instruction_t *insn = *i;
			if (!insn) continue;

			DISASM d;
			insn->Disassemble(d);
			
			if (d.Argument1.ArgType == MEMORY_TYPE)
			{
				int displacement = d.Argument1.Memory.Displacement;
				if (isGlobalData(displacement))
				{
					cout << "Function: " << fn->GetName() << "  global data found at displacement: " << displacement << endl;
					frees[node].insert(displacement);
				}
			}

			if (d.Argument2.ArgType == MEMORY_TYPE)
			{
				int displacement = d.Argument2.Memory.Displacement;
				if (isGlobalData(displacement))
				{
					cout << "Function: " << fn->GetName() << "  global data found at displacement: " << displacement << endl;
					frees[node].insert(displacement);
				}
			}

			if (d.Argument3.ArgType == MEMORY_TYPE)
			{
				int displacement = d.Argument3.Memory.Displacement;
				if (isGlobalData(displacement))
				{
					cout << "Function: " << fn->GetName() << "  global data found at displacement: " << displacement << endl;
					frees[node].insert(displacement);
				}
			}
		}
	}

	CallGraphNodeSet_t maybeMallocsWithGlobals;
	CallGraphNodeSet_t maybeFreesWithGlobals;

	// find the true malloc
	std::map<CallGraphNode_t*, std::set<int> >::iterator it;
	std::map<CallGraphNode_t*, std::set<int> >::iterator it2;

	for (it = mallocs.begin(); it != mallocs.end(); ++it)
	{
		CallGraphNode_t *node = it->first;
		if (!node || node->IsHellnode()) continue;
		Function_t *fn = node->GetFunction();
		set<int> displacements = it->second;
		cout << endl;
		cout << "investigating function: " << fn->GetName() << endl;
		for (set<int>::iterator d = displacements.begin(); d != displacements.end(); ++d)
		{
			int displacement = *d;	
			cout << "   displacement: " << displacement << endl;

			// look for displacement in maybe frees
			for (it2 = frees.begin(); it2 != frees.end(); ++it2)
			{
				CallGraphNode_t *node2 = it2->first;
				if (!node2 || node2->IsHellnode()) continue;
				Function_t *fn2 = node2->GetFunction();
				if (fn2 == fn) continue;
		cout << "   free function: " << fn2->GetName() << endl;
				set<int> displacements2 = it2->second;
				for (set<int>::iterator d2 = displacements2.begin(); d2 != displacements2.end(); ++d2)
				{
					int displacement2 = *d2;
		cout << "       displacement2(free): " << displacement2 << endl;
					if (displacement == displacement2)
					{
						maybeMallocsWithGlobals.insert(node);
						maybeFreesWithGlobals.insert(node2);
			cout << "     MATCH: " << fn->GetName() << " <--> " << fn2->GetName() << endl;
					}
				}
			}
		}
	}

	cout << "maybe mallocs with clustering algo: " << maybeMallocsWithGlobals.size() << endl;
	cout << "maybe frees with clustering algo: " << maybeFreesWithGlobals.size() << endl;

	m_maybeMallocs = maybeMallocsWithGlobals;
	m_maybeFrees = maybeFreesWithGlobals;
}

bool CGC_libc::isGlobalData(int p_address)
{
	for ( int i = 0; i < m_elfiop->sections.size(); ++i )
	{

        	int flags = m_elfiop->sections[i]->get_flags();
        	/* not a loaded section */
        	if( (flags & SHF_ALLOC) != SHF_ALLOC)
                	continue;
        	if( (flags & SHF_EXECINSTR) == SHF_EXECINSTR)
                	continue;

        	int first=m_elfiop->sections[i]->get_address();
        	int second=m_elfiop->sections[i]->get_address()+m_elfiop->sections[i]->get_size();
			if (p_address >= first && p_address <= second)
				return true;
	}

	return false;
}

// out: <nodes> has dominated functions erased
void CGC_libc::findDominant(CallGraphNodeSet_t& nodes)
{
	CallGraphNodeSet_t dominatedNodes;

	// for each function f
	//     A = get ancestors for each of f's immediate parent
	//     for each m in maybeMallocs
	//		if m is in all As, then m dominates f

	for (CallGraphNodeSet_t::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		CallGraphNode_t *node = *i;
		Function_t* fn = NULL;

		if (!node || node->IsHellnode())
			continue;

		fn = node->GetFunction();
		cout << "Looking for dominator of function " << fn->GetName() << endl;

		CallGraphNodeSet_t preds = m_cg.GetCallersOfNode(node);
		if (preds.size() == 0) continue;

		// check to see if immediate caller dominates
		if (preds.size() == 1)
		{
			bool yes_dominated = false;
			// make sure the caller is in <nodes>
			for (CallGraphNodeSet_t::iterator c = preds.begin(); c != preds.end(); ++c)
			{
				CallGraphNode_t *caller = *c;
				if (caller == NULL) continue;

				if (nodes.count(caller) >= 1)
					yes_dominated = true;
			}

			if (yes_dominated) {
				dominatedNodes.insert(node);
				continue;
			}
		}

		int preds_count = 0;
		CallGraphNodeSet_t ancestors[preds.size()];
		for (CallGraphNodeSet_t::iterator a = preds.begin(); a != preds.end(); ++a)
		{
			ancestors[preds_count] = m_cg.GetCallersOfNode(*a);
			ancestors[preds_count].insert(*a); 
			preds_count++;
		}

		// does m dominate fn?
		for (CallGraphNodeSet_t::iterator m = nodes.begin(); m != nodes.end(); ++m)
		{
			cout << "Does function " << (*m)->GetFunction()->GetName() << " dominate " << node->GetFunction()->GetName() << "?" << endl;
			int dominated = true;
			for (int a = 0; a < preds_count; ++a)
			{
				if (ancestors[a].count(*m) == 0)
				{
					dominated = false;
				}
			}

			if (dominated) {
				cout << "Function " << node->GetFunction()->GetName() << " is dominated by " << (*m)->GetFunction()->GetName() << endl;
				dominatedNodes.insert(node);
			}
		}
	}


	for (CallGraphNodeSet_t::iterator d = dominatedNodes.begin(); d != dominatedNodes.end(); ++d)
	{
		nodes.erase(*d);
	}
}

bool CGC_libc::execute()
{
	displayAllFunctions();

	cout << "CGC: syscall heuristic" << endl;

	findSyscallWrappers(); // finds the initial set of mallocs and frees

	displayMaybes(m_cg,m_mallocUniverse, "universe-malloc");

	cout << "CGC: prune mallocs" << endl;
	pruneMallocs();

	cout << "prune frees" << endl;
	pruneFrees();

	displayMaybes(m_cg,m_mallocUniverse, "universe-malloc-2");

	if (m_clustering)
	{
		cout << "# of candidate functions for malloc() prior to clustering: " << m_maybeMallocs.size() << endl;
		cout << "# of candidate functions for free() prior to clustering: " << m_maybeFrees.size() << endl;
		clusterFreeMalloc();
		displayMaybes(m_cg,m_maybeMallocs, "cluster-malloc");
		displayMaybes(m_cg,m_maybeFrees, "cluster-free");
	}
	else
		cout << "clustering heuristic is off" << endl;

	if (m_dominance) 
	{
		cout << "# of candidate functions for malloc() prior to domination heuristic: " << m_maybeMallocs.size() << endl;
		cout << "# of candidate functions for free(): prior to domination heuristic " << m_maybeFrees.size() << endl;
		findDominant(m_maybeMallocs);
		displayMaybes(m_cg,m_maybeMallocs, "dominator-malloc");

		findDominant(m_maybeFrees);
		displayMaybes(m_cg,m_maybeFrees, "dominator-free");
	}
	else
		cout << "domination heuristic is off" << endl;

	cout << endl << "Final summary" << endl;
	cout << "-----------------------------------------" << endl;
	cout << "Total # of functions: " << m_firp->GetFunctions().size() << endl;
	cout << "# of candidate functions for malloc(): " << m_maybeMallocs.size() << endl;
	cout << "# of candidate functions for free(): " << m_maybeFrees.size() << endl;
	cout << endl;

	displayFinalInference(m_cg,m_maybeMallocs, "malloc");
	displayFinalInference(m_cg,m_maybeFrees, "free");

	return true;
}

bool CGC_libc::renameSyscallWrappers()
{
	bool success = false;

	findSyscallWrappers(); 

	if (m__terminateWrapper && !m__terminateWrapper->IsHellnode() &&
		m__terminateWrapper->GetFunction())
	{
		cout << "renaming " << m__terminateWrapper->GetFunction()->GetName() << " to cinderella::terminate" << endl;
		m__terminateWrapper->GetFunction()->SetName("cinderella::terminate");
		success = true;
	}
		
	if (m_transmitWrapper && !m_transmitWrapper->IsHellnode() &&
		m_transmitWrapper->GetFunction())
	{
		cout << "renaming " << m_transmitWrapper->GetFunction()->GetName() << " to cinderella::transmit" << endl;
		m_transmitWrapper->GetFunction()->SetName("cinderella::transmit");
		success = true;
	}

	if (m_receiveWrapper && !m_receiveWrapper->IsHellnode() &&
		m_receiveWrapper->GetFunction())
	{
		cout << "renaming " << m_receiveWrapper->GetFunction()->GetName() << " to cinderella::receive" << endl;
		m_receiveWrapper->GetFunction()->SetName("cinderella::receive");
		success = true;
	}

	if (m_fdwaitWrapper && !m_fdwaitWrapper->IsHellnode() &&
		m_fdwaitWrapper->GetFunction())
	{
		cout << "renaming " << m_fdwaitWrapper->GetFunction()->GetName() << " to cinderella::fdwait" << endl;
		m_fdwaitWrapper->GetFunction()->SetName("cinderella::fdwait");
		success = true;
	}

	if (m_allocateWrapper && !m_allocateWrapper->IsHellnode() &&
		m_allocateWrapper->GetFunction())
	{
		cout << "renaming " << m_allocateWrapper->GetFunction()->GetName() << " to cinderella::allocate" << endl;
		m_allocateWrapper->GetFunction()->SetName("cinderella::allocate");
		success = true;
	}

	if (m_deallocateWrapper && !m_deallocateWrapper->IsHellnode() &&
		m_deallocateWrapper->GetFunction())
	{
		cout << "renaming " << m_deallocateWrapper->GetFunction()->GetName() << " to cinderella::deallocate" << endl;
		m_deallocateWrapper->GetFunction()->SetName("cinderella::deallocate");
		success = true;
	}

	if (m_randomWrapper && !m_randomWrapper->IsHellnode() &&
		m_randomWrapper->GetFunction())
	{
		cout << "renaming " << m_randomWrapper->GetFunction()->GetName() << " to cinderella::random" << endl;
		m_randomWrapper->GetFunction()->SetName("cinderella::random");
		success = true;
	}

	return success;
}
