#ifndef _LIBTRANSFORM_HOOK_START_H_
#define _LIBTRANSFORM_HOOK_START_H_

#include "../../libtransform/include/transform.hpp"
#include "../../libMEDSannotation/include/VirtualOffset.hpp"
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <libIRDB-syscall.hpp>
#include "elfio/elfio.hpp"

using namespace std;
using namespace libIRDB;

class HookStart : public libTransform::Transform
{
	public:
		HookStart(FileIR_t*p_variantIR);
		~HookStart();

		void CallbackName(const std::string &callback_name)
		{
			m_callback_name = callback_name;
		}
		std::string CallbackName()
		{
			return m_callback_name;
		}
		int execute();
	private:
		void LoadElf();
		Instruction_t *add_instrumentation(Instruction_t *site);
		std::string m_callback_name;
		std::unique_ptr<ELFIO::elfio> m_elfiop;
		std::unique_ptr<pqxx::largeobjectaccess> m_file_object;
		ELFIO::Elf64_Addr *m_plt_addresses;
};
#endif
