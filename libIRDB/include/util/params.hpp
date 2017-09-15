#ifndef _PARAMS_H
#define _PARAMS_H

extern bool IsParameterWrite(const libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn, std::string& output_dst);
extern bool CallFollows(libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn, const std::string& arg_str);

#endif
