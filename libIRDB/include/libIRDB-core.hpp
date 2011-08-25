#ifndef libIRDB_core
#define libIRDB_core

#include <string>
#include <set>
#include <assert.h>
#include <string.h>
#include <iostream>
#include <pqxx/pqxx>
#include <beaengine/BeaEngine.h>

namespace libIRDB 
{

class VariantID_t; // forward decl for many classes

#include <core/basetypes.hpp>
#include <core/dbinterface.hpp>
#include <core/doip.hpp>
#include <core/baseobj.hpp>
#include <core/address.hpp>
#include <core/instruction.hpp>
#include <core/file.hpp>
#include <core/function.hpp>
#include <core/variantid.hpp>
#include <core/variantir.hpp>
#include <core/pqxxdb.hpp>

};

#endif
