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

#include <string>
#include <sstream>
#include <strings.h>
#include <assert.h>

#include "MEDS_Register.hpp"

using namespace MEDS_Annotation;
using namespace std;

RegisterName Register::getRegister(char *p_reg)
{
	return Register::getRegister(std::string(p_reg));
}

bool Register::isValidRegister(std::string p_reg)
{
	return getRegister(p_reg) != IRDB_SDK::rn_UNKNOWN;
}

bool Register::isValidRegister(const RegisterName p_reg)
{
	return isValidRegister(Register::toString(p_reg));
}

RegisterName Register::getRegister(std::string p_reg)
{
	if (strcasecmp(p_reg.c_str(), "EFLAGS") ==0)
		return IRDB_SDK::rn_EFLAGS;
	else if (strcasecmp(p_reg.c_str(), "RAX") == 0)
		return IRDB_SDK::rn_RAX;
	else if (strcasecmp(p_reg.c_str(), "RBX") == 0)
		return IRDB_SDK::rn_RBX;
	else if (strcasecmp(p_reg.c_str(), "RCX") == 0)
		return IRDB_SDK::rn_RCX;
	else if (strcasecmp(p_reg.c_str(), "RDX") == 0)
		return IRDB_SDK::rn_RDX;
	else if (strcasecmp(p_reg.c_str(), "RSI") == 0)
		return IRDB_SDK::rn_RSI;
	else if (strcasecmp(p_reg.c_str(), "RDI") == 0)
		return IRDB_SDK::rn_RDI;
	else if (strcasecmp(p_reg.c_str(), "RBP") == 0)
		return IRDB_SDK::rn_RBP;
	else if (strcasecmp(p_reg.c_str(), "RSP") == 0)
		return IRDB_SDK::rn_RSP;
	else if (strcasecmp(p_reg.c_str(), "R8") == 0)
		return IRDB_SDK::rn_R8;
	else if (strcasecmp(p_reg.c_str(), "R9") == 0)
		return IRDB_SDK::rn_R9;
	else if (strcasecmp(p_reg.c_str(), "R10") == 0)
		return IRDB_SDK::rn_R10;
	else if (strcasecmp(p_reg.c_str(), "R11") == 0)
		return IRDB_SDK::rn_R11;
	else if (strcasecmp(p_reg.c_str(), "R12") == 0)
		return IRDB_SDK::rn_R12;
	else if (strcasecmp(p_reg.c_str(), "R13") == 0)
		return IRDB_SDK::rn_R13;
	else if (strcasecmp(p_reg.c_str(), "R14") == 0)
		return IRDB_SDK::rn_R14;
	else if (strcasecmp(p_reg.c_str(), "R15") == 0)
		return IRDB_SDK::rn_R15;
	else if (strcasecmp(p_reg.c_str(), "RIP") == 0)
		return IRDB_SDK::rn_RIP;

	else if (strcasecmp(p_reg.c_str(), "EAX") == 0)
		return IRDB_SDK::rn_EAX;
	else if (strcasecmp(p_reg.c_str(), "EBX") == 0)
		return IRDB_SDK::rn_EBX;
	else if (strcasecmp(p_reg.c_str(), "ECX") == 0)
		return IRDB_SDK::rn_ECX;
	else if (strcasecmp(p_reg.c_str(), "EDX") == 0)
		return IRDB_SDK::rn_EDX;
	else if (strcasecmp(p_reg.c_str(), "ESI") == 0)
		return IRDB_SDK::rn_ESI;
	else if (strcasecmp(p_reg.c_str(), "EDI") == 0)
		return IRDB_SDK::rn_EDI;
	else if (strcasecmp(p_reg.c_str(), "EBP") == 0)
		return IRDB_SDK::rn_EBP;
	else if (strcasecmp(p_reg.c_str(), "ESP") == 0)
		return IRDB_SDK::rn_ESP;
	else if (strcasecmp(p_reg.c_str(), "R8D") == 0)
		return IRDB_SDK::rn_R8D;
	else if (strcasecmp(p_reg.c_str(), "R9D") == 0)
		return IRDB_SDK::rn_R9D;
	else if (strcasecmp(p_reg.c_str(), "R10D") == 0)
		return IRDB_SDK::rn_R10D;
	else if (strcasecmp(p_reg.c_str(), "R11D") == 0)
		return IRDB_SDK::rn_R11D;
	else if (strcasecmp(p_reg.c_str(), "R12D") == 0)
		return IRDB_SDK::rn_R12D;
	else if (strcasecmp(p_reg.c_str(), "R13D") == 0)
		return IRDB_SDK::rn_R13D;
	else if (strcasecmp(p_reg.c_str(), "R14D") == 0)
		return IRDB_SDK::rn_R14D;
	else if (strcasecmp(p_reg.c_str(), "R15D") == 0)
		return IRDB_SDK::rn_R15D;

	else if (strcasecmp(p_reg.c_str(), "AX") == 0)
		return IRDB_SDK::rn_AX;
	else if (strcasecmp(p_reg.c_str(), "BX") == 0)
		return IRDB_SDK::rn_BX;
	else if (strcasecmp(p_reg.c_str(), "CX") == 0)
		return IRDB_SDK::rn_CX;
	else if (strcasecmp(p_reg.c_str(), "DX") == 0)
		return IRDB_SDK::rn_DX;
	else if (strcasecmp(p_reg.c_str(), "BP") == 0)
		return IRDB_SDK::rn_BP;
	else if (strcasecmp(p_reg.c_str(), "SP") == 0)
		return IRDB_SDK::rn_SP;
	else if (strcasecmp(p_reg.c_str(), "SI") == 0)
		return IRDB_SDK::rn_SI;
	else if (strcasecmp(p_reg.c_str(), "DI") == 0)
		return IRDB_SDK::rn_DI;
	else if (strcasecmp(p_reg.c_str(), "R8W") == 0)
		return IRDB_SDK::rn_R8W;
	else if (strcasecmp(p_reg.c_str(), "R9W") == 0)
		return IRDB_SDK::rn_R9W;
	else if (strcasecmp(p_reg.c_str(), "R10W") == 0)
		return IRDB_SDK::rn_R10W;
	else if (strcasecmp(p_reg.c_str(), "R11W") == 0)
		return IRDB_SDK::rn_R11W;
	else if (strcasecmp(p_reg.c_str(), "R12W") == 0)
		return IRDB_SDK::rn_R12W;
	else if (strcasecmp(p_reg.c_str(), "R13W") == 0)
		return IRDB_SDK::rn_R13W;
	else if (strcasecmp(p_reg.c_str(), "R14W") == 0)
		return IRDB_SDK::rn_R14W;
	else if (strcasecmp(p_reg.c_str(), "R15W") == 0)
		return IRDB_SDK::rn_R15W;

	else if (strcasecmp(p_reg.c_str(), "AH") == 0)
		return IRDB_SDK::rn_AL;
	else if (strcasecmp(p_reg.c_str(), "BH") == 0)
		return IRDB_SDK::rn_BL;
	else if (strcasecmp(p_reg.c_str(), "CH") == 0)
		return IRDB_SDK::rn_CL;
	else if (strcasecmp(p_reg.c_str(), "DH") == 0)
		return IRDB_SDK::rn_DL;
	else if (strcasecmp(p_reg.c_str(), "AL") == 0)
		return IRDB_SDK::rn_AL;
	else if (strcasecmp(p_reg.c_str(), "BL") == 0)
		return IRDB_SDK::rn_BL;
	else if (strcasecmp(p_reg.c_str(), "CL") == 0)
		return IRDB_SDK::rn_CL;
	else if (strcasecmp(p_reg.c_str(), "DL") == 0)
		return IRDB_SDK::rn_DL;
	else if (strcasecmp(p_reg.c_str(), "SIL") == 0)
		return IRDB_SDK::rn_SIL;
	else if (strcasecmp(p_reg.c_str(), "DIL") == 0)
		return IRDB_SDK::rn_DIL;
	else if (strcasecmp(p_reg.c_str(), "BPL") == 0)
		return IRDB_SDK::rn_BPL;
	else if (strcasecmp(p_reg.c_str(), "SPL") == 0)
		return IRDB_SDK::rn_SPL;
	else if (strcasecmp(p_reg.c_str(), "R8B") == 0)
		return IRDB_SDK::rn_R8B;
	else if (strcasecmp(p_reg.c_str(), "R9B") == 0)
		return IRDB_SDK::rn_R9B;
	else if (strcasecmp(p_reg.c_str(), "R10B") == 0)
		return IRDB_SDK::rn_R10B;
	else if (strcasecmp(p_reg.c_str(), "R11B") == 0)
		return IRDB_SDK::rn_R11B;
	else if (strcasecmp(p_reg.c_str(), "R12B") == 0)
		return IRDB_SDK::rn_R12B;
	else if (strcasecmp(p_reg.c_str(), "R13B") == 0)
		return IRDB_SDK::rn_R13B;
	else if (strcasecmp(p_reg.c_str(), "R14B") == 0)
		return IRDB_SDK::rn_R14B;
	else if (strcasecmp(p_reg.c_str(), "R15B") == 0)
		return IRDB_SDK::rn_R15B;
	else
		return IRDB_SDK::rn_UNKNOWN;
}

