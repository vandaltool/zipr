

#include <string>
#include <irdb-util> 

using namespace std;
using namespace IRDB_SDK;

RegisterID_t IRDB_SDK::strToRegister(const char *p_reg)
{
	return strToRegister(string(p_reg));
}

bool IRDB_SDK::isValidRegister(const RegisterID_t p_reg)
{
	return p_reg != rn_UNKNOWN;
}

RegisterID_t IRDB_SDK::strToRegister(const string& p_reg)
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

bool IRDB_SDK::is8bitRegister(const RegisterID_t p_reg)
{
	return p_reg == rn_AL || p_reg == rn_BL || p_reg == rn_CL || p_reg == rn_DL ||
		p_reg == rn_AH || p_reg == rn_BH || p_reg == rn_CH || p_reg == rn_DH ||
		p_reg == rn_SIL || p_reg == rn_DIL || p_reg == rn_BPL || p_reg == rn_SPL ||
		p_reg == rn_R8B || p_reg == rn_R9B || p_reg == rn_R10B || p_reg == rn_R11B ||
		p_reg == rn_R12B || p_reg == rn_R13B || p_reg == rn_R14B || p_reg == rn_R15B;
}

bool IRDB_SDK::is16bitRegister(const RegisterID_t p_reg)
{
	return p_reg == rn_AX || p_reg == rn_BX || p_reg == rn_CX || p_reg == rn_DX ||
		p_reg == rn_BP || p_reg == rn_SP || p_reg == rn_SI || p_reg == rn_DI ||
		p_reg == rn_R8W || p_reg == rn_R9W || p_reg == rn_R10W || p_reg == rn_R11W ||
		p_reg == rn_R12W || p_reg == rn_R13W || p_reg == rn_R14W || p_reg == rn_R15W;
}

bool IRDB_SDK::is32bitRegister(const RegisterID_t p_reg)
{
	return p_reg == rn_EAX || p_reg == rn_EBX || p_reg == rn_ECX || p_reg == rn_EDX || 
		p_reg == rn_ESI || p_reg == rn_EDI || p_reg == rn_ESP || p_reg == rn_EBP ||
		p_reg == rn_R8D || p_reg == rn_R9D || p_reg == rn_R10D || p_reg == rn_R11D ||
		p_reg == rn_R12D || p_reg == rn_R13D || p_reg == rn_R14D || p_reg == rn_R15D;
}

bool IRDB_SDK::is64bitRegister(const RegisterID_t p_reg)
{
	return p_reg == rn_RAX || p_reg == rn_RBX || p_reg == rn_RCX || p_reg == rn_RDX || 
		p_reg == rn_RSI || p_reg == rn_RDI || p_reg == rn_RBP || p_reg == rn_RSP ||
		p_reg == rn_R8 || p_reg == rn_R9 || p_reg == rn_R10 || p_reg == rn_R11 || 
		p_reg == rn_R12 || p_reg == rn_R13 || p_reg == rn_R14 || p_reg == rn_R15 || p_reg == rn_RIP;
}

