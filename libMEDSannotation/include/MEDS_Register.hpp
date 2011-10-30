#ifndef _MEDS_REGISTER_H
#define _MEDS_REGISTER_H

#include <string>

namespace MEDS_Annotation 
{

class Register {
public:
  enum RegisterName { UNKNOWN, EAX, EBX, ECX, EDX, AX, BX, CX, DX, AL, BL, CL, DL };
  static RegisterName getRegister(std::string);
};

}

#endif
