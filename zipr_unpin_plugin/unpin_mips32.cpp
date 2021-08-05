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


#include <string>
#include <algorithm>
#include "unpin.h"
#include <memory>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <irdb-util>


using namespace IRDB_SDK;
using namespace std;
using namespace Zipr_SDK;

static inline uint32_t rotr32 (uint32_t n, unsigned int c)
{
  const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n>>c) | (n<<( (-c)&mask ));
}


#define ALLOF(a) begin(a),end(a)
// per machine stuff
void UnpinMips32_t::HandleRetAddrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }

void UnpinMips32_t::HandlePcrelReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }

void UnpinMips32_t::HandleAbsptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); } 


void UnpinMips32_t::HandleImmedptrReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }

void UnpinMips32_t::HandleCallbackReloc(Instruction_t* from_insn, Relocation_t* reloc)
{ assert(0); }




