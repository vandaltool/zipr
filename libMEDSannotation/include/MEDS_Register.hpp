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
#include <irdb-util>

namespace MEDS_Annotation 
{


#if 0
enum RegisterName 
{
	IRDB_SDK::rn_UNKNOWN, 
	IRDB_SDK::rn_EFLAGS, 
	IRDB_SDK::rn_RIP,
	IRDB_SDK::rn_EAX, IRDB_SDK::rn_EBX, IRDB_SDK::rn_ECX, IRDB_SDK::rn_EDX, IRDB_SDK::rn_ESI, IRDB_SDK::rn_EDI, IRDB_SDK::rn_EBP, IRDB_SDK::rn_ESP, IRDB_SDK::rn_R8D, IRDB_SDK::rn_R9D, IRDB_SDK::rn_R10D, IRDB_SDK::rn_R11D, IRDB_SDK::rn_R12D, IRDB_SDK::rn_R13D, IRDB_SDK::rn_R14D, IRDB_SDK::rn_R15D, 
	IRDB_SDK::rn_RAX, IRDB_SDK::rn_RBX, IRDB_SDK::rn_RCX, IRDB_SDK::rn_RDX, IRDB_SDK::rn_RBP, IRDB_SDK::rn_RSP, IRDB_SDK::rn_RSI, IRDB_SDK::rn_RDI, IRDB_SDK::rn_R8,  IRDB_SDK::rn_R9,  IRDB_SDK::rn_R10,  IRDB_SDK::rn_R11,  IRDB_SDK::rn_R12,  IRDB_SDK::rn_R13,  IRDB_SDK::rn_R14,  IRDB_SDK::rn_R15, 
	IRDB_SDK::rn_AX, IRDB_SDK::rn_BX, IRDB_SDK::rn_CX, IRDB_SDK::rn_DX, IRDB_SDK::rn_BP, IRDB_SDK::rn_SP, IRDB_SDK::rn_SI, IRDB_SDK::rn_DI, IRDB_SDK::rn_R8W, IRDB_SDK::rn_R9W, IRDB_SDK::rn_R10W, IRDB_SDK::rn_R11W, IRDB_SDK::rn_R12W, IRDB_SDK::rn_R13W, IRDB_SDK::rn_R14W, IRDB_SDK::rn_R15W, 
	IRDB_SDK::rn_AH, IRDB_SDK::rn_BH, IRDB_SDK::rn_CH, IRDB_SDK::rn_DH, IRDB_SDK::rn_SIH, IRDB_SDK::rn_DIH, IRDB_SDK::rn_BPH, IRDB_SDK::rn_SPH, /* 'H' versions of regs only exist for lower 8 regs */ 
	IRDB_SDK::rn_AL, IRDB_SDK::rn_BL, IRDB_SDK::rn_CL, IRDB_SDK::rn_DL, IRDB_SDK::rn_SIL, IRDB_SDK::rn_DIL, IRDB_SDK::rn_BPL, IRDB_SDK::rn_SPL, IRDB_SDK::rn_R8B, IRDB_SDK::rn_R9B, IRDB_SDK::rn_R10B, IRDB_SDK::rn_R11B, IRDB_SDK::rn_R12B, IRDB_SDK::rn_R13B, IRDB_SDK::rn_R14B, IRDB_SDK::rn_R15B, 
};

typedef std::set<RegisterName> RegisterSet_t;
#endif
using RegisterName   = IRDB_SDK::RegisterID;
using RegisterName_t = IRDB_SDK::RegisterID_t;
using RegisterSet_t  = std::set<RegisterName_t>;




class Register 
{

	public:
  		static RegisterName getRegister(std::string);
  		static RegisterName getRegister(char *str);
  		static bool isValidRegister(std::string);
  		static bool isValidRegister(const RegisterName);
  		static bool is64bit(RegisterName);
  		static bool is32bit(RegisterName);
  		static bool is16bit(RegisterName);
  		static bool is8bit(RegisterName);
  		static int getBitWidth(RegisterName);
  		static std::string toString(RegisterName);
  		static RegisterName getFreeRegister64(const RegisterSet_t &p_used);
  		static std::string readRegisterSet(const std::string &in, RegisterSet_t &out);
		static RegisterName promoteTo64(const RegisterName p_reg);
		static RegisterName demoteTo32(const RegisterName p_reg);
		static RegisterName demoteTo16(const RegisterName p_reg);

};

}

#endif