bool Register::is8bit(RegisterName p_reg)
{
	return p_reg == IRDB_SDK::rn_AL || p_reg == IRDB_SDK::rn_BL || p_reg == IRDB_SDK::rn_CL || p_reg == IRDB_SDK::rn_DL ||
		p_reg == IRDB_SDK::rn_AH || p_reg == IRDB_SDK::rn_BH || p_reg == IRDB_SDK::rn_CH || p_reg == IRDB_SDK::rn_DH ||
		p_reg == IRDB_SDK::rn_SIL || p_reg == IRDB_SDK::rn_DIL || p_reg == IRDB_SDK::rn_BPL || p_reg == IRDB_SDK::rn_SPL ||
		p_reg == IRDB_SDK::rn_R8B || p_reg == IRDB_SDK::rn_R9B || p_reg == IRDB_SDK::rn_R10B || p_reg == IRDB_SDK::rn_R11B ||
		p_reg == IRDB_SDK::rn_R12B || p_reg == IRDB_SDK::rn_R13B || p_reg == IRDB_SDK::rn_R14B || p_reg == IRDB_SDK::rn_R15B;
}

bool Register::is16bit(RegisterName p_reg)
{
	return p_reg == IRDB_SDK::rn_AX || p_reg == IRDB_SDK::rn_BX || p_reg == IRDB_SDK::rn_CX || p_reg == IRDB_SDK::rn_DX ||
		p_reg == IRDB_SDK::rn_BP || p_reg == IRDB_SDK::rn_SP || p_reg == IRDB_SDK::rn_SI || p_reg == IRDB_SDK::rn_DI ||
		p_reg == IRDB_SDK::rn_R8W || p_reg == IRDB_SDK::rn_R9W || p_reg == IRDB_SDK::rn_R10W || p_reg == IRDB_SDK::rn_R11W ||
		p_reg == IRDB_SDK::rn_R12W || p_reg == IRDB_SDK::rn_R13W || p_reg == IRDB_SDK::rn_R14W || p_reg == IRDB_SDK::rn_R15W;
}

