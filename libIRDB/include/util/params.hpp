#ifndef _PARAMS_H
#define _PARAMS_H

extern bool IsParameterWrite(const libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn, std::string& output_dst);
extern bool CallFollows(const libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn, const std::string& arg_str, const std::string & = "");
extern bool LeaFlowsIntoCall(const libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn);
extern bool LeaFlowsIntoPrintf(const libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn);
extern bool FlowsIntoCall(const libIRDB::FileIR_t *firp, libIRDB::Instruction_t* insn);

#endif
