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
  enum RegisterName { UNKNOWN, EFLAGS, EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, AX, BX, CX, DX, BP, SP, SI, DI, AH, BH, CH, DH, AL, BL, CL, DL, RAX, RBX, RCX, RDX, RBP, RSP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D, R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W, SIL, DIL, BPL, SPL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B };
  static RegisterName getRegister(std::string);
  static RegisterName getRegister(char *str);
  static bool is64bit(RegisterName);
  static bool is32bit(RegisterName);
  static bool is16bit(RegisterName);
  static bool is8bit(RegisterName);
  static int getBitWidth(RegisterName);
  static std::string toString(RegisterName);
  static Register::RegisterName getFreeRegister64(std::set<Register::RegisterName> p_used);
};

}

#endif
