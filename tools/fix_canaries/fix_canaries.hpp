#ifndef _LIBTRANSFORM_FIX_CANARIES_H_
#define _LIBTRANSFORM_FIX_CANARIES_H_

#include "../../libtransform/include/transform.hpp"
#include "../../libMEDSannotation/include/VirtualOffset.hpp"
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <libIRDB-syscall.hpp>
#include "elfio/elfio.hpp"
#include <functional>

using namespace std;
using namespace libIRDB;
class FixCanaries : public libTransform::Transform
{
	public:
		FixCanaries(FileIR_t*p_variantIR);
		int execute();
		~FixCanaries();
		void set_verbose(bool v) { m_verbose = v; }
		void set_callback(const std::string &);
	private:
		libIRDB::Instruction_t *add_instrumentation(libIRDB::Instruction_t *, const char *, const char *);
		void LoadElf();
		void FindStartAddress();
		std::unique_ptr<ELFIO::elfio> m_elfiop;
		std::unique_ptr<pqxx::largeobjectaccess> file_object;
		bool m_verbose = false;
		std::string m_callback;
		ELFIO::Elf64_Addr m_start_addr;
};
#endif
