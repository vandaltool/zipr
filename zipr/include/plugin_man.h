#ifndef plugin_man_h
#define plugin_man_h

typedef Zipr_SDK::ZiprPluginInterface_t* DLFunctionHandle_t;
typedef std::vector<DLFunctionHandle_t> DLFunctionHandleSet_t;

typedef Zipr_SDK::ZiprPluginInterface_t* (*GetPluginInterface_t)(
	Zipr_SDK::Zipr_t*
	);


class ZiprPluginManager_t : public ZiprPluginInterface_t
{
	public:
		ZiprPluginManager_t() :
			m_verbose("verbose")
		{}

		ZiprPluginManager_t
			(
				Zipr_SDK::Zipr_t* zipr_obj,
				zipr::ZiprOptions_t *p_opts
			)
			:
				m_verbose("verbose"),
				m_opts(p_opts)
			{
				auto opts_global_ns = dynamic_cast<ZiprOptionsNamespace_t*>(m_opts->getNamespace("global"));
				if (opts_global_ns)
					opts_global_ns->addOption(&m_verbose);
				open_plugins(zipr_obj, p_opts);
			}

        	virtual void PinningBegin();
        	virtual void PinningEnd();
	
        	virtual void DollopBegin();
        	virtual void DollopEnd();
	
        	virtual void CallbackLinkingBegin();
        	virtual void CallbackLinkingEnd();

		virtual RangeAddress_t PlaceScoopsBegin(const RangeAddress_t max_addr);
		virtual RangeAddress_t PlaceScoopsEnd(const RangeAddress_t max_addr);

		virtual bool DoPluginsPlop(IRDB_SDK::Instruction_t*,std::list<DLFunctionHandle_t>&);

		virtual bool DoesPluginAddress(const Zipr_SDK::Dollop_t *, const RangeAddress_t &, Range_t &, bool &coalesce, bool &fallthrough_allowed, DLFunctionHandle_t &);
		virtual bool DoesPluginRetargetCallback(const RangeAddress_t &, const DollopEntry_t *, RangeAddress_t &, DLFunctionHandle_t &) ;
		virtual bool DoesPluginRetargetPin(const RangeAddress_t &, const Dollop_t *, RangeAddress_t &, DLFunctionHandle_t &) ;


	private:

		void open_plugins
			(
				Zipr_SDK::Zipr_t* zipr_obj,
				Zipr_SDK::ZiprOptionsManager_t *p_opts
			);
		ZiprBooleanOption_t m_verbose;
		zipr::ZiprOptions_t *m_opts;
		// storage for handles that've been dlopened()
		DLFunctionHandleSet_t m_handleList;

};
#endif
