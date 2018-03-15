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

#include <string>
#include <cstdio>

#include "function_descriptor.h"

void wahoo::Function::_init()
{
  m_name = "";
  m_address = -1;
  m_size = -1;
  m_frameSize = 0;
  m_isSafe = false;
  m_useFP = false;
  m_outArgsRegionSize = 0;
  m_functionID = -1;
}

wahoo::Function::Function()
{
  _init();
}

wahoo::Function::Function(app_iaddr_t p_start)
{
  _init();
  setAddress(p_start);
}

wahoo::Function::Function(string p_name, app_iaddr_t p_start, int p_size)
{
  _init();
  setName(p_name);
  setAddress(p_start);
  setSize(p_size);
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
  for (auto i = 0U; i < m_allInstructions.size(); ++i)
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
