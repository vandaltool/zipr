#ifndef plugin_man_h
#define plugin_man_h

typedef Zipr_SDK::ZiprPluginInterface_t* DLFunctionHandle_t;
typedef std::set<DLFunctionHandle_t> DLFunctionHandleSet_t;

typedef Zipr_SDK::ZiprPluginInterface_t* (*GetPluginInterface_t)(
        Zipr_SDK::MemorySpace_t *p_ms,
        ELFIO::elfio *p_elfio,
        libIRDB::FileIR_t *p_firp,
        Zipr_SDK::InstructionLocationMap_t *p_fil
	);


class ZiprPluginManager_t : public ZiprPluginInterface_t
{
	public:
		ZiprPluginManager_t() :
			m_verbose("verbose")
		{}

		ZiprPluginManager_t
			(
        		 Zipr_SDK::MemorySpace_t *p_ms,
        		 ELFIO::elfio *p_elfio,
        		 libIRDB::FileIR_t *p_firp,
        		 Zipr_SDK::ZiprOptions_t *p_opts,
        		 Zipr_SDK::InstructionLocationMap_t *p_fil
			)
			:
			m_verbose("verbose"),
			m_opts(p_opts)
			{
				ZiprOptionsNamespace_t *opts_global_ns = m_opts->Namespace("global");
				if (opts_global_ns)
					opts_global_ns->AddOption(&m_verbose);
				open_plugins(p_ms,p_elfio,p_firp,p_opts,p_fil);
			}

        	virtual void PinningBegin();
        	virtual void PinningEnd();
	
        	virtual void DollopBegin();
        	virtual void DollopEnd();
	
        	virtual void CallbackLinkingBegin();
        	virtual void CallbackLinkingEnd();

					virtual bool DoesPluginPlop(libIRDB::Instruction_t*,DLFunctionHandle_t&);

	private:

		void open_plugins
			(
        		 Zipr_SDK::MemorySpace_t *p_ms,
        		 ELFIO::elfio *p_elfio,
        		 libIRDB::FileIR_t *p_firp,
        		 Zipr_SDK::ZiprOptions_t *p_opts,
        		 Zipr_SDK::InstructionLocationMap_t *p_fil
			);
		ZiprBooleanOption_t m_verbose;
		ZiprOptions_t *m_opts;
		// storage for handles that've been dlopened()
		DLFunctionHandleSet_t m_handleList;

};
#endif
