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
#include <strings.h>

#include "MEDS_Register.hpp"

using namespace MEDS_Annotation;

Register::RegisterName Register::getRegister(char *p_reg)
{
	return Register::getRegister(std::string(p_reg));
}

bool Register::isValidRegister(std::string p_reg)
{
	return getRegister(p_reg) != rn_UNKNOWN;
}

Register::RegisterName Register::getRegister(std::string p_reg)
{
	if (strcasecmp(p_reg.c_str(), "EFLAGS") ==0)
		return rn_EFLAGS;
	else if (strcasecmp(p_reg.c_str(), "RAX") == 0)
		return rn_RAX;
	else if (strcasecmp(p_reg.c_str(), "RBX") == 0)
		return rn_RBX;
	else if (strcasecmp(p_reg.c_str(), "RCX") == 0)
		return rn_RCX;
	else if (strcasecmp(p_reg.c_str(), "RDX") == 0)
		return rn_RDX;
	else if (strcasecmp(p_reg.c_str(), "RSI") == 0)
		return rn_RSI;
	else if (strcasecmp(p_reg.c_str(), "RDI") == 0)
		return rn_RDI;
	else if (strcasecmp(p_reg.c_str(), "RBP") == 0)
		return rn_RBP;
	else if (strcasecmp(p_reg.c_str(), "RSP") == 0)
		return rn_RSP;
	else if (strcasecmp(p_reg.c_str(), "R8") == 0)
		return rn_R8;
	else if (strcasecmp(p_reg.c_str(), "R9") == 0)
		return rn_R9;
	else if (strcasecmp(p_reg.c_str(), "R10") == 0)
		return rn_R10;
	else if (strcasecmp(p_reg.c_str(), "R11") == 0)
		return rn_R11;
	else if (strcasecmp(p_reg.c_str(), "R12") == 0)
		return rn_R12;
	else if (strcasecmp(p_reg.c_str(), "R13") == 0)
		return rn_R13;
	else if (strcasecmp(p_reg.c_str(), "R14") == 0)
		return rn_R14;
	else if (strcasecmp(p_reg.c_str(), "R15") == 0)
		return rn_R15;
	else if (strcasecmp(p_reg.c_str(), "RIP") == 0)
		return rn_RIP;

	else if (strcasecmp(p_reg.c_str(), "EAX") == 0)
		return rn_EAX;
	else if (strcasecmp(p_reg.c_str(), "EBX") == 0)
		return rn_EBX;
	else if (strcasecmp(p_reg.c_str(), "ECX") == 0)
		return rn_ECX;
	else if (strcasecmp(p_reg.c_str(), "EDX") == 0)
		return rn_EDX;
	else if (strcasecmp(p_reg.c_str(), "ESI") == 0)
		return rn_ESI;
	else if (strcasecmp(p_reg.c_str(), "EDI") == 0)
		return rn_EDI;
	else if (strcasecmp(p_reg.c_str(), "EBP") == 0)
		return rn_EBP;
	else if (strcasecmp(p_reg.c_str(), "ESP") == 0)
		return rn_ESP;
	else if (strcasecmp(p_reg.c_str(), "R8D") == 0)
		return rn_R8D;
	else if (strcasecmp(p_reg.c_str(), "R9D") == 0)
		return rn_R9D;
	else if (strcasecmp(p_reg.c_str(), "R10D") == 0)
		return rn_R10D;
	else if (strcasecmp(p_reg.c_str(), "R11D") == 0)
		return rn_R11D;
	else if (strcasecmp(p_reg.c_str(), "R12D") == 0)
		return rn_R12D;
	else if (strcasecmp(p_reg.c_str(), "R13D") == 0)
		return rn_R13D;
	else if (strcasecmp(p_reg.c_str(), "R14D") == 0)
		return rn_R14D;
	else if (strcasecmp(p_reg.c_str(), "R15D") == 0)
		return rn_R15D;

	else if (strcasecmp(p_reg.c_str(), "AX") == 0)
		return rn_AX;
	else if (strcasecmp(p_reg.c_str(), "BX") == 0)
		return rn_BX;
	else if (strcasecmp(p_reg.c_str(), "CX") == 0)
		return rn_CX;
	else if (strcasecmp(p_reg.c_str(), "DX") == 0)
		return rn_DX;
	else if (strcasecmp(p_reg.c_str(), "BP") == 0)
		return rn_BP;
	else if (strcasecmp(p_reg.c_str(), "SP") == 0)
		return rn_SP;
	else if (strcasecmp(p_reg.c_str(), "SI") == 0)
		return rn_SI;
	else if (strcasecmp(p_reg.c_str(), "DI") == 0)
		return rn_DI;
	else if (strcasecmp(p_reg.c_str(), "R8W") == 0)
		return rn_R8W;
	else if (strcasecmp(p_reg.c_str(), "R9W") == 0)
		return rn_R9W;
	else if (strcasecmp(p_reg.c_str(), "R10W") == 0)
		return rn_R10W;
	else if (strcasecmp(p_reg.c_str(), "R11W") == 0)
		return rn_R11W;
	else if (strcasecmp(p_reg.c_str(), "R12W") == 0)
		return rn_R12W;
	else if (strcasecmp(p_reg.c_str(), "R13W") == 0)
		return rn_R13W;
	else if (strcasecmp(p_reg.c_str(), "R14W") == 0)
		return rn_R14W;
	else if (strcasecmp(p_reg.c_str(), "R15W") == 0)
		return rn_R15W;

	else if (strcasecmp(p_reg.c_str(), "AH") == 0)
		return rn_AL;
	else if (strcasecmp(p_reg.c_str(), "BH") == 0)
		return rn_BL;
	else if (strcasecmp(p_reg.c_str(), "CH") == 0)
		return rn_CL;
	else if (strcasecmp(p_reg.c_str(), "DH") == 0)
		return rn_DL;
	else if (strcasecmp(p_reg.c_str(), "AL") == 0)
		return rn_AL;
	else if (strcasecmp(p_reg.c_str(), "BL") == 0)
		return rn_BL;
	else if (strcasecmp(p_reg.c_str(), "CL") == 0)
		return rn_CL;
	else if (strcasecmp(p_reg.c_str(), "DL") == 0)
		return rn_DL;
	else if (strcasecmp(p_reg.c_str(), "SIL") == 0)
		return rn_SIL;
	else if (strcasecmp(p_reg.c_str(), "DIL") == 0)
		return rn_DIL;
	else if (strcasecmp(p_reg.c_str(), "BPL") == 0)
		return rn_BPL;
	else if (strcasecmp(p_reg.c_str(), "SPL") == 0)
		return rn_SPL;
	else if (strcasecmp(p_reg.c_str(), "R8B") == 0)
		return rn_R8B;
	else if (strcasecmp(p_reg.c_str(), "R9B") == 0)
		return rn_R9B;
	else if (strcasecmp(p_reg.c_str(), "R10B") == 0)
		return rn_R10B;
	else if (strcasecmp(p_reg.c_str(), "R11B") == 0)
		return rn_R11B;
	else if (strcasecmp(p_reg.c_str(), "R12B") == 0)
		return rn_R12B;
	else if (strcasecmp(p_reg.c_str(), "R13B") == 0)
		return rn_R13B;
	else if (strcasecmp(p_reg.c_str(), "R14B") == 0)
		return rn_R14B;
	else if (strcasecmp(p_reg.c_str(), "R15B") == 0)
		return rn_R15B;
	else
		return rn_UNKNOWN;
}