bool Register::is32bit(RegisterName p_reg)
{
	return p_reg == IRDB_SDK::rn_EAX || p_reg == IRDB_SDK::rn_EBX || p_reg == IRDB_SDK::rn_ECX || p_reg == IRDB_SDK::rn_EDX || 
		p_reg == IRDB_SDK::rn_ESI || p_reg == IRDB_SDK::rn_EDI || p_reg == IRDB_SDK::rn_ESP || p_reg == IRDB_SDK::rn_EBP ||
		p_reg == IRDB_SDK::rn_R8D || p_reg == IRDB_SDK::rn_R9D || p_reg == IRDB_SDK::rn_R10D || p_reg == IRDB_SDK::rn_R11D ||
		p_reg == IRDB_SDK::rn_R12D || p_reg == IRDB_SDK::rn_R13D || p_reg == IRDB_SDK::rn_R14D || p_reg == IRDB_SDK::rn_R15D;
}

bool Register::is64bit(RegisterName p_reg)
{
	return p_reg == IRDB_SDK::rn_RAX || p_reg == IRDB_SDK::rn_RBX || p_reg == IRDB_SDK::rn_RCX || p_reg == IRDB_SDK::rn_RDX || 
		p_reg == IRDB_SDK::rn_RSI || p_reg == IRDB_SDK::rn_RDI || p_reg == IRDB_SDK::rn_RBP || p_reg == IRDB_SDK::rn_RSP ||
		p_reg == IRDB_SDK::rn_R8 || p_reg == IRDB_SDK::rn_R9 || p_reg == IRDB_SDK::rn_R10 || p_reg == IRDB_SDK::rn_R11 || 
		p_reg == IRDB_SDK::rn_R12 || p_reg == IRDB_SDK::rn_R13 || p_reg == IRDB_SDK::rn_R14 || p_reg == IRDB_SDK::rn_R15 || p_reg == IRDB_SDK::rn_RIP;
}

