

typedef Zipr_SDK::ZiprPluginInterface_t* DLFunctionHandle_t;
typedef std::set<DLFunctionHandle_t> DLFunctionHandleSet_t;

typedef Zipr_SDK::ZiprPluginInterface_t* (*GetPluginInterface_t)(
        Zipr_SDK::MemorySpace_t *p_ms,
        ELFIO::elfio *p_elfio,
        libIRDB::FileIR_t *p_firp,
        Zipr_SDK::Options_t *p_opts,
        Zipr_SDK::InstructionLocationMap_t *p_fil
	);


class ZiprPluginManager_t : public ZiprPluginInterface_t
{
	public:
		ZiprPluginManager_t
			(
        		 Zipr_SDK::MemorySpace_t *p_ms,
        		 ELFIO::elfio *p_elfio,
        		 libIRDB::FileIR_t *p_firp,
        		 Zipr_SDK::Options_t *p_opts,
        		 Zipr_SDK::InstructionLocationMap_t *p_fil
			)
			: m_opts(p_opts)
			{ open_plugins(p_ms,p_elfio,p_firp,p_opts,p_fil); }

        	virtual void PinningBegin();
        	virtual void PinningEnd();
	
        	virtual void DollopBegin();
        	virtual void DollopEnd();
	
        	virtual void CallbackLinkingBegin();
        	virtual void CallbackLinkingEnd();

	private:

		Options_t *m_opts;
		void open_plugins
			(
        		 Zipr_SDK::MemorySpace_t *p_ms,
        		 ELFIO::elfio *p_elfio,
        		 libIRDB::FileIR_t *p_firp,
        		 Zipr_SDK::Options_t *p_opts,
        		 Zipr_SDK::InstructionLocationMap_t *p_fil
			);

		// storage for handles that've been dlopened()
		DLFunctionHandleSet_t m_handleList;

};
