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

Register::RegisterName Register::getRegister(std::string p_reg)
{
	if (strcasecmp(p_reg.c_str(), "EFLAGS") ==0)
		return EFLAGS;
	else if (strcasecmp(p_reg.c_str(), "RAX") == 0)
		return RAX;
	else if (strcasecmp(p_reg.c_str(), "RBX") == 0)
		return RBX;
	else if (strcasecmp(p_reg.c_str(), "RCX") == 0)
		return RCX;
	else if (strcasecmp(p_reg.c_str(), "RDX") == 0)
		return RDX;
	else if (strcasecmp(p_reg.c_str(), "RSI") == 0)
		return RSI;
	else if (strcasecmp(p_reg.c_str(), "RDI") == 0)
		return RDI;
	else if (strcasecmp(p_reg.c_str(), "RBP") == 0)
		return RBP;
	else if (strcasecmp(p_reg.c_str(), "RSP") == 0)
		return RSP;
	else if (strcasecmp(p_reg.c_str(), "R8") == 0)
		return R8;
	else if (strcasecmp(p_reg.c_str(), "R9") == 0)
		return R9;
	else if (strcasecmp(p_reg.c_str(), "R10") == 0)
		return R10;
	else if (strcasecmp(p_reg.c_str(), "R11") == 0)
		return R11;
	else if (strcasecmp(p_reg.c_str(), "R12") == 0)
		return R12;
	else if (strcasecmp(p_reg.c_str(), "R13") == 0)
		return R13;
	else if (strcasecmp(p_reg.c_str(), "R14") == 0)
		return R14;
	else if (strcasecmp(p_reg.c_str(), "R15") == 0)
		return R15;

	else if (strcasecmp(p_reg.c_str(), "EAX") == 0)
		return EAX;
	else if (strcasecmp(p_reg.c_str(), "EBX") == 0)
		return EBX;
	else if (strcasecmp(p_reg.c_str(), "ECX") == 0)
		return ECX;
	else if (strcasecmp(p_reg.c_str(), "EDX") == 0)
		return EDX;
	else if (strcasecmp(p_reg.c_str(), "ESI") == 0)
		return ESI;
	else if (strcasecmp(p_reg.c_str(), "EDI") == 0)
		return EDI;
	else if (strcasecmp(p_reg.c_str(), "EBP") == 0)
		return EBP;
	else if (strcasecmp(p_reg.c_str(), "ESP") == 0)
		return ESP;
	else if (strcasecmp(p_reg.c_str(), "R8D") == 0)
		return R8D;
	else if (strcasecmp(p_reg.c_str(), "R9D") == 0)
		return R9D;
	else if (strcasecmp(p_reg.c_str(), "R10D") == 0)
		return R10D;
	else if (strcasecmp(p_reg.c_str(), "R11D") == 0)
		return R11D;
	else if (strcasecmp(p_reg.c_str(), "R12D") == 0)
		return R12D;
	else if (strcasecmp(p_reg.c_str(), "R13D") == 0)
		return R13D;
	else if (strcasecmp(p_reg.c_str(), "R14D") == 0)
		return R14D;
	else if (strcasecmp(p_reg.c_str(), "R15D") == 0)
		return R15D;

	else if (strcasecmp(p_reg.c_str(), "AX") == 0)
		return AX;
	else if (strcasecmp(p_reg.c_str(), "BX") == 0)
		return BX;
	else if (strcasecmp(p_reg.c_str(), "CX") == 0)
		return CX;
	else if (strcasecmp(p_reg.c_str(), "DX") == 0)
		return DX;
	else if (strcasecmp(p_reg.c_str(), "BP") == 0)
		return BP;
	else if (strcasecmp(p_reg.c_str(), "SP") == 0)
		return SP;
	else if (strcasecmp(p_reg.c_str(), "SI") == 0)
		return SI;
	else if (strcasecmp(p_reg.c_str(), "DI") == 0)
		return DI;
	else if (strcasecmp(p_reg.c_str(), "R8W") == 0)
		return R8W;
	else if (strcasecmp(p_reg.c_str(), "R9W") == 0)
		return R9W;
	else if (strcasecmp(p_reg.c_str(), "R10W") == 0)
		return R10W;
	else if (strcasecmp(p_reg.c_str(), "R11W") == 0)
		return R11W;
	else if (strcasecmp(p_reg.c_str(), "R12W") == 0)
		return R12W;
	else if (strcasecmp(p_reg.c_str(), "R13W") == 0)
		return R13W;
	else if (strcasecmp(p_reg.c_str(), "R14W") == 0)
		return R14W;
	else if (strcasecmp(p_reg.c_str(), "R15W") == 0)
		return R15W;

	else if (strcasecmp(p_reg.c_str(), "AH") == 0)
		return AL;
	else if (strcasecmp(p_reg.c_str(), "BH") == 0)
		return BL;
	else if (strcasecmp(p_reg.c_str(), "CH") == 0)
		return CL;
	else if (strcasecmp(p_reg.c_str(), "DH") == 0)
		return DL;
	else if (strcasecmp(p_reg.c_str(), "AL") == 0)
		return AL;
	else if (strcasecmp(p_reg.c_str(), "BL") == 0)
		return BL;
	else if (strcasecmp(p_reg.c_str(), "CL") == 0)
		return CL;
	else if (strcasecmp(p_reg.c_str(), "DL") == 0)
		return DL;
	else if (strcasecmp(p_reg.c_str(), "SIL") == 0)
		return SIL;
	else if (strcasecmp(p_reg.c_str(), "DIL") == 0)
		return DIL;
	else if (strcasecmp(p_reg.c_str(), "BPL") == 0)
		return BPL;
	else if (strcasecmp(p_reg.c_str(), "SPL") == 0)
		return SPL;
	else if (strcasecmp(p_reg.c_str(), "R8B") == 0)
		return R8B;
	else if (strcasecmp(p_reg.c_str(), "R9B") == 0)
		return R9B;
	else if (strcasecmp(p_reg.c_str(), "R10B") == 0)
		return R10B;
	else if (strcasecmp(p_reg.c_str(), "R11B") == 0)
		return R11B;
	else if (strcasecmp(p_reg.c_str(), "R12B") == 0)
		return R12B;
	else if (strcasecmp(p_reg.c_str(), "R13B") == 0)
		return R13B;
	else if (strcasecmp(p_reg.c_str(), "R14B") == 0)
		return R14B;
	else if (strcasecmp(p_reg.c_str(), "R15B") == 0)
		return R15B;
	else
		return UNKNOWN;
}