bool Register::is8bit(Register::RegisterName p_reg)
{
	return p_reg == rn_AL || p_reg == rn_BL || p_reg == rn_CL || p_reg == rn_DL ||
		p_reg == rn_AH || p_reg == rn_BH || p_reg == rn_CH || p_reg == rn_DH ||
		p_reg == rn_SIL || p_reg == rn_DIL || p_reg == rn_BPL || p_reg == rn_SPL ||
		p_reg == rn_R8B || p_reg == rn_R9B || p_reg == rn_R10B || p_reg == rn_R11B ||
		p_reg == rn_R12B || p_reg == rn_R13B || p_reg == rn_R14B || p_reg == rn_R15B;
}

bool Register::is16bit(Register::RegisterName p_reg)
{
	return p_reg == rn_AX || p_reg == rn_BX || p_reg == rn_CX || p_reg == rn_DX ||
		p_reg == rn_BP || p_reg == rn_SP || p_reg == rn_SI || p_reg == rn_DI ||
		p_reg == rn_R8W || p_reg == rn_R9W || p_reg == rn_R10W || p_reg == rn_R11W ||
		p_reg == rn_R12W || p_reg == rn_R13W || p_reg == rn_R14W || p_reg == rn_R15W;
}

bool Register::is32bit(Register::RegisterName p_reg)
{
	return p_reg == rn_EAX || p_reg == rn_EBX || p_reg == rn_ECX || p_reg == rn_EDX || 
		p_reg == rn_ESI || p_reg == rn_EDI || p_reg == rn_ESP || p_reg == rn_EBP ||
		p_reg == rn_R8D || p_reg == rn_R9D || p_reg == rn_R10D || p_reg == rn_R11D ||
		p_reg == rn_R12D || p_reg == rn_R13D || p_reg == rn_R14D || p_reg == rn_R15D;
}

