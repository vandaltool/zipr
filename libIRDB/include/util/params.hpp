#ifndef _PARAMS_H
#define _PARAMS_H

extern bool IsParameterWrite(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn, std::string& output_dst);
extern bool CallFollows(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn, const std::string& arg_str, const std::string & = "");
extern bool LeaFlowsIntoCall(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn);
extern bool LeaFlowsIntoPrintf(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn);
extern bool FlowsIntoCall(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn);

#endif
