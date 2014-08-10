
#include "AnnotationBoundaryGenerator.hpp"
#include <map>

#include <iostream>

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;
//using namespace MEDS_Annotation;


vector<Range> AnnotationBoundaryGenerator::GetBoundaries(libIRDB::Function_t *func)
{
	vector<Range> ranges;
	
//	std::multimap<VirtualOffset, MEDS_AnnotationBase> 
	MEDS_Annotations_t annotations = annotParser->getAnnotations();

	for(
		set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
		)
	{
		Instruction_t* instr = *it;
		virtual_offset_t irdb_vo = instr->GetAddress()->GetVirtualOffset();

		if (irdb_vo == 0) continue;

		VirtualOffset vo(irdb_vo);

		//std::pair<std::multimap<VirtualOffset, MEDS_AnnotationBase>::iterator,std::multimap<VirtualOffset, MEDS_AnnotationBase>::iterator> ret;
		std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;
		ret = annotations.equal_range(vo);
		MEDS_InstructionCheckAnnotation annotation;
		MEDS_InstructionCheckAnnotation* p_annotation;
		for (MEDS_Annotations_t::iterator it = ret.first; it != ret.second; ++it)
		{
			p_annotation=dynamic_cast<MEDS_InstructionCheckAnnotation*>(&it->second);
			if(p_annotation==NULL)
				continue;
			annotation = *p_annotation;
			if (annotation.isValid() && annotation.isMemset())
			{
				//cerr<<"Memset annot found"<<endl;
	
				int objectSize = annotation.getObjectSize();
				int offset = annotation.getStackOffset();

				Range cur;
				cur.SetOffset(offset);
				cur.SetSize(objectSize);
	
				if (annotation.isEbpOffset()) 
				{
					if(offset < 0)
					{
						ranges.push_back(cur);
					}
			
				} else if (annotation.isEspOffset()) 
				{
					if(offset >= 0)
					{
						ranges.push_back(cur);
					}
				} else 
				{
					// something went wrong
					assert(0);
				}
			}
		}
	}

	return ranges;
}
