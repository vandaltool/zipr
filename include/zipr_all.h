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

#ifndef zipr_all_h
#define zipr_all_h

#include <stdlib.h>
#include <stdint.h>
#include <set>
#include <list>
#include <map>
#include <irdb-core>
#include <irdb-transform>
#include <algorithm>
#include <memory>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

#include <zipr-sdk>
#include <zipr_options.h>

namespace zipr
{

using namespace std;
using namespace Zipr_SDK;

#define PAGE_SIZE 4096

#include <sled.h>
#include <unresolved.h>
#include <zipr_mem_space.h>
#include <plugin_man.h>
}

// trying to move the namespace zipr into the header files
#include <zipr_dollop_man.h>

namespace zipr
{
#include <zipr_utils.h>
#include <pinner/pinner_base.hpp>
#include <patcher/patcher_base.hpp>
#include <sizer/sizer_base.hpp>
#include <arch/arch_base.hpp>
#include <zipr_impl.h>
#include <zipr_optimizations.h>
#include <zipr_stats.h>
#include <elfwrite.h>
#include <ehwrite.h>

};

#include <zipr_dollop.h>

#endif
