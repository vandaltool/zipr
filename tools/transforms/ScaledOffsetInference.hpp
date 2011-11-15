
#ifndef __SCALEDOFFSETINFERENCE
#define __SCALEDOFFSETINFERENCE

#include "OffsetInference.hpp"
#include "PNStackLayoutInference.hpp"

class ScaledOffsetInference : public PNStackLayoutInference
{
protected:
    OffsetInference *offset_inference;
public:
    ScaledOffsetInference(OffsetInference *offset_inference);
    virtual PNStackLayout* GetPNStackLayout(libIRDB::Function_t *func);
    virtual std::string GetInferenceName() const;
    
};
#endif