std::string Register::toString(RegisterName p_reg)
{
	if (p_reg == IRDB_SDK::rn_UNKNOWN) return std::string("UNKNOWN");

	else if (p_reg == IRDB_SDK::rn_EFLAGS) return std::string("EFLAGS");

	else if (p_reg == IRDB_SDK::rn_RAX) return std::string("RAX");
	else if (p_reg == IRDB_SDK::rn_RBX) return std::string("RBX");
	else if (p_reg == IRDB_SDK::rn_RCX) return std::string("RCX");
	else if (p_reg == IRDB_SDK::rn_RDX) return std::string("RDX");
	else if (p_reg == IRDB_SDK::rn_RSI) return std::string("RSI");
	else if (p_reg == IRDB_SDK::rn_RDI) return std::string("RDI");
	else if (p_reg == IRDB_SDK::rn_RBP) return std::string("RBP");
	else if (p_reg == IRDB_SDK::rn_RSP) return std::string("RSP");
	else if (p_reg == IRDB_SDK::rn_R8) return std::string("R8");
	else if (p_reg == IRDB_SDK::rn_R9) return std::string("R9");
	else if (p_reg == IRDB_SDK::rn_R10) return std::string("R10");
	else if (p_reg == IRDB_SDK::rn_R11) return std::string("R11");
	else if (p_reg == IRDB_SDK::rn_R12) return std::string("R12");
	else if (p_reg == IRDB_SDK::rn_R13) return std::string("R13");
	else if (p_reg == IRDB_SDK::rn_R14) return std::string("R14");
	else if (p_reg == IRDB_SDK::rn_R15) return std::string("R15");
	else if (p_reg == IRDB_SDK::rn_RIP) return std::string("RIP");

	else if (p_reg == IRDB_SDK::rn_EAX) return std::string("EAX");
	else if (p_reg == IRDB_SDK::rn_EBX) return std::string("EBX");
	else if (p_reg == IRDB_SDK::rn_ECX) return std::string("ECX");
	else if (p_reg == IRDB_SDK::rn_EDX) return std::string("EDX");
	else if (p_reg == IRDB_SDK::rn_EDI) return std::string("EDI");
	else if (p_reg == IRDB_SDK::rn_ESI) return std::string("ESI");
	else if (p_reg == IRDB_SDK::rn_EBP) return std::string("EBP");
	else if (p_reg == IRDB_SDK::rn_ESP) return std::string("ESP");
	else if (p_reg == IRDB_SDK::rn_R8D) return std::string("R8D");
	else if (p_reg == IRDB_SDK::rn_R9D) return std::string("R9D");
	else if (p_reg == IRDB_SDK::rn_R10D) return std::string("R10D");
	else if (p_reg == IRDB_SDK::rn_R11D) return std::string("R11D");
	else if (p_reg == IRDB_SDK::rn_R12D) return std::string("R12D");
	else if (p_reg == IRDB_SDK::rn_R13D) return std::string("R13D");
	else if (p_reg == IRDB_SDK::rn_R14D) return std::string("R14D");
	else if (p_reg == IRDB_SDK::rn_R15D) return std::string("R15D");

	else if (p_reg == IRDB_SDK::rn_AX) return std::string("AX");
	else if (p_reg == IRDB_SDK::rn_BX) return std::string("BX");
	else if (p_reg == IRDB_SDK::rn_CX) return std::string("CX");
	else if (p_reg == IRDB_SDK::rn_DX) return std::string("DX");
	else if (p_reg == IRDB_SDK::rn_BP) return std::string("BP");
	else if (p_reg == IRDB_SDK::rn_SP) return std::string("SP");
	else if (p_reg == IRDB_SDK::rn_SI) return std::string("SI");
	else if (p_reg == IRDB_SDK::rn_DI) return std::string("DI");
	else if (p_reg == IRDB_SDK::rn_R8W) return std::string("R8W");
	else if (p_reg == IRDB_SDK::rn_R9W) return std::string("R9W");
	else if (p_reg == IRDB_SDK::rn_R10W) return std::string("R10W");
	else if (p_reg == IRDB_SDK::rn_R11W) return std::string("R11W");
	else if (p_reg == IRDB_SDK::rn_R12W) return std::string("R12W");
	else if (p_reg == IRDB_SDK::rn_R13W) return std::string("R13W");
	else if (p_reg == IRDB_SDK::rn_R14W) return std::string("R14W");
	else if (p_reg == IRDB_SDK::rn_R15W) return std::string("R15W");

	else if (p_reg == IRDB_SDK::rn_AH) return std::string("AH");
	else if (p_reg == IRDB_SDK::rn_BH) return std::string("BH");
	else if (p_reg == IRDB_SDK::rn_CH) return std::string("CH");
	else if (p_reg == IRDB_SDK::rn_DH) return std::string("DH");
	else if (p_reg == IRDB_SDK::rn_AL) return std::string("AL");
	else if (p_reg == IRDB_SDK::rn_BL) return std::string("BL");
	else if (p_reg == IRDB_SDK::rn_CL) return std::string("CL");
	else if (p_reg == IRDB_SDK::rn_DL) return std::string("DL");
	else if (p_reg == IRDB_SDK::rn_SIL) return std::string("SIL");
	else if (p_reg == IRDB_SDK::rn_DIL) return std::string("DIL");
	else if (p_reg == IRDB_SDK::rn_BPL) return std::string("BPL");
	else if (p_reg == IRDB_SDK::rn_SPL) return std::string("SPL");
	else if (p_reg == IRDB_SDK::rn_R8B) return std::string("R8B");
	else if (p_reg == IRDB_SDK::rn_R9B) return std::string("R9B");
	else if (p_reg == IRDB_SDK::rn_R10B) return std::string("R10B");
	else if (p_reg == IRDB_SDK::rn_R11B) return std::string("R11B");
	else if (p_reg == IRDB_SDK::rn_R12B) return std::string("R12B");
	else if (p_reg == IRDB_SDK::rn_R13B) return std::string("R13B");
	else if (p_reg == IRDB_SDK::rn_R14B) return std::string("R14B");
	else if (p_reg == IRDB_SDK::rn_R15B) return std::string("R15B");

	else return std::string("UNEXPECTED REGISTER VALUE");
}

