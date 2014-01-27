#ifndef _MEDS_REGISTER_H
#define _MEDS_REGISTER_H

#include <string>

namespace MEDS_Annotation 
{

class Register {
public:
  enum RegisterName { UNKNOWN, EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, AX, BX, CX, DX, BP, SP, SI, DI, AH, BH, CH, DH, AL, BL, CL, DL, RAX, RBX, RCX, RDX, RBP, RSP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D, R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W, SIL, DIL, BPL, SPL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B };
  static RegisterName getRegister(std::string);
  static RegisterName getRegister(char *str);
  static bool is64bit(RegisterName);
  static bool is32bit(RegisterName);
  static bool is16bit(RegisterName);
  static bool is8bit(RegisterName);
  static int getBitWidth(RegisterName);
  static std::string toString(RegisterName);
};

}

#endif