string IRDB_SDK::registerToString(const RegisterID_t p_reg)
{
	if (p_reg == rn_UNKNOWN) return string("UNKNOWN");

	else if (p_reg == rn_EFLAGS) return string("EFLAGS");

	else if (p_reg == rn_RAX) return string("RAX");
	else if (p_reg == rn_RBX) return string("RBX");
	else if (p_reg == rn_RCX) return string("RCX");
	else if (p_reg == rn_RDX) return string("RDX");
	else if (p_reg == rn_RSI) return string("RSI");
	else if (p_reg == rn_RDI) return string("RDI");
	else if (p_reg == rn_RBP) return string("RBP");
	else if (p_reg == rn_RSP) return string("RSP");
	else if (p_reg == rn_R8) return string("R8");
	else if (p_reg == rn_R9) return string("R9");
	else if (p_reg == rn_R10) return string("R10");
	else if (p_reg == rn_R11) return string("R11");
	else if (p_reg == rn_R12) return string("R12");
	else if (p_reg == rn_R13) return string("R13");
	else if (p_reg == rn_R14) return string("R14");
	else if (p_reg == rn_R15) return string("R15");
	else if (p_reg == rn_RIP) return string("RIP");

	else if (p_reg == rn_EAX) return string("EAX");
	else if (p_reg == rn_EBX) return string("EBX");
	else if (p_reg == rn_ECX) return string("ECX");
	else if (p_reg == rn_EDX) return string("EDX");
	else if (p_reg == rn_EDI) return string("EDI");
	else if (p_reg == rn_ESI) return string("ESI");
	else if (p_reg == rn_EBP) return string("EBP");
	else if (p_reg == rn_ESP) return string("ESP");
	else if (p_reg == rn_R8D) return string("R8D");
	else if (p_reg == rn_R9D) return string("R9D");
	else if (p_reg == rn_R10D) return string("R10D");
	else if (p_reg == rn_R11D) return string("R11D");
	else if (p_reg == rn_R12D) return string("R12D");
	else if (p_reg == rn_R13D) return string("R13D");
	else if (p_reg == rn_R14D) return string("R14D");
	else if (p_reg == rn_R15D) return string("R15D");

	else if (p_reg == rn_AX) return string("AX");
	else if (p_reg == rn_BX) return string("BX");
	else if (p_reg == rn_CX) return string("CX");
	else if (p_reg == rn_DX) return string("DX");
	else if (p_reg == rn_BP) return string("BP");
	else if (p_reg == rn_SP) return string("SP");
	else if (p_reg == rn_SI) return string("SI");
	else if (p_reg == rn_DI) return string("DI");
	else if (p_reg == rn_R8W) return string("R8W");
	else if (p_reg == rn_R9W) return string("R9W");
	else if (p_reg == rn_R10W) return string("R10W");
	else if (p_reg == rn_R11W) return string("R11W");
	else if (p_reg == rn_R12W) return string("R12W");
	else if (p_reg == rn_R13W) return string("R13W");
	else if (p_reg == rn_R14W) return string("R14W");
	else if (p_reg == rn_R15W) return string("R15W");

	else if (p_reg == rn_AH) return string("AH");
	else if (p_reg == rn_BH) return string("BH");
	else if (p_reg == rn_CH) return string("CH");
	else if (p_reg == rn_DH) return string("DH");
	else if (p_reg == rn_AL) return string("AL");
	else if (p_reg == rn_BL) return string("BL");
	else if (p_reg == rn_CL) return string("CL");
	else if (p_reg == rn_DL) return string("DL");
	else if (p_reg == rn_SIL) return string("SIL");
	else if (p_reg == rn_DIL) return string("DIL");
	else if (p_reg == rn_BPL) return string("BPL");
	else if (p_reg == rn_SPL) return string("SPL");
	else if (p_reg == rn_R8B) return string("R8B");
	else if (p_reg == rn_R9B) return string("R9B");
	else if (p_reg == rn_R10B) return string("R10B");
	else if (p_reg == rn_R11B) return string("R11B");
	else if (p_reg == rn_R12B) return string("R12B");
	else if (p_reg == rn_R13B) return string("R13B");
	else if (p_reg == rn_R14B) return string("R14B");
	else if (p_reg == rn_R15B) return string("R15B");

	else return string("UNEXPECTED REGISTER VALUE");
}


