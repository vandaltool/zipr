#ifndef cgc_libc_h
#define cgc_libc_h

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <libIRDB-syscall.hpp>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"


using namespace libIRDB;

class CGC_libc {
	public:
		CGC_libc(FileIR_t *p_firp);
		void setPositiveInferences(std::string p_positiveFile);
		bool execute();

		void displayAllFunctions();
		void enableClusteringHeuristic() { m_clustering = true; }
		void enableDominanceHeuristic() { m_dominance = true; }

	private:
		void findSyscallWrappers();
		void findPotentialMallocs();
		void findPotentialFrees();
		void emitFunctionInfo(Function_t *);
		bool hasMallocFunctionPrototype(Function_t *p_fn);
		bool hasFreeFunctionPrototype(Function_t *p_fn);
		void clusterFreeMalloc();
		bool isGlobalData(int p_address);
		void findDominant(std::set<Function_t*> &);

	private:
		FileIR_t *m_firp;
		Callgraph_t m_cg;
		Syscalls_t m_syscalls;
		Function_t* m__terminateWrapper;
		Function_t* m_transmitWrapper;
		Function_t* m_receiveWrapper;
		Function_t* m_fdwaitWrapper;
		Function_t* m_allocateWrapper;
		Function_t* m_deallocateWrapper;
		Function_t* m_randomWrapper;
		std::set<Function_t*> m_maybeMallocs;
		std::set<Function_t*> m_maybeFrees;
		std::set<Function_t*> m_mallocUniverse;
		Function_t* m_startNode;
		Function_t* m_hellNode;
		bool m_skipHellNode;
		bool m_clustering;
		bool m_dominance;
		ELFIO::elfio* m_elfiop;
};

#endif
