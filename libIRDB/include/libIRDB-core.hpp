/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

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
class File_t; // forward decl for many classes
class Instruction_t; // forward decl for many classes

#include <core/basetypes.hpp>
#include <core/dbinterface.hpp>
#include <core/doip.hpp>
#include <core/baseobj.hpp>
#include <core/reloc.hpp>
#include <core/address.hpp>
// xxx #include <core/instructioncfg.hpp>
#include <core/icfs.hpp>
#include <core/instruction.hpp>
#include <core/file.hpp>
#include <core/function.hpp>
#include <core/variantid.hpp>
#include <core/archdesc.hpp>
#include <core/type.hpp>
#include <core/scoop.hpp>
#include <core/fileir.hpp>
#include <core/pqxxdb.hpp>

};

#endif