int Register::getBitWidth(RegisterName p_reg)
{
	switch (p_reg)
	{
		case IRDB_SDK::rn_RAX:
		case IRDB_SDK::rn_RBX:
		case IRDB_SDK::rn_RCX:
		case IRDB_SDK::rn_RDX:
		case IRDB_SDK::rn_RSI:
		case IRDB_SDK::rn_RDI:
		case IRDB_SDK::rn_RBP:
		case IRDB_SDK::rn_RSP:
		case IRDB_SDK::rn_R8:
		case IRDB_SDK::rn_R9:
		case IRDB_SDK::rn_R10:
		case IRDB_SDK::rn_R11:
		case IRDB_SDK::rn_R12:
		case IRDB_SDK::rn_R13:
		case IRDB_SDK::rn_R14:
		case IRDB_SDK::rn_R15:
		case IRDB_SDK::rn_RIP:
			return 64;
			break;
		case IRDB_SDK::rn_EAX:
		case IRDB_SDK::rn_EBX:
		case IRDB_SDK::rn_ECX:
		case IRDB_SDK::rn_EDX:
		case IRDB_SDK::rn_EDI:
		case IRDB_SDK::rn_ESI:
		case IRDB_SDK::rn_EBP:
		case IRDB_SDK::rn_ESP:
		case IRDB_SDK::rn_R8D:
		case IRDB_SDK::rn_R9D:
		case IRDB_SDK::rn_R10D:
		case IRDB_SDK::rn_R11D:
		case IRDB_SDK::rn_R12D:
		case IRDB_SDK::rn_R13D:
		case IRDB_SDK::rn_R14D:
		case IRDB_SDK::rn_R15D:
			return 32;
			break;

		case IRDB_SDK::rn_AX:				
		case IRDB_SDK::rn_BX:				
		case IRDB_SDK::rn_CX:				
		case IRDB_SDK::rn_DX:				
		case IRDB_SDK::rn_BP:				
		case IRDB_SDK::rn_SP:				
		case IRDB_SDK::rn_SI:				
		case IRDB_SDK::rn_DI:				
		case IRDB_SDK::rn_R8W:
		case IRDB_SDK::rn_R9W:
		case IRDB_SDK::rn_R10W:
		case IRDB_SDK::rn_R11W:
		case IRDB_SDK::rn_R12W:
		case IRDB_SDK::rn_R13W:
		case IRDB_SDK::rn_R14W:
		case IRDB_SDK::rn_R15W:
			return 16;
			break;

		case IRDB_SDK::rn_AH:				
		case IRDB_SDK::rn_BH:				
		case IRDB_SDK::rn_CH:				
		case IRDB_SDK::rn_DH:				
		case IRDB_SDK::rn_AL:				
		case IRDB_SDK::rn_BL:				
		case IRDB_SDK::rn_CL:				
		case IRDB_SDK::rn_DL:				
		case IRDB_SDK::rn_SIL:
		case IRDB_SDK::rn_DIL:
		case IRDB_SDK::rn_BPL:
		case IRDB_SDK::rn_SPL:
		case IRDB_SDK::rn_R8B:
		case IRDB_SDK::rn_R9B:
		case IRDB_SDK::rn_R10B:
		case IRDB_SDK::rn_R11B:
		case IRDB_SDK::rn_R12B:
		case IRDB_SDK::rn_R13B:
		case IRDB_SDK::rn_R14B:
		case IRDB_SDK::rn_R15B:
			return 8;
			break;

		default:
			return -1;
			break;
	}
}


