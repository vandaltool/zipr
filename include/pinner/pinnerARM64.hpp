#ifndef ZIPR_PINNER_ARM64
#define ZIPR_PINNER_ARM64

class ZiprPinnerARM64_t  : public ZiprPinnerBase_t
{
	public:
                ZiprPinnerARM64_t(Zipr_SDK::Zipr_t* p_parent);
                virtual void doPinning() override;

        private:
                Zipr_SDK::Zipr_t* m_parent;

};


#endif
