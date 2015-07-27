#ifndef _LIBTRANSFORM_HOOK_DYNAMIC_CALLS_H_
#define _LIBTRANSFORM_HOOK_DYNAMIC_CALLS_H_

#include "../../libtransform/include/transform.hpp"
#include "../../libMEDSannotation/include/VirtualOffset.hpp"
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <libIRDB-syscall.hpp>
#include "elfio/elfio.hpp"
#include <functional>

using namespace std;
using namespace libIRDB;

class Finally
{
	public:
		Finally(std::function<void (void)> finally) : m_finally(finally) {}
		~Finally() { m_finally(); };
	private:
		std::function<void (void)> m_finally;
};

class HookDynamicCalls : public libTransform::Transform
{
	public:
		HookDynamicCalls(FileIR_t*p_variantIR);
		int execute();
		~HookDynamicCalls();
		void SetToHook(std::map<std::string,int> to_hook);
	private:
		libIRDB::Instruction_t *add_instrumentation(libIRDB::Instruction_t *,unsigned long);
		virtual_offset_t GetSymbolOffset(string &);
		ELFIO::Elf64_Addr ReadAddressInSectionAtOffset(ELFIO::section *,ELFIO::Elf64_Off);
		void LoadPltIndexTable();
		void MakeSymbolOffsetTable();
		void LoadElf();
		std::unique_ptr<ELFIO::elfio> m_elfiop;
		std::unique_ptr<pqxx::largeobjectaccess> file_object;
		ELFIO::Elf64_Addr *m_plt_addresses;
		std::unique_ptr<std::map<std::string, ELFIO::Elf64_Addr>> m_symbol_offset_table;
		std::map<std::string,int> m_to_hook;

};
#endif
