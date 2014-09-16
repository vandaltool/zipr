
#ifndef __OFFSETSTACKLAYOUTINFERENCE
#define __OFFSETSTACKLAYOUTINFERENCE
#include "PNStackLayoutInference.hpp"
#include "PNRegularExpressions.hpp"
#include <map>
#include <string>

class OffsetInference : public PNStackLayoutInference
{

protected:
	std::map<libIRDB::Function_t*,PNStackLayout*> direct;
	std::map<libIRDB::Function_t*,PNStackLayout*> scaled;
	std::map<libIRDB::Function_t*,PNStackLayout*> all_offsets;
	std::map<libIRDB::Function_t*,PNStackLayout*> p1;

	PNRegularExpressions *pn_regex;

	virtual void FindAllOffsets(libIRDB::Function_t *func);
	virtual PNStackLayout* GetLayout(std::map<libIRDB::Function_t*,PNStackLayout*> &mymap, libIRDB::Function_t *func);
	//	virtual void GetInstructions(std::vector<libIRDB::Instruction_t*> &instructions,libIRDB::BasicBlock_t *block,std::set<libIRDB::BasicBlock_t*> &block_set);
	virtual StackLayout* SetupLayout(libIRDB::Function_t *func);
public:
	OffsetInference() : pn_regex(NULL) {}
	virtual ~OffsetInference();
	virtual PNStackLayout* GetPNStackLayout(libIRDB::Function_t *func);
	virtual PNStackLayout* GetDirectAccessLayout(libIRDB::Function_t *func);
	virtual PNStackLayout* GetScaledAccessLayout(libIRDB::Function_t *func);
	virtual PNStackLayout* GetP1AccessLayout(libIRDB::Function_t *func);
	virtual std::string GetInferenceName() const;
};

#endif
