#include "DirectOffsetInference.hpp"

using namespace std;
using namespace libIRDB;

DirectOffsetInference::DirectOffsetInference(OffsetInference *offset_inference)
{
    //TODO: throw exception
    assert(offset_inference != NULL);

    this->offset_inference = offset_inference;
}

PNStackLayout* DirectOffsetInference::GetPNStackLayout(Function_t *func)
{
    return offset_inference->GetDirectAccessLayout(func);
}

std::string DirectOffsetInference::GetInferenceName() const
{
    return "Direct Offset Inference";
}
