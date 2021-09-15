#ifndef ZIPR_PINNER
#define ZIPR_PINNER

class ZiprPinnerBase_t 
{
	protected:
		ZiprPinnerBase_t() { }
	public:
		virtual void doPinning()=0;
		static unique_ptr<ZiprPinnerBase_t> factory(Zipr_SDK::Zipr_t* parent);
		virtual ~ZiprPinnerBase_t() { }
		virtual void registerOptions(ZiprOptions_t* opts) { }
};


#endif
