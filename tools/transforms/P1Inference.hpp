
//TODO: for now we can't trust the DB for the frame size,
//in the future the constructor should be passed the size as found
//by the database

#ifndef __P1INFERENCE
#define __P1INFERENCE

#include "PNStackLayoutInference.hpp"
#include "OffsetInference.hpp"

class P1Inference : public PNStackLayoutInference
{
protected:
	OffsetInference *offset_inference;
public:
	P1Inference(OffsetInference *offset_inference);
	virtual PNStackLayout* GetPNStackLayout(libIRDB::Function_t *func);
	virtual std::string GetInferenceName() const;
};

#endif
