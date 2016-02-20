
#ifndef zipr_utils_h
#define zipr_utils_h

namespace Utils {
	extern size_t CALLBACK_TRAMPOLINE_SIZE;
	extern size_t TRAMPOLINE_SIZE;
	void PrintStat(std::ostream &out, std::string description, double value);
	int DetermineWorstCaseInsnSize(libIRDB::Instruction_t*, bool account_for_jump = true);

	/** \brief Calculate entire size of a dollop 
	 *         and it's fallthroughs.
	 *
	 * Calculate the space needed to place dollop
	 * and all of it's fallthroughs. This function
	 * makes sure not to overcalculate the trampoline
	 * space to accomodate for the fallthroughs.
	 */
	size_t DetermineWorstCaseDollopSizeInclFallthrough(Dollop_t *dollop);

	/** \brief Is a dollop and all its fallthroughs unplaced?
	 *
	 * Return true if _dollop_ and each of its fallthrough 
	 * dollops are unplaced. Return false otherwise.
	 */
	bool IsDollopInclFallthroughUnplaced(Dollop_t *dollop);
}
#endif
