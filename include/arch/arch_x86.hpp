#ifndef ARCHX86_HPP
#define ARCHX86_HPP


class ZiprArchitectureHelperX86_t  : public ZiprArchitectureHelperBase_t
{
	public:
		ZiprArchitectureHelperX86_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprArchitectureHelperBase_t(p_zipr_obj)
                {
                }

		virtual IRDB_SDK::Instruction_t* createNewJumpInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			const auto bits=string("\xe9\x00\x00\x00\x00",5); /* jmp 0 */
                        return IRDBUtility::addNewDatabits(p_firp, p_existing, bits);
		}
		virtual IRDB_SDK::Instruction_t* createNewHaltInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			const auto bits=string("\xf4",1); 	/* hlt */
                        return IRDBUtility::addNewDatabits(p_firp, p_existing, bits);
		}
};
#endif