bool Register::is8bit(Register::RegisterName p_reg)
{
	return p_reg == AL || p_reg == BL || p_reg == CL || p_reg == DL ||
		p_reg == AH || p_reg == BH || p_reg == CH || p_reg == DH ||
		p_reg == SIL || p_reg == DIL || p_reg == BPL || p_reg == SPL ||
		p_reg == R8B || p_reg == R9B || p_reg == R10B || p_reg == R11B ||
		p_reg == R12B || p_reg == R13B || p_reg == R14B || p_reg == R15B;
}

bool Register::is16bit(Register::RegisterName p_reg)
{
	return p_reg == AX || p_reg == BX || p_reg == CX || p_reg == DX ||
		p_reg == BP || p_reg == SP || p_reg == SI || p_reg == DI ||
		p_reg == R8W || p_reg == R9W || p_reg == R10W || p_reg == R11W ||
		p_reg == R12W || p_reg == R13W || p_reg == R14W || p_reg == R15W;
}

bool Register::is32bit(Register::RegisterName p_reg)
{
	return p_reg == EAX || p_reg == EBX || p_reg == ECX || p_reg == EDX || 
		p_reg == ESI || p_reg == EDI || p_reg == ESP || p_reg == EBP ||
		p_reg == R8D || p_reg == R9D || p_reg == R10D || p_reg == R11D ||
		p_reg == R12D || p_reg == R13D || p_reg == R14D || p_reg == R15D;
}

bool Register::is64bit(Register::RegisterName p_reg)
{
	return p_reg == RAX || p_reg == RBX || p_reg == RCX || p_reg == RDX || 
		p_reg == RSI || p_reg == RDI || p_reg == RBP || p_reg == RSP ||
		p_reg == R8 || p_reg == R9 || p_reg == R10 || p_reg == R11 || 
		p_reg == R12 || p_reg == R13 || p_reg == R14 || p_reg == R15;
}

