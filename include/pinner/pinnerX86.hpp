#ifndef ZIPR_PINNER
#define ZIPR_PINNER

class ZiprPinner_t 
{
	public:
		virtual void doPinning()=0;
		static unique_ptr<ZiprPinner_t> factory(Zipr_SDK::Zipr_t* parent);
};


#endif
