
#ifndef zipr_utils_h
#define zipr_utils_h

namespace Utils {
	extern size_t CALLBACK_TRAMPOLINE_SIZE;
	int DetermineWorstCaseInsnSize(libIRDB::Instruction_t*);
}
#endif
