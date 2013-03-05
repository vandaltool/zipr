
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
	
	std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> annotations = annotParser->getAnnotations();

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

		MEDS_InstructionCheckAnnotation annotation = annotations[vo];
	
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

	return ranges;
}
