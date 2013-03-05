#ifndef __PRECEDENCEBOUNDLAYOUTINFERENCE
#define __PRECEDENCEBOUNDLAYOUTINFERENCE
#include "PNStackLayoutInference.hpp"
#include "PNRegularExpressions.hpp"
#include "PrecedenceBoundaryGenerator.hpp"
#include <map>
#include <string>

class PrecedenceBoundaryInference : public PNStackLayoutInference
{
protected:
	PNStackLayoutInference *base_inference;
	PrecedenceBoundaryGenerator *pbgen;
public:
	//deconstructor?
	PrecedenceBoundaryInference(PNStackLayoutInference *inf, PrecedenceBoundaryGenerator *pbgen) : base_inference(inf) ,pbgen(pbgen) {/*TODO: exception if passed null?*/}
	virtual PNStackLayout* GetPNStackLayout(libIRDB::Function_t *func);
	virtual std::string GetInferenceName() const;
};
#endif
