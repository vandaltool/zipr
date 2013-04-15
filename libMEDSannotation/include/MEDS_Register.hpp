#ifndef _MEDS_REGISTER_H
#define _MEDS_REGISTER_H

#include <string>

namespace MEDS_Annotation 
{

class Register {
public:
  enum RegisterName { UNKNOWN, EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, AX, BX, CX, DX, BP, SP, SI, DI, AH, BH, CH, DH, AL, BL, CL, DL };
  static RegisterName getRegister(std::string);
  static RegisterName getRegister(char *str);
  static bool is32bit(RegisterName);
  static bool is16bit(RegisterName);
  static bool is8bit(RegisterName);
  static int getBitWidth(RegisterName);
  static std::string toString(RegisterName);
};

}

#endif
