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

#include "instruction_descriptor.h"
#include "function_descriptor.h"

wahoo::Instruction::Instruction()
{
  m_address = 0;
  m_size = -1;
  m_function = NULL;
  m_asm = "";

  m_allocSite = false;
  m_deallocSite = false;
  m_stackRef = false;
  m_varStackRef = false;
  m_isVisited = false;
  m_data = NULL;
}

wahoo::Instruction::Instruction(libIRDB::virtual_offset_t  p_address, int p_size, Function* p_func)
{
  m_address = p_address;
  m_size = p_size;
  m_function = p_func;
  m_isVisited = false;

  m_allocSite = false;
  m_deallocSite = false;
  m_stackRef = false;
  m_data = NULL;
}

wahoo::Instruction::~Instruction()
{
  m_function = NULL;
}

// warning: MEDS annotation (via IDA Pro plugin) is not perfect
//          and may miss fumctions
void wahoo::Instruction::markAllocSite() 
{ 
  m_allocSite = true; 
  if (m_function)
    m_function->addStackAllocationInstruction(this);
}

void wahoo::Instruction::markDeallocSite() 
{ 
  m_deallocSite = true; 
  if (m_function)
    m_function->addStackDeallocationInstruction(this);
}

void wahoo::Instruction::markStackRef() 
{ 
  m_stackRef = true; 
  if (m_function)
    m_function->addStackReferenceInstruction(this);
}

void wahoo::Instruction::markVarStackRef() 
{ 
  m_varStackRef = true; 
}