// favor registers R10..R15
RegisterName Register::getFreeRegister64(const RegisterSet_t& p_taken)
{

#define ret_if_free4(a, b, c, d)\
	if (p_taken.count(a) == 0 && p_taken.count(b) == 0 && p_taken.count(c) == 0 && p_taken.count(d) == 0 ) \
		return a; \

#define ret_if_free5(a, b, c, d, e)\
	if (p_taken.count(a) == 0 && p_taken.count(b) == 0 && p_taken.count(c) == 0 && p_taken.count(d) == 0  && p_taken.count(e)==0) \
		return a; \

	ret_if_free4(IRDB_SDK::rn_R8, IRDB_SDK::rn_R8D, IRDB_SDK::rn_R8W, IRDB_SDK::rn_R8B);
	ret_if_free4(IRDB_SDK::rn_R9, IRDB_SDK::rn_R9D, IRDB_SDK::rn_R9W, IRDB_SDK::rn_R9B);
	ret_if_free4(IRDB_SDK::rn_R10, IRDB_SDK::rn_R10D, IRDB_SDK::rn_R10W, IRDB_SDK::rn_R10B);
	ret_if_free4(IRDB_SDK::rn_R11, IRDB_SDK::rn_R11D, IRDB_SDK::rn_R11W, IRDB_SDK::rn_R11B);
	ret_if_free4(IRDB_SDK::rn_R12, IRDB_SDK::rn_R12D, IRDB_SDK::rn_R12W, IRDB_SDK::rn_R12B);
	ret_if_free4(IRDB_SDK::rn_R13, IRDB_SDK::rn_R13D, IRDB_SDK::rn_R13W, IRDB_SDK::rn_R13B);
	ret_if_free4(IRDB_SDK::rn_R14, IRDB_SDK::rn_R14D, IRDB_SDK::rn_R14W, IRDB_SDK::rn_R14B);
	ret_if_free4(IRDB_SDK::rn_R15, IRDB_SDK::rn_R15D, IRDB_SDK::rn_R15W, IRDB_SDK::rn_R15B);
	ret_if_free5(IRDB_SDK::rn_RAX, IRDB_SDK::rn_EAX, IRDB_SDK::rn_AX, IRDB_SDK::rn_AL, IRDB_SDK::rn_AH);
	ret_if_free5(IRDB_SDK::rn_RCX, IRDB_SDK::rn_ECX, IRDB_SDK::rn_CX, IRDB_SDK::rn_CL, IRDB_SDK::rn_CH);
	ret_if_free5(IRDB_SDK::rn_RDX, IRDB_SDK::rn_EDX, IRDB_SDK::rn_DX, IRDB_SDK::rn_DL, IRDB_SDK::rn_DH);
	ret_if_free5(IRDB_SDK::rn_RBX, IRDB_SDK::rn_EBX, IRDB_SDK::rn_BX, IRDB_SDK::rn_BL, IRDB_SDK::rn_BH);
	ret_if_free5(IRDB_SDK::rn_RSI, IRDB_SDK::rn_ESI, IRDB_SDK::rn_SI, IRDB_SDK::rn_SIL, IRDB_SDK::rn_SIH);
	ret_if_free5(IRDB_SDK::rn_RDI, IRDB_SDK::rn_EDI, IRDB_SDK::rn_DI, IRDB_SDK::rn_DIL, IRDB_SDK::rn_DIH);
	ret_if_free5(IRDB_SDK::rn_RSP, IRDB_SDK::rn_ESP, IRDB_SDK::rn_SP, IRDB_SDK::rn_SPL, IRDB_SDK::rn_SPH);
	ret_if_free5(IRDB_SDK::rn_RBP, IRDB_SDK::rn_EBP, IRDB_SDK::rn_BP, IRDB_SDK::rn_BPL, IRDB_SDK::rn_BPH);
	return IRDB_SDK::rn_UNKNOWN;
}

