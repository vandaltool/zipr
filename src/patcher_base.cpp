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
#include "patcher/patcher_x86.hpp"
}

#include <irdb-core>
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

#define ALLOF(a) begin(a),end(a)

using namespace IRDB_SDK;
using namespace std;
using namespace zipr;

unique_ptr<ZiprPatcherBase_t> ZiprPatcherBase_t::factory(Zipr_SDK::Zipr_t* p_parent)
{
	auto l_firp=p_parent->getFileIR();
        auto ret= l_firp->getArchitecture()->getMachineType() == admtX86_64   ?  (ZiprPatcherBase_t*)new ZiprPatcherX86_t  (p_parent) :
                  l_firp->getArchitecture()->getMachineType() == admtI386     ?  (ZiprPatcherBase_t*)new ZiprPatcherX86_t  (p_parent) :
                  l_firp->getArchitecture()->getMachineType() == admtAarch64  ?  (ZiprPatcherBase_t*)new ZiprPatcherARM64_t(p_parent) :
                  throw domain_error("Cannot init architecture");

        return unique_ptr<ZiprPatcherBase_t>(ret);
}

