#ifndef _LIBTRANSFORM_ABSOLUTIFY_H_
#define _LIBTRANSFORM_ABSOLUTIFY_H_

#include "../../libtransform/include/transform.hpp"
#include "../../libMEDSannotation/include/VirtualOffset.hpp"
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <libIRDB-syscall.hpp>
#include "elfio/elfio.hpp"

using namespace std;
using namespace libIRDB;

class Absolutify : public libTransform::Transform
{
	public:
		Absolutify(FileIR_t*p_variantIR);
		~Absolutify();
		int execute();
		std::unique_ptr<ELFIO::elfio> m_elfiop;
		std::unique_ptr<pqxx::largeobjectaccess> file_object;
};
#endif
