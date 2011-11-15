
#ifndef __DIRECTOFFSETINFERENCE
#define __DIRECTOFFSETINFERENCE

#include "OffsetInference.hpp"
#include "PNStackLayoutInference.hpp"

class DirectOffsetInference : public PNStackLayoutInference
{
protected:
    OffsetInference *offset_inference;
public:
    DirectOffsetInference(OffsetInference *offset_inference);
    virtual PNStackLayout* GetPNStackLayout(libIRDB::Function_t *func);
    virtual std::string GetInferenceName() const;

    
};


#endif
