#ifndef SIZERBASE_HPP
#define SIZERBASE_HPP

class ZiprImpl_t;

class ZiprSizerBase_t
{
	protected:
		ZiprSizerBase_t(Zipr_SDK::Zipr_t* p_zipr_obj, 
				const size_t p_CALLBACK_TRAMPOLINE_SIZE, 
				const size_t p_TRAMPOLINE_SIZE, 
				const size_t p_LONG_PIN_SIZE,
				const size_t p_SHORT_PIN_SIZE,
				const size_t p_ALIGNMENT,
				const size_t p_UNPIN_ALIGNMENT
				) ;
		ZiprMemorySpace_t &memory_space;
		ZiprImpl_t &m_zipr_obj;
	public:
		virtual ~ZiprSizerBase_t() {}
		// methods
		/** Calculate entire size of a dollop and it's fallthroughs.
		 *
		 * Calculate the space needed to place dollop
		 * and all of it's fallthroughs. This function
		 * makes sure not to overcalculate the trampoline
		 * space to accomodate for the fallthroughs.
		 */
		virtual size_t DetermineDollopSizeInclFallthrough(Zipr_SDK::Dollop_t *dollop) const;
		virtual size_t DetermineInsnSize(IRDB_SDK::Instruction_t*, bool account_for_jump = true) const =0;
		virtual Range_t DoPlacement(size_t pminimum_valid_req_size, const Zipr_SDK::Dollop_t* p_dollop) const;
		virtual RangeAddress_t PlopDollopEntryWithTarget(Zipr_SDK::DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const =0;

		// maybe try to make these private/protected and provide getters eventually.
		const size_t CALLBACK_TRAMPOLINE_SIZE;
		const size_t TRAMPOLINE_SIZE;
		const size_t LONG_PIN_SIZE;
		const size_t SHORT_PIN_SIZE;
		const size_t ALIGNMENT;
		const size_t UNPIN_ALIGNMENT;

	// factory
	static std::unique_ptr<ZiprSizerBase_t> factory(Zipr_SDK::Zipr_t *zipr_obj);

};

#endif
