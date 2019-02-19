/*
 * Copyright (c) 2014, 2015 - University of Virginia 
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

#include <iostream>

#include <irdb-core>

// #include "elfio/elfio.hpp"

#include "null_transform.h"
#include "stackref_hash.h"



using namespace wahoo;

//
// The NullTransform produces assembly SPRI rules that
// are semantically equivalent to the original instructions.
// 
// The following instructions are currently transformed:
//
//    leave
//    ret
//    sub esp, K     # when the instruction is used to allocate the stack frame 
//    push ebp
//    pop ebp
//
// This trasformation is used primarily for testing purposes.
// 

void NullTransform::rewrite()
{
  // only transform instructions contained in well-defined functions
  for (map<VirtualOffset_t, wahoo::Function*>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
  {
	  VirtualOffset_t addr = it->first;
    wahoo::Function* f = it->second;
    if (!f)
    {
      fprintf(stderr,"nulltransform: warning: NULL function data structure at pc: 0x%8p\n", (void*)addr);
      continue;
    }

    for (auto j = 0U; j < f->getInstructions().size(); ++j)
    {
      wahoo::Instruction *instr = f->getInstructions()[j];
      char buf[1024];
      sprintf(buf, "%s", instr->getAsm().c_str());
      fprintf(getAsmSpri(), "# orig: %s\n", buf);
      if (strstr(buf,"leave"))
      {
        fprintf(getAsmSpri(), "0x%8p -> .\n", (void*)instr->getAddress());
        fprintf(getAsmSpri(), ". ** leave\n");
        fprintf(getAsmSpri(), ". -> 0x%8p\n", (void*)(instr->getAddress() + instr->getSize()));
      }
      else if (strstr(buf,"ret"))
      {
        fprintf(getAsmSpri(), "0x%8p -> .\n", (void*)instr->getAddress());
        fprintf(getAsmSpri(), ". ** nop\n");
        fprintf(getAsmSpri(), ". ** ret\n");
        fprintf(getAsmSpri(), ". -> 0x%8p\n", (void*)(instr->getAddress() + instr->getSize()));
      }
      else if (instr->isAllocSite())
      {
        unsigned int allocSize;
        sscanf(buf,"sub esp , %x", &allocSize);
        fprintf(getAsmSpri(), "# alloc site: stack frame size: %u\n", allocSize);
        if (strstr(buf,"sub esp"))
        {
          fprintf(getAsmSpri(), "0x%8p -> .\n", (void*)instr->getAddress());
          fprintf(getAsmSpri(), ". ** nop\n");
          fprintf(getAsmSpri(), ". ** sub esp, 0x%x\n", allocSize + 283);
          fprintf(getAsmSpri(), ". ** add esp, 0x%x\n", 283);
          fprintf(getAsmSpri(), ". -> 0x%8p\n", (void*)(instr->getAddress() + instr->getSize()));
        }
      }
      else if (strstr(buf,"push ebp"))
      {
        fprintf(getAsmSpri(), "0x%8p -> .\n", (void*)(instr->getAddress()));
        fprintf(getAsmSpri(), ". ** push ebp\n");
        fprintf(getAsmSpri(), ". ** nop\n");
        fprintf(getAsmSpri(), ". ** add ebp, 1\n");
        fprintf(getAsmSpri(), ". ** sub ebp, 1\n");
        fprintf(getAsmSpri(), ". ** push ebp\n");
        fprintf(getAsmSpri(), ". ** pop ebp\n");
        fprintf(getAsmSpri(), ". -> 0x%8p\n", (void*)(instr->getAddress() + instr->getSize()));
      }
      else if (strstr(buf,"pop ebp"))
      {
        fprintf(getAsmSpri(), "0x%8p -> .\n", (void*)instr->getAddress());
        fprintf(getAsmSpri(), ". ** push ebp\n");
        fprintf(getAsmSpri(), ". ** pop ebp\n");
        fprintf(getAsmSpri(), ". ** nop\n");
        fprintf(getAsmSpri(), ". ** add ebp, 1\n");
        fprintf(getAsmSpri(), ". ** sub ebp, 1\n");
        fprintf(getAsmSpri(), ". ** pop ebp\n");
        fprintf(getAsmSpri(), ". ** nop\n");
        fprintf(getAsmSpri(), ". -> 0x%8p\n", (void*)(instr->getAddress() + instr->getSize()));
      }
    }
  }
}