string Register::readRegisterSet(const string &in, RegisterSet_t &out)
{
	size_t pos=in.find("ZZ");
	string regname;
	stringstream ss(in);

	while ( ss>>regname )
	{
		if( regname=="ZZ")
			return in.substr(pos+3);

		out.insert(getRegister(regname));
	}

	assert(0 && "No terminator found for register list");
}

RegisterName Register::promoteTo64(const RegisterName p_reg)
{
	if (is64bit(p_reg))
		return p_reg;

	switch (p_reg)
	{
		case IRDB_SDK::rn_AL:				
		case IRDB_SDK::rn_AH:				
		case IRDB_SDK::rn_AX:				
		case IRDB_SDK::rn_EAX:
			return IRDB_SDK::rn_RAX;
		case IRDB_SDK::rn_BL:				
		case IRDB_SDK::rn_BH:				
		case IRDB_SDK::rn_BX:				
		case IRDB_SDK::rn_EBX:
			return IRDB_SDK::rn_RBX;
		case IRDB_SDK::rn_CL:				
		case IRDB_SDK::rn_CH:				
		case IRDB_SDK::rn_CX:				
		case IRDB_SDK::rn_ECX:
			return IRDB_SDK::rn_RCX;
		case IRDB_SDK::rn_DL:				
		case IRDB_SDK::rn_DH:				
		case IRDB_SDK::rn_DX:				
		case IRDB_SDK::rn_EDX:
			return IRDB_SDK::rn_RDX;
		case IRDB_SDK::rn_DIL:
		case IRDB_SDK::rn_DI:				
		case IRDB_SDK::rn_EDI:
			return IRDB_SDK::rn_RDI;
		case IRDB_SDK::rn_SIL:
		case IRDB_SDK::rn_SI:				
		case IRDB_SDK::rn_ESI:
			return IRDB_SDK::rn_RSI;
		case IRDB_SDK::rn_BPL:
		case IRDB_SDK::rn_BP:				
		case IRDB_SDK::rn_EBP:
			return IRDB_SDK::rn_RBP;
		case IRDB_SDK::rn_SPL:
		case IRDB_SDK::rn_SP:				
		case IRDB_SDK::rn_ESP:
			return IRDB_SDK::rn_RSP;
		case IRDB_SDK::rn_R8B:
		case IRDB_SDK::rn_R8W:
		case IRDB_SDK::rn_R8D:
			return IRDB_SDK::rn_R8;
		case IRDB_SDK::rn_R9B:
		case IRDB_SDK::rn_R9W:
		case IRDB_SDK::rn_R9D:
			return IRDB_SDK::rn_R9;
		case IRDB_SDK::rn_R10B:
		case IRDB_SDK::rn_R10W:
		case IRDB_SDK::rn_R10D:
			return IRDB_SDK::rn_R10;
		case IRDB_SDK::rn_R11B:
		case IRDB_SDK::rn_R11W:
		case IRDB_SDK::rn_R11D:
			return IRDB_SDK::rn_R11;
		case IRDB_SDK::rn_R12B:
		case IRDB_SDK::rn_R12W:
		case IRDB_SDK::rn_R12D:
			return IRDB_SDK::rn_R12;
		case IRDB_SDK::rn_R13B:
		case IRDB_SDK::rn_R13W:
		case IRDB_SDK::rn_R13D:
			return IRDB_SDK::rn_R13;
		case IRDB_SDK::rn_R14B:
		case IRDB_SDK::rn_R14W:
		case IRDB_SDK::rn_R14D:
			return IRDB_SDK::rn_R14;
		case IRDB_SDK::rn_R15B:
		case IRDB_SDK::rn_R15W:
		case IRDB_SDK::rn_R15D:
			return IRDB_SDK::rn_R15;

		default:
			return IRDB_SDK::rn_UNKNOWN;
			break;
	}
}

