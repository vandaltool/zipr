#ifndef _LIBTRANSFORM_CALLBACKS_H_
#define _LIBTRANSFORM_CALLBACKS_H_

#include <irdb-core>
#include <irdb-transform>
#include "elfio/elfio.hpp"
#include <functional>

using namespace std;
using namespace IRDB_SDK;

class Callbacks : public Transform_t
{
	public:
		Callbacks(FileIR_t *p_variantIR);
		int execute();
		~Callbacks();
		void SetCallbackFile(string);
	private:
		bool CreateCallbacksScoop();
		bool GetPltCallTarget(IRDB_SDK::Instruction_t *, VirtualOffset_t &);
		IRDB_SDK::Instruction_t *add_instrumentation(IRDB_SDK::Instruction_t *,unsigned long);
		VirtualOffset_t GetSymbolOffset(string &);
		ELFIO::Elf64_Addr ReadAddressInSectionAtOffset(ELFIO::section *,ELFIO::Elf64_Off);
		void MakeSymbolOffsetTable();
		bool LoadElf(string);
		bool IsStraightCall(Instruction_t *);
		std::unique_ptr<ELFIO::elfio> m_elfiop;
// 		std::unique_ptr<pqxx::largeobjectaccess> file_object;
		std::unique_ptr<std::map<std::string, ELFIO::Elf64_Addr>> m_symbol_offset_table;
		std::string m_callback_file;
		std::map<VirtualOffset_t, IRDB_SDK::Instruction_t*> m_indtargs;
		IRDB_SDK::DataScoop_t *callbacks_scoop;
		IRDB_SDK::FileIR_t *m_firp;
		uintptr_t m_text_offset;
};
#endif
