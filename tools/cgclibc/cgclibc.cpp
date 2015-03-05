#include <set>
#include <iostream>

#include "cgclibc.hpp"

using namespace std;

void CGC_libc::emitFunctionInfo(Function_t *p_fn)
{
	if (p_fn && p_fn->GetEntryPoint() && p_fn->GetEntryPoint()->GetAddress())
	{
		cout << "function 0x" << hex << p_fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() << dec << " " << p_fn->GetName() << endl;
	}
}

// format of file
//    <test> positive <function> <address>
void CGC_libc::setPositiveInferences(std::string p_positiveFile)
{
	cerr << "Need to reimplement" << endl;
	/*
	ifstream pin(p_positiveFile.c_str(), std::ifstream::in);
	while (!pin.eof())
	{
		char buf[2024];
		char libcFn[2024];
		Function_t* entryPoint;

		if (pin.getline(buf, 2000))
		{
			sscanf(buf,"%*s %*s %s %p", libcFn, &entryPoint); 
			cout << "fn: " << libcFn << " " << entryPoint << endl;

			if (strcmp(libcFn,"malloc")==0)
			{
				set<Function_t*>::iterator it;
				for (it = m_firp->GetFunctions().begin(); it != m_firp->GetFunctions().end(); ++it)
				{
					Function_t *fn = *it;
					if (fn && fn->GetEntryPoint() && fn->GetEntryPoint()->GetAddress() && fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() == (uintptr_t) entryPoint)
					{
						m_mallocUniverse.insert(fn);
					}
				}
			} 
		}

	}

	pin.close();
	*/
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
		cout << "static positive " << str << " 0x" << hex << p_fn->GetEntryPoint()->GetAddress()->GetVirtualOffset() << dec << " " << p_fn->GetName() << endl;
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

	set<SyscallSite_t>::iterator it;
	for (it = sites.begin(); it != sites.end(); ++it)
	{
		SyscallSite_t site = *it;
		Function_t* fn = site.GetSite()->GetFunction();

		if (!fn) continue;

		switch (site.GetSyscallNumber()) 
		{
			case SNT_terminate:
				m__terminateWrapper = m_cg.FindNode(fn);
cout << "m__terminateWrapper found: " << fn->GetName() << endl;
				break;
			case SNT_transmit:
				m_transmitWrapper = m_cg.FindNode(fn);
cout << "m_transmitWrapper found: " << fn->GetName() << endl;
				break;
			case SNT_receive:
				m_receiveWrapper = m_cg.FindNode(fn);
cout << "m_receiveWrapper found: " << fn->GetName() << endl;
				break;
			case SNT_fdwait:
				m_fdwaitWrapper = m_cg.FindNode(fn);
cout << "m_fdwaitWrapper found: " << fn->GetName() << endl;
				break;
			case SNT_allocate:
				m_allocateWrapper = m_cg.FindNode(fn);
				m_cg.GetAncestors(m_allocateWrapper, m_maybeMallocs, m_skipHellNode);
cout << "m_allocateWrapper found: " << fn->GetName() << endl;
				break;
			case SNT_deallocate:
				m_deallocateWrapper = m_cg.FindNode(fn);
				m_cg.GetAncestors(m_deallocateWrapper, m_maybeFrees, m_skipHellNode);
cout << "m_deallocateWrapper found: " << fn->GetName() << endl;
	displayMaybes(m_cg,m_maybeFrees, "off-the-bat: free()");

				break;
			case SNT_random:
				m_randomWrapper = m_cg.FindNode(fn);
cout << "m_randomWrapper found: " << fn->GetName() << endl;
				break;
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
//	set<Function_t*> t = m_maybeMallocs; 

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

#ifdef NOT_NECESSARY
		if (m_startNode) 
		{
			// CGC-hardwired rule: _start calls main and _terminate
			if (m_cg.GetCallersOfNode(fn).count(m_startNode) > 0 && m_cg.GetCalleesOfNode(m_startNode).size() == 2)
			{
	cout << "Directly reachable from _start, should be main() so not malloc(): _start calls " << m_cg.GetCalleesOfNode(m_startNode).size() << " functions" << endl;
				m_maybeMallocs.erase(fn);
				continue;
			}
		
			if (!m_cg.Reachable(m_startNode, fn, m_skipHellNode))
			{
				cout << "Unreachable node" << endl;
				m_maybeMallocs.erase(fn);
				continue;
			}
			else
				cout << "Reachable node" << endl;
		}
#endif
	}

	for (CallGraphNodeSet_t::iterator i = m_maybeMallocs.begin(); i != m_maybeMallocs.end(); ++i)
	{
		CallGraphNode_t *node = *i;
		cout << "Maybe malloc: " << m_cg.GetNodeName(node) << endl;
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

/*
		if (m_startNode)
		{
			// CGC-hardwired rule: _start calls main and _terminate
			if (m_cg.GetCallersOfNode(fn).count(m_startNode) > 0 && m_cg.GetCalleesOfNode(m_startNode).size() == 2)
			{
	cout << "Directly reachable from _start, should be main() so not free()" << endl;
				m_maybeFrees.erase(fn);
				continue;
			}

			if (!m_cg.Reachable(m_startNode, fn, m_skipHellNode))
			{
				m_maybeFrees.erase(fn);
				continue;
			}
		}
*/
	}

	for (i = m_maybeFrees.begin(); i != m_maybeFrees.end(); ++i)
	{
		CallGraphNode_t *node = *i;
		if (!node) {
			continue;
		}
		cout << "Maybe free(): " << m_cg.GetNodeName(node) << endl;
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

void CGC_libc::findDominant(set<Function_t*> &nodes)
{
#ifdef TODO
	set<Function_t*> dominatedFunctions;

	// for each function f
	//     A = get ancestors for each of f's immediate parent
	//     for each m in maybeMallocs
	//		if m is in all As, then m dominates f

	for (set<Function_t*>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		Function_t *fn = *i;
		if (!fn) continue;

		cout << "Looking for dominator of function " << fn->GetName() << endl;

		set<Function_t*> preds = m_cg.GetCallersOfNode(fn);
		if (preds.size() < 1) continue;

		// check to see if immediate caller dominates (ignore HELLNODE if needed)
		if (preds.size() == 1 || (preds.size() == 2 && preds.count(m_hellNode) == 1))
		{
			bool yes_dominated = false;
			// make sure the caller is in <nodes>
			for (set<Function_t*>::iterator c = preds.begin(); c != preds.end(); ++c)
			{
				Function_t *caller = *c;
				if (caller == NULL) continue;

				if (nodes.count(caller) >= 1)
					yes_dominated = true;

			}

			if (yes_dominated) {
				dominatedFunctions.insert(fn);
				continue;
			}
		}

/*
*** unstable do not use ***
		int preds_count = 0;
		set<Function_t*> ancestors[preds.size()];
		for (set<Function_t*>::iterator a = preds.begin(); a != preds.end(); ++a)
		{
			ancestors[preds_count] = m_cg.GetCallersOfNode(*a);
			ancestors[preds_count].insert(*a); 
			preds_count++;
		}

		// does m dominate fn?
		for (set<Function_t*>::iterator m = nodes.begin(); m != nodes.end(); ++m)
		{
			cout << "Does function " << (*m)->GetName() << " dominate " << fn->GetName() << "?" << endl;
			int dominated = true;
			for (int a = 0; a < preds_count; ++a)
			{
				if (ancestors[a].count(*m) == 0)
				{
					dominated = false;
				}
			}

			if (dominated) {
				cout << "Function " << fn->GetName() << " is dominated by " << (*m)->GetName() << endl;
				dominatedFunctions.insert(fn);
			}
		}
*/
	}


	for (set<Function_t*>::iterator d = dominatedFunctions.begin(); d != dominatedFunctions.end(); ++d)
	{
		nodes.erase(*d);
	}
#endif
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

#ifdef TODO
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
#endif

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
