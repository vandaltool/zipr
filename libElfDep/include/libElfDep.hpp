
#include <libIRDB-core.hpp>
#include <transform.hpp>
#include <string>
#include <memory>


namespace libIRDB
{

using namespace libIRDB;
using namespace std;
using namespace libTransform;

class ElfDependencies_t : public Transform
{
	public:

		ElfDependencies_t(IRDB_SDK::FileIR_t* firp);
		void prependLibraryDepedencies(const string& libName) { transformer->prependLibraryDepedencies(libName); }
		void appendLibraryDepedencies(const string& libName)  { transformer->appendLibraryDepedencies(libName); }

		// return scoop and offset
		pair<IRDB_SDK::DataScoop_t*,int> appendGotEntry(const string &symbolName)  { return transformer->appendGotEntry(symbolName); }

		// return instruction that's the plt entry.
		IRDB_SDK::Instruction_t* appendPltEntry(const string &symbolName)  { return transformer->appendPltEntry(symbolName); }

	private:

	class ElfDependenciesBase_t : public Transform
	{
		public:
			ElfDependenciesBase_t(IRDB_SDK::FileIR_t* firp) : Transform(NULL, firp, NULL) {}
			virtual void prependLibraryDepedencies(const string &libraryName)=0;
			virtual void appendLibraryDepedencies(const string &libraryName)=0;
			virtual pair<IRDB_SDK::DataScoop_t*,int> appendGotEntry(const string &name)=0; 
			virtual IRDB_SDK::Instruction_t* appendPltEntry(const string &name)=0; 

	};

	template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
	class ElfDependenciesImpl_t : public ElfDependenciesBase_t
	{

		public:
			ElfDependenciesImpl_t(IRDB_SDK::FileIR_t* fipr);

			virtual void prependLibraryDepedencies(const string& libraryName);
			virtual void appendLibraryDepedencies(const string& libraryName);
			virtual pair<IRDB_SDK::DataScoop_t*,int> appendGotEntry(const string& name); 
			virtual IRDB_SDK::Instruction_t* appendPltEntry(const string& name); 
			

		private:
			bool add_dl_support();
			IRDB_SDK::Instruction_t* find_runtime_resolve(IRDB_SDK::DataScoop_t* gotplt_scoop);
			IRDB_SDK::DataScoop_t* add_got_entry(const std::string& name);
			bool add_libdl_as_needed_support(string libName);
			bool execute();

	};

	unique_ptr<ElfDependenciesBase_t> transformer;
};


}
