#ifndef SIZERBASE_HPP
#define SIZERBASE_HPP


class ZiprSizerBase_t
{
	protected:
		ZiprSizerBase_t(Zipr_SDK::Zipr_t* p_zipr_obj, 
				const size_t p_CALLBACK_TRAMPOLINE_SIZE, 
				const size_t p_TRAMPOLINE_SIZE, 
				const size_t p_LONG_PIN_SIZE, 
				const size_t p_SHORT_PIN_SIZE,
				const size_t p_ALIGNMENT
				) :
			memory_space(*dynamic_cast<zipr::ZiprMemorySpace_t*>(p_zipr_obj->GetMemorySpace())),
			CALLBACK_TRAMPOLINE_SIZE(p_CALLBACK_TRAMPOLINE_SIZE),
			TRAMPOLINE_SIZE         (p_TRAMPOLINE_SIZE         ),
			LONG_PIN_SIZE           (p_LONG_PIN_SIZE           ),
			SHORT_PIN_SIZE          (p_SHORT_PIN_SIZE          ),
			ALIGNMENT               (p_ALIGNMENT               )
		{
		}
		zipr::ZiprMemorySpace_t &memory_space;
	public:
		// methods
		/** Calculate entire size of a dollop and it's fallthroughs.
		 *
		 * Calculate the space needed to place dollop
		 * and all of it's fallthroughs. This function
		 * makes sure not to overcalculate the trampoline
		 * space to accomodate for the fallthroughs.
		 */
		virtual size_t DetermineDollopSizeInclFallthrough(Dollop_t *dollop) const;
		virtual size_t DetermineInsnSize(libIRDB::Instruction_t*, bool account_for_jump = true) const =0;
		virtual Range_t DoPlacement(size_t pminimum_valid_req_size) const;


		// maybe try to make these private/protected and provide getters eventually.
		const size_t CALLBACK_TRAMPOLINE_SIZE;
		const size_t TRAMPOLINE_SIZE;
		const size_t LONG_PIN_SIZE;
		const size_t SHORT_PIN_SIZE;
		const size_t ALIGNMENT;

	// factory
	static std::unique_ptr<ZiprSizerBase_t> factory(Zipr_SDK::Zipr_t *zipr_obj);

};

#endif
