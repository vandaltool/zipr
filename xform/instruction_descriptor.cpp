#include <string.h>
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
//  m_data[0] = '\0';
  m_data = NULL;
}

wahoo::Instruction::Instruction(app_iaddr_t p_address, int p_size, Function* p_func)
{
  m_address = p_address;
  m_size = p_size;
  m_function = p_func;

  m_allocSite = false;
  m_deallocSite = false;
  m_stackRef = false;
 // m_data[0] = '\0';
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

/*
void wahoo::Instruction::setData(void *data, int len) 
{
  memcpy(m_data, data, len);
}
*/
