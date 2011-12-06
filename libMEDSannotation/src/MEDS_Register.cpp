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

bool Register::is8bit(Register::RegisterName p_reg)
{
	return p_reg == AL || p_reg == BL || p_reg == CL || p_reg == DL ||
		p_reg == AH || p_reg == BH || p_reg == CH || p_reg == DH;
}

bool Register::is16bit(Register::RegisterName p_reg)
{
	return p_reg == AX || p_reg == BX || p_reg == CX || p_reg == DX;
}

bool Register::is32bit(Register::RegisterName p_reg)
{
	return p_reg == EAX || p_reg == EBX || p_reg == ECX || p_reg == EDX || 
		p_reg == ESI || p_reg == EDI || p_reg == ESP || p_reg == EBP;
}

std::string Register::toString(Register::RegisterName p_reg)
{
	if (p_reg == UNKNOWN) return std::string("UNKNOWN");
	else if (p_reg == EAX) return std::string("EAX");
	else if (p_reg == EBX) return std::string("EBX");
	else if (p_reg == ECX) return std::string("ECX");
	else if (p_reg == EDX) return std::string("EDX");
	else if (p_reg == EDI) return std::string("EDI");
	else if (p_reg == ESI) return std::string("ESI");
	else if (p_reg == EBP) return std::string("EBP");
	else if (p_reg == ESP) return std::string("ESP");
	else if (p_reg == AX) return std::string("AX");
	else if (p_reg == BX) return std::string("BX");
	else if (p_reg == CX) return std::string("CX");
	else if (p_reg == DX) return std::string("DX");
	else if (p_reg == AH) return std::string("AH");
	else if (p_reg == BH) return std::string("BH");
	else if (p_reg == CH) return std::string("CH");
	else if (p_reg == DH) return std::string("DH");
	else if (p_reg == AL) return std::string("AL");
	else if (p_reg == BL) return std::string("BL");
	else if (p_reg == CL) return std::string("CL");
	else if (p_reg == DL) return std::string("DL");
	else return std::string("UNEXPECTED REGISTER VALUE");
}

int Register::getBitWidth(Register::RegisterName p_reg)
{
	switch (p_reg)
	{
		case EAX:
		case EBX:
		case ECX:
		case EDX:
		case EDI:
		case ESI:
		case EBP:
		case ESP:
			return 32;
			break;

		case AX:				
		case BX:				
		case CX:				
		case DX:				
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
			return 8;
			break;

		default:
			return -1;
			break;
	}
}