RegisterID_t IRDB_SDK::convertRegisterTo64bit(const RegisterID_t p_reg)
{
	if (is64bitRegister(p_reg))
		return p_reg;

	switch (p_reg)
	{
		case rn_AL:				
		case rn_AH:				
		case rn_AX:				
		case rn_EAX:
			return rn_RAX;
		case rn_BL:				
		case rn_BH:				
		case rn_BX:				
		case rn_EBX:
			return rn_RBX;
		case rn_CL:				
		case rn_CH:				
		case rn_CX:				
		case rn_ECX:
			return rn_RCX;
		case rn_DL:				
		case rn_DH:				
		case rn_DX:				
		case rn_EDX:
			return rn_RDX;
		case rn_DIL:
		case rn_DI:				
		case rn_EDI:
			return rn_RDI;
		case rn_SIL:
		case rn_SI:				
		case rn_ESI:
			return rn_RSI;
		case rn_BPL:
		case rn_BP:				
		case rn_EBP:
			return rn_RBP;
		case rn_SPL:
		case rn_SP:				
		case rn_ESP:
			return rn_RSP;
		case rn_R8B:
		case rn_R8W:
		case rn_R8D:
			return rn_R8;
		case rn_R9B:
		case rn_R9W:
		case rn_R9D:
			return rn_R9;
		case rn_R10B:
		case rn_R10W:
		case rn_R10D:
			return rn_R10;
		case rn_R11B:
		case rn_R11W:
		case rn_R11D:
			return rn_R11;
		case rn_R12B:
		case rn_R12W:
		case rn_R12D:
			return rn_R12;
		case rn_R13B:
		case rn_R13W:
		case rn_R13D:
			return rn_R13;
		case rn_R14B:
		case rn_R14W:
		case rn_R14D:
			return rn_R14;
		case rn_R15B:
		case rn_R15W:
		case rn_R15D:
			return rn_R15;

		default:
			return rn_UNKNOWN;
			break;
	}
}

RegisterID_t IRDB_SDK::convertRegisterTo32bit(const RegisterID_t p_reg)
{
	if (is32bitRegister(p_reg))
		return p_reg;

	switch (p_reg)
	{
		case rn_RAX:   return  rn_EAX;
		case rn_RBX:   return  rn_EBX;
		case rn_RCX:   return  rn_ECX;
		case rn_RDX:   return  rn_EDX;
		case rn_RBP:   return  rn_EBP;
		case rn_RSP:   return  rn_ESP;
		case rn_RSI:   return  rn_ESI;
		case rn_RDI:   return  rn_EDI;
		case  rn_R8:   return  rn_R8D;
		case  rn_R9:   return  rn_R9D;
		case rn_R10:   return rn_R10D;
		case rn_R11:   return rn_R11D;
		case rn_R12:   return rn_R12D;
		case rn_R13:   return rn_R13D;
		case rn_R14:   return rn_R14D;
		case rn_R15:   return rn_R15D;
		default:
			return rn_UNKNOWN;
			break;
	}
}

RegisterID_t IRDB_SDK::convertRegisterTo16bit(const RegisterID_t p_reg)
{
	if (is16bitRegister(p_reg))
		return p_reg;

	switch (p_reg)
	{
		case rn_RAX: case  rn_EAX:   return rn_AX;
		case rn_RBX: case  rn_EBX:   return rn_BX;
		case rn_RCX: case  rn_ECX:   return rn_CX;
		case rn_RDX: case  rn_EDX:   return rn_DX;
		case rn_RBP: case  rn_EBP:   return rn_BP;
		case rn_RSP: case  rn_ESP:   return rn_SP;
		case rn_RSI: case  rn_ESI:   return rn_SI;
		case rn_RDI: case  rn_EDI:   return rn_DI;
		case  rn_R8: case  rn_R8D:   return rn_R8W;
		case  rn_R9: case  rn_R9D:   return rn_R9W;
		case rn_R10: case rn_R10D:   return rn_R10W;
		case rn_R11: case rn_R11D:   return rn_R11W;
		case rn_R12: case rn_R12D:   return rn_R12W;
		case rn_R13: case rn_R13D:   return rn_R13W;
		case rn_R14: case rn_R14D:   return rn_R14W;
		case rn_R15: case rn_R15D:   return rn_R15W;
		default:
			return rn_UNKNOWN;
			break;
	}
}

