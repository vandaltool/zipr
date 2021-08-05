#ifndef ARCHBASE_HPP
#define ARCHBASE_HPP


class ZiprArchitectureHelperBase_t
{
	private:
		const unique_ptr<ZiprPinnerBase_t > m_pinner ;
		const unique_ptr<ZiprPatcherBase_t> m_patcher;
		const unique_ptr<ZiprSizerBase_t  > m_sizer  ;

		ZiprArchitectureHelperBase_t()=delete;

	protected:
		Zipr_t*                             m_zipr   ;
		ZiprArchitectureHelperBase_t(Zipr_SDK::Zipr_t* p_zipr_obj);

	public:
		virtual IRDB_SDK::Instruction_t* createNewJumpInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing)=0;
		virtual IRDB_SDK::Instruction_t* createNewHaltInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing)=0;
		virtual RangeAddress_t           splitDollop             (IRDB_SDK::FileIR_t* firp, Zipr_SDK::Dollop_t* to_split, const RangeAddress_t p_cur_addr);

		static std::unique_ptr<ZiprArchitectureHelperBase_t> factory(Zipr_SDK::Zipr_t* zipr_obj);

		ZiprPinnerBase_t * getPinner () const { return m_pinner .get(); }
		ZiprPatcherBase_t* getPatcher() const { return m_patcher.get(); }
		ZiprSizerBase_t  * getSizer  () const { return m_sizer  .get(); }

};

#endif
