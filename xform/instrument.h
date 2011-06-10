/*
 * instrument.h -- see instrument.c
 *
 * Copyright (c) 2000, 2001, 2010 - University of Virginia 
 *
 * This file is part of the Memory Error Detection System (MEDS) infrastructure.
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

/*
 * instrument.h -- see instrument.c
 * author - jdh8d
 */
#ifndef _instrument_h
#define _instrument_h


/*
no needed for SPRI
void add_smp_instrumentation(strata_fragment_t *frag, app_iaddr_t PC, insn_t *insn);
void add_smp_postinstrumentation(strata_fragment_t *frag, app_iaddr_t PC, insn_t *insn);
*/






/*  the registers are pushed onto the stack with "push eax; lahf, pusha" in this order */
typedef struct reg_values reg_values_t;
struct reg_values 
{
       	int edi;
       	int esi;
       	int ebp;
       	int esp_dummy;
       	int ebx;
       	int edx;
       	int ecx;
       	int eax;
       	int eflags;
};


/*
no needed for SPRI
app_iaddr_t targ_watched_called_instrument(app_iaddr_t next_PC, watch *w, strata_fragment_t *frag);
*/


/* #define NO_ANNOT_STACK */

#endif

