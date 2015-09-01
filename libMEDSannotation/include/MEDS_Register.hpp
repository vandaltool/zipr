/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
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

#ifndef _MEDS_REGISTER_H
#define _MEDS_REGISTER_H

#include <string>
#include <set>

namespace MEDS_Annotation 
{

class Register {
public:
  enum RegisterName { 
	rn_UNKNOWN, 
	rn_EFLAGS, 
	rn_RIP,
	rn_EAX, rn_EBX, rn_ECX, rn_EDX, rn_ESI, rn_EDI, rn_EBP, rn_ESP, rn_R8D, rn_R9D, rn_R10D, rn_R11D, rn_R12D, rn_R13D, rn_R14D, rn_R15D, 
	rn_RAX, rn_RBX, rn_RCX, rn_RDX, rn_RBP, rn_RSP, rn_RSI, rn_RDI, rn_R8,  rn_R9,  rn_R10,  rn_R11,  rn_R12,  rn_R13,  rn_R14,  rn_R15, 
	rn_AX, rn_BX, rn_CX, rn_DX, rn_BP, rn_SP, rn_SI, rn_DI, rn_R8W, rn_R9W, rn_R10W, rn_R11W, rn_R12W, rn_R13W, rn_R14W, rn_R15W, 
	rn_AH, rn_BH, rn_CH, rn_DH, rn_SIH, rn_DIH, rn_BPH, rn_SPH, /* 'H' versions of regs only exist for lower 8 regs */ 
	rn_AL, rn_BL, rn_CL, rn_DL, rn_SIL, rn_DIL, rn_BPL, rn_SPL, rn_R8B, rn_R9B, rn_R10B, rn_R11B, rn_R12B, rn_R13B, rn_R14B, rn_R15B, 
	};
  static RegisterName getRegister(std::string);
  static RegisterName getRegister(char *str);
  static bool isValidRegister(std::string);
  static bool is64bit(RegisterName);
  static bool is32bit(RegisterName);
  static bool is16bit(RegisterName);
  static bool is8bit(RegisterName);
  static int getBitWidth(RegisterName);
  static std::string toString(RegisterName);
  static RegisterName getFreeRegister64(std::set<Register::RegisterName> p_used);
};

}

#endif
