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
		ZiprArchitectureHelperBase_t(Zipr_SDK::Zipr_t* p_zipr_obj);
	public:
		virtual libIRDB::Instruction_t* createNewJumpInstruction(libIRDB::FileIR_t *p_firp, libIRDB::Instruction_t* p_existing)=0;
		virtual libIRDB::Instruction_t* createNewHaltInstruction(libIRDB::FileIR_t *p_firp, libIRDB::Instruction_t* p_existing)=0;
		static std::unique_ptr<ZiprArchitectureHelperBase_t> factory(Zipr_SDK::Zipr_t* zipr_obj);

		ZiprPinnerBase_t * getPinner () const { return m_pinner .get(); }
		ZiprPatcherBase_t* getPatcher() const { return m_patcher.get(); }
		ZiprSizerBase_t  * getSizer  () const { return m_sizer  .get(); }

};

#endif
