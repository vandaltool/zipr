/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 * 
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#include <zipr_all.h>
namespace zipr
{
#include "patcher/patcher_arm64.hpp"
}
#include <libIRDB-core.hpp>
#include <Rewrite_Utility.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <fstream>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "targ-config.h"
//#include <bea_deprecated.hpp>

#define ALLOF(a) begin(a),end(a)

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;
using namespace IRDBUtility;

ZiprPatcherARM64_t::ZiprPatcherARM64_t(Zipr_SDK::Zipr_t* p_parent)
{ assert(0); }
void ZiprPatcherARM64_t::ApplyNopToPatch(RangeAddress_t addr)
{ assert(0); }
void ZiprPatcherARM64_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{ assert(0); }
void ZiprPatcherARM64_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{ assert(0); }


