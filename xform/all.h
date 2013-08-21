/*
 * all.h
 *
 * Copyright (c) 2011 - University of Virginia 
 *
 * This file is part of the STROBE (STack Rewriting of Binary Executables) project.
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#ifndef _MEDS_ANNOT_H
#define _MEDS_ANNOT_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "strata_defines.h"

/* x86_32-specific headers */
#include "targ-config.h"
#include "instrument.h"

#include "spri_alloc.h"
#include "bitvector.h"
#include "gen_hash.h"
#include "constant_hash.h"
#include "stackref_hash.h"
#include "framesize_hash.h"
#include "framerestore_hash.h"
#include "instrmap_hash.h"
#include "funclist_hash.h"
#include "constant_hash.h"

#endif