bool Register::is64bit(Register::RegisterName p_reg)
{
	return p_reg == rn_RAX || p_reg == rn_RBX || p_reg == rn_RCX || p_reg == rn_RDX || 
		p_reg == rn_RSI || p_reg == rn_RDI || p_reg == rn_RBP || p_reg == rn_RSP ||
		p_reg == rn_R8 || p_reg == rn_R9 || p_reg == rn_R10 || p_reg == rn_R11 || 
		p_reg == rn_R12 || p_reg == rn_R13 || p_reg == rn_R14 || p_reg == rn_R15 || p_reg == rn_RIP;
}

std::string Register::toString(Register::RegisterName p_reg)
{
	if (p_reg == rn_UNKNOWN) return std::string("UNKNOWN");

	else if (p_reg == rn_EFLAGS) return std::string("EFLAGS");

	else if (p_reg == rn_RAX) return std::string("RAX");
	else if (p_reg == rn_RBX) return std::string("RBX");
	else if (p_reg == rn_RCX) return std::string("RCX");
	else if (p_reg == rn_RDX) return std::string("RDX");
	else if (p_reg == rn_RSI) return std::string("RSI");
	else if (p_reg == rn_RDI) return std::string("RDI");
	else if (p_reg == rn_RBP) return std::string("RBP");
	else if (p_reg == rn_RSP) return std::string("RSP");
	else if (p_reg == rn_R8) return std::string("R8");
	else if (p_reg == rn_R9) return std::string("R9");
	else if (p_reg == rn_R10) return std::string("R10");
	else if (p_reg == rn_R11) return std::string("R11");
	else if (p_reg == rn_R12) return std::string("R12");
	else if (p_reg == rn_R13) return std::string("R13");
	else if (p_reg == rn_R14) return std::string("R14");
	else if (p_reg == rn_R15) return std::string("R15");
	else if (p_reg == rn_RIP) return std::string("RIP");

	else if (p_reg == rn_EAX) return std::string("EAX");
	else if (p_reg == rn_EBX) return std::string("EBX");
	else if (p_reg == rn_ECX) return std::string("ECX");
	else if (p_reg == rn_EDX) return std::string("EDX");
	else if (p_reg == rn_EDI) return std::string("EDI");
	else if (p_reg == rn_ESI) return std::string("ESI");
	else if (p_reg == rn_EBP) return std::string("EBP");
	else if (p_reg == rn_ESP) return std::string("ESP");
	else if (p_reg == rn_R8D) return std::string("R8D");
	else if (p_reg == rn_R9D) return std::string("R9D");
	else if (p_reg == rn_R10D) return std::string("R10D");
	else if (p_reg == rn_R11D) return std::string("R11D");
	else if (p_reg == rn_R12D) return std::string("R12D");
	else if (p_reg == rn_R13D) return std::string("R13D");
	else if (p_reg == rn_R14D) return std::string("R14D");
	else if (p_reg == rn_R15D) return std::string("R15D");

	else if (p_reg == rn_AX) return std::string("AX");
	else if (p_reg == rn_BX) return std::string("BX");
	else if (p_reg == rn_CX) return std::string("CX");
	else if (p_reg == rn_DX) return std::string("DX");
	else if (p_reg == rn_BP) return std::string("BP");
	else if (p_reg == rn_SP) return std::string("SP");
	else if (p_reg == rn_SI) return std::string("SI");
	else if (p_reg == rn_DI) return std::string("DI");
	else if (p_reg == rn_R8W) return std::string("R8W");
	else if (p_reg == rn_R9W) return std::string("R9W");
	else if (p_reg == rn_R10W) return std::string("R10W");
	else if (p_reg == rn_R11W) return std::string("R11W");
	else if (p_reg == rn_R12W) return std::string("R12W");
	else if (p_reg == rn_R13W) return std::string("R13W");
	else if (p_reg == rn_R14W) return std::string("R14W");
	else if (p_reg == rn_R15W) return std::string("R15W");

	else if (p_reg == rn_AH) return std::string("AH");
	else if (p_reg == rn_BH) return std::string("BH");
	else if (p_reg == rn_CH) return std::string("CH");
	else if (p_reg == rn_DH) return std::string("DH");
	else if (p_reg == rn_AL) return std::string("AL");
	else if (p_reg == rn_BL) return std::string("BL");
	else if (p_reg == rn_CL) return std::string("CL");
	else if (p_reg == rn_DL) return std::string("DL");
	else if (p_reg == rn_SIL) return std::string("SIL");
	else if (p_reg == rn_DIL) return std::string("DIL");
	else if (p_reg == rn_BPL) return std::string("BPL");
	else if (p_reg == rn_SPL) return std::string("SPL");
	else if (p_reg == rn_R8B) return std::string("R8B");
	else if (p_reg == rn_R9B) return std::string("R9B");
	else if (p_reg == rn_R10B) return std::string("R10B");
	else if (p_reg == rn_R11B) return std::string("R11B");
	else if (p_reg == rn_R12B) return std::string("R12B");
	else if (p_reg == rn_R13B) return std::string("R13B");
	else if (p_reg == rn_R14B) return std::string("R14B");
	else if (p_reg == rn_R15B) return std::string("R15B");

	else return std::string("UNEXPECTED REGISTER VALUE");
}

