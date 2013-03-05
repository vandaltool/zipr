#include "ScaledOffsetInference.hpp"
#include <cassert>

using namespace std;
using namespace libIRDB;

ScaledOffsetInference::ScaledOffsetInference(OffsetInference *offset_inference)
{
	//TODO: throw exception
	assert(offset_inference != NULL);

	this->offset_inference = offset_inference;
}

PNStackLayout* ScaledOffsetInference::GetPNStackLayout(Function_t *func)
{
	return offset_inference->GetScaledAccessLayout(func);
}

std::string ScaledOffsetInference::GetInferenceName() const
{
	return "Scaled Offset Inference";
}
