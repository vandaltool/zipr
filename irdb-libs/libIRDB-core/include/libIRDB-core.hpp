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
#include <vector>
#include <set>
#include <assert.h>
#include <string.h>
#include <iostream>
#include <pqxx/pqxx>
#include <ctime>
#include <stdint.h>

// include SDK
#include <irdb-core>

#include <basetypes.hpp>
#include <dbinterface.hpp>
#include <doip.hpp>
#include <baseobj.hpp>
#include <reloc.hpp>
#include <address.hpp>
#include <icfs.hpp>
#include <instruction.hpp>
#include <file.hpp>
#include <function.hpp>
#include <variantid.hpp>
#include <archdesc.hpp>
#include <type.hpp>
#include <scoop.hpp>
#include <eh.hpp>
#include <fileir.hpp>
#include <pqxxdb.hpp>
#include <IRDB_Objects.hpp>
#include <decode.hpp>

int command_to_stream(const std::string& command, std::ostream& stream);


#endif