std::string Register::toString(Register::RegisterName p_reg)
{
	if (p_reg == UNKNOWN) return std::string("UNKNOWN");

	else if (p_reg == EFLAGS) return std::string("EFLAGS");

	else if (p_reg == RAX) return std::string("RAX");
	else if (p_reg == RBX) return std::string("RBX");
	else if (p_reg == RCX) return std::string("RCX");
	else if (p_reg == RDX) return std::string("RDX");
	else if (p_reg == RSI) return std::string("RSI");
	else if (p_reg == RDI) return std::string("RDI");
	else if (p_reg == RBP) return std::string("RBP");
	else if (p_reg == RSP) return std::string("RSP");
	else if (p_reg == R8) return std::string("R8");
	else if (p_reg == R9) return std::string("R9");
	else if (p_reg == R10) return std::string("R10");
	else if (p_reg == R11) return std::string("R11");
	else if (p_reg == R12) return std::string("R12");
	else if (p_reg == R13) return std::string("R13");
	else if (p_reg == R14) return std::string("R14");
	else if (p_reg == R15) return std::string("R15");

	else if (p_reg == EAX) return std::string("EAX");
	else if (p_reg == EBX) return std::string("EBX");
	else if (p_reg == ECX) return std::string("ECX");
	else if (p_reg == EDX) return std::string("EDX");
	else if (p_reg == EDI) return std::string("EDI");
	else if (p_reg == ESI) return std::string("ESI");
	else if (p_reg == EBP) return std::string("EBP");
	else if (p_reg == ESP) return std::string("ESP");
	else if (p_reg == R8D) return std::string("R8D");
	else if (p_reg == R9D) return std::string("R9D");
	else if (p_reg == R10D) return std::string("R10D");
	else if (p_reg == R11D) return std::string("R11D");
	else if (p_reg == R12D) return std::string("R12D");
	else if (p_reg == R13D) return std::string("R13D");
	else if (p_reg == R14D) return std::string("R14D");
	else if (p_reg == R15D) return std::string("R15D");

	else if (p_reg == AX) return std::string("AX");
	else if (p_reg == BX) return std::string("BX");
	else if (p_reg == CX) return std::string("CX");
	else if (p_reg == DX) return std::string("DX");
	else if (p_reg == BP) return std::string("BP");
	else if (p_reg == SP) return std::string("SP");
	else if (p_reg == SI) return std::string("SI");
	else if (p_reg == DI) return std::string("DI");
	else if (p_reg == R8W) return std::string("R8W");
	else if (p_reg == R9W) return std::string("R9W");
	else if (p_reg == R10W) return std::string("R10W");
	else if (p_reg == R11W) return std::string("R11W");
	else if (p_reg == R12W) return std::string("R12W");
	else if (p_reg == R13W) return std::string("R13W");
	else if (p_reg == R14W) return std::string("R14W");
	else if (p_reg == R15W) return std::string("R15W");

	else if (p_reg == AH) return std::string("AH");
	else if (p_reg == BH) return std::string("BH");
	else if (p_reg == CH) return std::string("CH");
	else if (p_reg == DH) return std::string("DH");
	else if (p_reg == AL) return std::string("AL");
	else if (p_reg == BL) return std::string("BL");
	else if (p_reg == CL) return std::string("CL");
	else if (p_reg == DL) return std::string("DL");
	else if (p_reg == SIL) return std::string("SIL");
	else if (p_reg == DIL) return std::string("DIL");
	else if (p_reg == BPL) return std::string("BPL");
	else if (p_reg == SPL) return std::string("SPL");
	else if (p_reg == R8B) return std::string("R8B");
	else if (p_reg == R9B) return std::string("R9B");
	else if (p_reg == R10B) return std::string("R10B");
	else if (p_reg == R11B) return std::string("R11B");
	else if (p_reg == R12B) return std::string("R12B");
	else if (p_reg == R13B) return std::string("R13B");
	else if (p_reg == R14B) return std::string("R14B");
	else if (p_reg == R15B) return std::string("R15B");

	else return std::string("UNEXPECTED REGISTER VALUE");
}

