#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <sys/wait.h>
#include <map>
#include <fstream>

class P1Transform
{
 public:
  P1Transform();
  
  void rewrite(char *);
  bool rewrite(libIRDB::VariantIR_t*, libIRDB::Function_t*, std::map< libIRDB::Instruction_t*, std::string>&);
  
 private:
  int getStackFramePadding(libIRDB::Function_t*);
  
  // regex patterns for detecting and transforming stack instruction references
  regex_t m_stack_ebp_pattern;
  regex_t m_stack_alloc_pattern;
  regex_t m_stack_dealloc_pattern;
  regex_t m_lea_hack_pattern;
  regex_t m_fancy_ebp_pattern;
  regex_t m_stack_esp_pattern;
  regex_t m_fancy_esp_pattern;
  
};
