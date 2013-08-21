/*
 * config.h - see below.
 *
 * Copyright (c) 2000, 2001, 2010 - Zephyr Software
 *
 * This file is part of the Strata dynamic code modification infrastructure.
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

/* 
 * config.h
 * target dependent types
 *
 * $Id: config.h,v 1.4.4.1 2007-05-22 19:01:04 jdh8d Exp $
 *
 */

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <stdint.h>
#include <inttypes.h>


#define MAX_OPCODE_LENGTH 15
#define MAX_PREFIX_LENGTH 4


typedef uintptr_t app_iaddr_t;

typedef unsigned char uchar;

/* Target specific integer types for statistics evaluator. */ 
typedef int                s_int32_t;
typedef unsigned           s_uint32_t;
typedef long long          s_int64_t;
typedef unsigned long long s_uint64_t;
typedef s_uint64_t 	   counter_t;

#define T_ARCH "x86_64"


#ifndef NULL
#define NULL 0
#endif

#include <elf.h>

typedef Elf64_Ehdr  IRDB_Elf_Ehdr;
typedef Elf64_Shdr  IRDB_Elf_Shdr;
typedef Elf64_Off   IRDB_Elf_Off;
typedef Elf64_Half  IRDB_Elf_Half;
typedef Elf64_Word  IRDB_Elf_Word;
typedef Elf64_Addr  IRDB_Elf_Addr;
typedef Elf64_Sym  IRDB_Elf_Sym;
typedef Elf64_Rel  IRDB_Elf_Rel;
typedef Elf64_Rela  IRDB_Elf_Rela;

#define IRDB_ELF_ST_TYPE ELF64_ST_TYPE


#endif