int Register::getBitWidth(Register::RegisterName p_reg)
{
	switch (p_reg)
	{
		case RAX:
		case RBX:
		case RCX:
		case RDX:
		case RSI:
		case RDI:
		case RBP:
		case RSP:
		case R8:
		case R9:
		case R10:
		case R11:
		case R12:
		case R13:
		case R14:
		case R15:
			return 64;
			break;
		case EAX:
		case EBX:
		case ECX:
		case EDX:
		case EDI:
		case ESI:
		case EBP:
		case ESP:
		case R8D:
		case R9D:
		case R10D:
		case R11D:
		case R12D:
		case R13D:
		case R14D:
		case R15D:
			return 32;
			break;

		case AX:				
		case BX:				
		case CX:				
		case DX:				
		case BP:				
		case SP:				
		case SI:				
		case DI:				
		case R8W:
		case R9W:
		case R10W:
		case R11W:
		case R12W:
		case R13W:
		case R14W:
		case R15W:
			return 16;
			break;

		case AH:				
		case BH:				
		case CH:				
		case DH:				
		case AL:				
		case BL:				
		case CL:				
		case DL:				
		case SIL:
		case DIL:
		case BPL:
		case SPL:
		case R8B:
		case R9B:
		case R10B:
		case R11B:
		case R12B:
		case R13B:
		case R14B:
		case R15B:
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
	if (p_taken.count(R10) == 0 && p_taken.count(R10D) == 0 && p_taken.count(R10W) == 0 && p_taken.count(R10B) == 0 )
		return R10;
	if (p_taken.count(R11) == 0 && p_taken.count(R11D) == 0 && p_taken.count(R11W) == 0 && p_taken.count(R11B) == 0 )
		return R11;
	if (p_taken.count(R12) == 0 && p_taken.count(R12D) == 0 && p_taken.count(R12W) == 0 && p_taken.count(R12B) == 0 )
		return R12;
	if (p_taken.count(R13) == 0 && p_taken.count(R13D) == 0 && p_taken.count(R13W) == 0 && p_taken.count(R13B) == 0 )
		return R13;
	if (p_taken.count(R14) == 0 && p_taken.count(R14D) == 0 && p_taken.count(R14W) == 0 && p_taken.count(R14B) == 0 )
		return R14;
	if (p_taken.count(R15) == 0 && p_taken.count(R15D) == 0 && p_taken.count(R15W) == 0 && p_taken.count(R15B) == 0 )
		return R15;
	if (p_taken.count(RAX) == 0 && p_taken.count(EAX) == 0  && p_taken.count(AX) == 0   && p_taken.count(AL) == 0 )
		return RAX;
	if (p_taken.count(RBX) == 0 && p_taken.count(EBX) == 0  && p_taken.count(BX) == 0   && p_taken.count(BL) == 0 )
		return RBX;
	if (p_taken.count(RCX) == 0 && p_taken.count(ECX) == 0  && p_taken.count(CX) == 0   && p_taken.count(CL) == 0 )
		return RCX;
	if (p_taken.count(RDX) == 0 && p_taken.count(EDX) == 0  && p_taken.count(DX) == 0   && p_taken.count(DL) == 0 )
		return RDX;
	if (p_taken.count(RSI) == 0 && p_taken.count(ESI) == 0  && p_taken.count(SI) == 0   && p_taken.count(SIL) == 0 )
		return RSI;
	if (p_taken.count(RDI) == 0 && p_taken.count(EDI) == 0  && p_taken.count(DI) == 0   && p_taken.count(DIL) == 0 )
		return RDI;
	if (p_taken.count(R8) == 0  && p_taken.count(R8D) == 0  && p_taken.count(R8W) == 0  && p_taken.count(R8B) == 0 )
		return R8;
	if (p_taken.count(R9) == 0  && p_taken.count(R9D) == 0  && p_taken.count(R9W) == 0  && p_taken.count(R9B) == 0 )
		return R9;
	if (p_taken.count(RSP) == 0 && p_taken.count(ESP) == 0  && p_taken.count(SP) == 0   && p_taken.count(SPL) == 0 )
		return RSP;
	if (p_taken.count(RBP) == 0 && p_taken.count(EBP) == 0  && p_taken.count(BP) == 0   && p_taken.count(BPL) == 0 )
		return RBP;
	return UNKNOWN;
}