RegisterName Register::demoteTo32(const RegisterName p_reg)
{
	if (is32bit(p_reg))
		return p_reg;

	switch (p_reg)
	{
		case IRDB_SDK::rn_RAX:   return  IRDB_SDK::rn_EAX;
		case IRDB_SDK::rn_RBX:   return  IRDB_SDK::rn_EBX;
		case IRDB_SDK::rn_RCX:   return  IRDB_SDK::rn_ECX;
		case IRDB_SDK::rn_RDX:   return  IRDB_SDK::rn_EDX;
		case IRDB_SDK::rn_RBP:   return  IRDB_SDK::rn_EBP;
		case IRDB_SDK::rn_RSP:   return  IRDB_SDK::rn_ESP;
		case IRDB_SDK::rn_RSI:   return  IRDB_SDK::rn_ESI;
		case IRDB_SDK::rn_RDI:   return  IRDB_SDK::rn_EDI;
		case  IRDB_SDK::rn_R8:   return  IRDB_SDK::rn_R8D;
		case  IRDB_SDK::rn_R9:   return  IRDB_SDK::rn_R9D;
		case IRDB_SDK::rn_R10:   return IRDB_SDK::rn_R10D;
		case IRDB_SDK::rn_R11:   return IRDB_SDK::rn_R11D;
		case IRDB_SDK::rn_R12:   return IRDB_SDK::rn_R12D;
		case IRDB_SDK::rn_R13:   return IRDB_SDK::rn_R13D;
		case IRDB_SDK::rn_R14:   return IRDB_SDK::rn_R14D;
		case IRDB_SDK::rn_R15:   return IRDB_SDK::rn_R15D;
		default:
			return IRDB_SDK::rn_UNKNOWN;
			break;
	}
}

RegisterName Register::demoteTo16(const RegisterName p_reg)
{
	if (is16bit(p_reg))
		return p_reg;

	switch (p_reg)
	{
		case IRDB_SDK::rn_RAX: case  IRDB_SDK::rn_EAX:   return IRDB_SDK::rn_AX;
		case IRDB_SDK::rn_RBX: case  IRDB_SDK::rn_EBX:   return IRDB_SDK::rn_BX;
		case IRDB_SDK::rn_RCX: case  IRDB_SDK::rn_ECX:   return IRDB_SDK::rn_CX;
		case IRDB_SDK::rn_RDX: case  IRDB_SDK::rn_EDX:   return IRDB_SDK::rn_DX;
		case IRDB_SDK::rn_RBP: case  IRDB_SDK::rn_EBP:   return IRDB_SDK::rn_BP;
		case IRDB_SDK::rn_RSP: case  IRDB_SDK::rn_ESP:   return IRDB_SDK::rn_SP;
		case IRDB_SDK::rn_RSI: case  IRDB_SDK::rn_ESI:   return IRDB_SDK::rn_SI;
		case IRDB_SDK::rn_RDI: case  IRDB_SDK::rn_EDI:   return IRDB_SDK::rn_DI;
		case  IRDB_SDK::rn_R8: case  IRDB_SDK::rn_R8D:   return IRDB_SDK::rn_R8W;
		case  IRDB_SDK::rn_R9: case  IRDB_SDK::rn_R9D:   return IRDB_SDK::rn_R9W;
		case IRDB_SDK::rn_R10: case IRDB_SDK::rn_R10D:   return IRDB_SDK::rn_R10W;
		case IRDB_SDK::rn_R11: case IRDB_SDK::rn_R11D:   return IRDB_SDK::rn_R11W;
		case IRDB_SDK::rn_R12: case IRDB_SDK::rn_R12D:   return IRDB_SDK::rn_R12W;
		case IRDB_SDK::rn_R13: case IRDB_SDK::rn_R13D:   return IRDB_SDK::rn_R13W;
		case IRDB_SDK::rn_R14: case IRDB_SDK::rn_R14D:   return IRDB_SDK::rn_R14W;
		case IRDB_SDK::rn_R15: case IRDB_SDK::rn_R15D:   return IRDB_SDK::rn_R15W;
		default:
			return IRDB_SDK::rn_UNKNOWN;
			break;
	}
}