int Register::getBitWidth(Register::RegisterName p_reg)
{
	switch (p_reg)
	{
		case rn_RAX:
		case rn_RBX:
		case rn_RCX:
		case rn_RDX:
		case rn_RSI:
		case rn_RDI:
		case rn_RBP:
		case rn_RSP:
		case rn_R8:
		case rn_R9:
		case rn_R10:
		case rn_R11:
		case rn_R12:
		case rn_R13:
		case rn_R14:
		case rn_R15:
		case rn_RIP:
			return 64;
			break;
		case rn_EAX:
		case rn_EBX:
		case rn_ECX:
		case rn_EDX:
		case rn_EDI:
		case rn_ESI:
		case rn_EBP:
		case rn_ESP:
		case rn_R8D:
		case rn_R9D:
		case rn_R10D:
		case rn_R11D:
		case rn_R12D:
		case rn_R13D:
		case rn_R14D:
		case rn_R15D:
			return 32;
			break;

		case rn_AX:				
		case rn_BX:				
		case rn_CX:				
		case rn_DX:				
		case rn_BP:				
		case rn_SP:				
		case rn_SI:				
		case rn_DI:				
		case rn_R8W:
		case rn_R9W:
		case rn_R10W:
		case rn_R11W:
		case rn_R12W:
		case rn_R13W:
		case rn_R14W:
		case rn_R15W:
			return 16;
			break;

		case rn_AH:				
		case rn_BH:				
		case rn_CH:				
		case rn_DH:				
		case rn_AL:				
		case rn_BL:				
		case rn_CL:				
		case rn_DL:				
		case rn_SIL:
		case rn_DIL:
		case rn_BPL:
		case rn_SPL:
		case rn_R8B:
		case rn_R9B:
		case rn_R10B:
		case rn_R11B:
		case rn_R12B:
		case rn_R13B:
		case rn_R14B:
		case rn_R15B:
			return 8;
			break;

		default:
			return -1;
			break;
	}
}


// favor registers R10..R15
Register::RegisterName Register::getFreeRegister64(std::set<Register::RegisterName> p_taken)
{

#define ret_if_free4(a, b, c, d)\
	if (p_taken.count(a) == 0 && p_taken.count(b) == 0 && p_taken.count(c) == 0 && p_taken.count(d) == 0 ) \
		return a; \

#define ret_if_free5(a, b, c, d, e)\
	if (p_taken.count(a) == 0 && p_taken.count(b) == 0 && p_taken.count(c) == 0 && p_taken.count(d) == 0  && p_taken.count(e)==0) \
		return a; \

	ret_if_free4(rn_R8, rn_R8D, rn_R8W, rn_R8B);
	ret_if_free4(rn_R9, rn_R9D, rn_R9W, rn_R9B);
	ret_if_free4(rn_R10, rn_R10D, rn_R10W, rn_R10B);
	ret_if_free4(rn_R11, rn_R11D, rn_R11W, rn_R11B);
	ret_if_free4(rn_R12, rn_R12D, rn_R12W, rn_R12B);
	ret_if_free4(rn_R13, rn_R13D, rn_R13W, rn_R13B);
	ret_if_free4(rn_R14, rn_R14D, rn_R14W, rn_R14B);
	ret_if_free4(rn_R15, rn_R15D, rn_R15W, rn_R15B);
	ret_if_free5(rn_RAX, rn_EAX, rn_AX, rn_AL, rn_AH);
	ret_if_free5(rn_RCX, rn_ECX, rn_CX, rn_CL, rn_CH);
	ret_if_free5(rn_RDX, rn_EDX, rn_DX, rn_DL, rn_DH);
	ret_if_free5(rn_RBX, rn_EBX, rn_BX, rn_BL, rn_BH);
	ret_if_free5(rn_RSI, rn_ESI, rn_SI, rn_SIL, rn_SIH);
	ret_if_free5(rn_RDI, rn_EDI, rn_DI, rn_DIL, rn_DIH);
	ret_if_free5(rn_RSP, rn_ESP, rn_SP, rn_SPL, rn_SPH);
	ret_if_free5(rn_RBP, rn_EBP, rn_BP, rn_BPL, rn_BPH);
	return rn_UNKNOWN;
}
