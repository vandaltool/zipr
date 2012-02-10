#include <string>
#include <cstdio>

#include "function_descriptor.h"

wahoo::Function::Function()
{
  m_name = "";
  m_address = -1;
  m_size = -1;
  m_isSafe = false;
  m_useFP = false;
  m_outArgsRegionSize = 0;
  m_functionID = -1;
}

wahoo::Function::Function(string p_name, app_iaddr_t p_start, int p_size)
{
  m_name = p_name;
  m_address = p_start;
  m_size = p_size;
  m_isSafe = false;
  m_useFP = false;
  m_outArgsRegionSize = 0;
  m_functionID = -1;
}

wahoo::Function::~Function()
{

}

bool wahoo::Function::operator == (const Function &other)
{
  return (other.m_name == this->m_name && other.m_address == this->m_address);
}

bool wahoo::Function::operator == (const app_iaddr_t p_addr)
{
  return (this->m_address == p_addr);
}

bool wahoo::Function::operator != (const Function &other)
{
  return (other.m_name != this->m_name || other.m_address != this->m_address);
}

bool wahoo::Function::operator != (const app_iaddr_t p_addr)
{
  return (this->m_address != p_addr);
}

void wahoo::Function::addInstruction(wahoo::Instruction* p_instr)
{
  m_allInstructions.push_back(p_instr);
}

void wahoo::Function::addStackAllocationInstruction(wahoo::Instruction* p_instr)
{
  m_stackAllocInstructions.push_back(p_instr);
}

void wahoo::Function::addStackDeallocationInstruction(wahoo::Instruction* p_instr)
{
  m_stackDeallocInstructions.push_back(p_instr);
}

void wahoo::Function::addStackReferenceInstruction(wahoo::Instruction* p_instr)
{
  m_stackRefsInstructions.push_back(p_instr);
}

vector<wahoo::Instruction*> wahoo::Function::getInstructions()
{
  return m_allInstructions;
}

vector<wahoo::Instruction*> wahoo::Function::getStackAllocationInstructions()
{
  return m_stackAllocInstructions;
}

vector<wahoo::Instruction*> wahoo::Function::getStackDeallocationInstructions()
{
  return m_stackDeallocInstructions;
}

vector<wahoo::Instruction*> wahoo::Function::getStackReferencesInstructions()
{
  return m_stackRefsInstructions;
}

double wahoo::Function::getInstructionCoverage(int *p_count, int *p_total)
{
  unsigned count = 0;
  if (m_allInstructions.size() == 0)
    return 0.0;
  for (int i = 0; i < m_allInstructions.size(); ++i)
  {
    if (m_allInstructions[i]->isVisited())
      count++;
  } 
 
  *p_count = count;
  *p_total = m_allInstructions.size();
  return (double) count / (double) m_allInstructions.size();
}

double wahoo::Function::getInstructionCoverage()
{
  int count, total;
  return getInstructionCoverage(&count, &total);
}
