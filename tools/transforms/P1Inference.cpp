
#include "P1Inference.hpp"
#include <cstdlib>

using namespace std;
using namespace libIRDB;

P1Inference::P1Inference(OffsetInference *offset_inference)
{
    this->offset_inference = offset_inference;
}


PNStackLayout* P1Inference::GetPNStackLayout(Function_t *func)
{
    return offset_inference->GetP1AccessLayout(func);
}


std::string P1Inference::GetInferenceName() const
{
    return "P1 Inference";
}
