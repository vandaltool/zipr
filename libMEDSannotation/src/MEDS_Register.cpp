#include <string>
#include <strings.h>

#include "MEDS_Register.hpp"

using namespace MEDS_Annotation;

Register::RegisterName Register::getRegister(std::string p_reg)
{
	if (strcasecmp(p_reg.c_str(), "EAX") == 0)
		return EAX;
	else if (strcasecmp(p_reg.c_str(), "EBX") == 0)
		return EBX;
	else if (strcasecmp(p_reg.c_str(), "ECX") == 0)
		return ECX;
	else if (strcasecmp(p_reg.c_str(), "EDX") == 0)
		return EDX;
	else if (strcasecmp(p_reg.c_str(), "EDI") == 0)
		return EDI;
	else if (strcasecmp(p_reg.c_str(), "ESI") == 0)
		return ESI;
	else if (strcasecmp(p_reg.c_str(), "EBP") == 0)
		return EBP;
	else if (strcasecmp(p_reg.c_str(), "ESP") == 0)
		return ESP;
	else if (strcasecmp(p_reg.c_str(), "AX") == 0)
		return AX;
	else if (strcasecmp(p_reg.c_str(), "BX") == 0)
		return BX;
	else if (strcasecmp(p_reg.c_str(), "CX") == 0)
		return CX;
	else if (strcasecmp(p_reg.c_str(), "DX") == 0)
		return DX;
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
	else
		return UNKNOWN;
}

bool Register::is8bit(Register::RegisterName reg)
{
	return reg == AL || reg == BL || reg == CL || reg == DL ||
		reg == AH || reg == BH || reg == CH || reg == DH;
}

bool Register::is16bit(Register::RegisterName reg)
{
	return reg == AX || reg == BX || reg == CX || reg == DX;
}

bool Register::is32bit(Register::RegisterName reg)
{
	return reg == EAX || reg == EBX || reg == ECX || reg == EDX || 
		reg == ESI || reg == EDI || reg == ESP || reg == EBP;
}
