/*
 * Copyright (c) 2016 - Zephyr Software
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */



#include <libIRDB-core.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cctype>
#include <assert.h>
#include <libgen.h>

#include "targ-config.h"

#include "MEDS_AnnotationParser.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"

using namespace libIRDB;
using namespace std;
using namespace MEDS_Annotation;


#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"

static void add_annotations(FileIR_t* firp)
{
	int num_safe_functions = 0;
	char *fileBasename = basename((char*)firp->GetFile()->GetURL().c_str());

	cout<<"Adding FR annotations to "<<firp->GetFile()->GetURL()<<endl;

	MEDS_AnnotationParser annotationParser;
	string annotationFilename;
	if (strcmp(fileBasename, BINARY_NAME) == 0)
		annotationFilename = string(BINARY_NAME);
	else   
		annotationFilename = string(SHARED_OBJECTS_DIR) + "/" + fileBasename ;
	
	cerr << "annotation file: " << annotationFilename << endl;
	annotationParser.parseFile(annotationFilename+".annot.full");

	// now, look through each instruction and match the insn to the annotation.
	cout<< "Annot size is "<<std::dec<< annotationParser.getAnnotations().size() << endl;

	for(set<Instruction_t*>::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		assert(insn);


		/* find annotations for this insn */
		std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;
		VirtualOffset vo(insn->GetAddress()->GetVirtualOffset());

		cout<<"Checking annotations for "<<std::hex<<vo.to_string()<<endl;

       	/* find it in the annotations */
       	ret = annotationParser.getAnnotations().equal_range(vo);
       	MEDS_SafeFuncAnnotation* p_annotation;
	
       	/* for each annotation for this instruction */
       	for (MEDS_Annotations_t::iterator it2 = ret.first; it2 != ret.second; ++it2)
       	{
			p_annotation=dynamic_cast<MEDS_SafeFuncAnnotation*>(it2->second);
			if(p_annotation==NULL)
				continue;

			cout<<"Found Func SAFE annotation for "<<std::hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
			if (p_annotation->isSafe())
			{
        		Relocation_t* reloc=new Relocation_t;
				reloc->SetOffset(0);
				reloc->SetType("stars::safefn");
        		insn->GetRelocations().insert(reloc);
        		firp->GetRelocations().insert(reloc);
				num_safe_functions++;
			}
		}
	}

	cout << "# ATTRIBUTE num_safe_functions=" << dec << num_safe_functions << endl;
}


main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: fill_in_safefn <id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));
		assert(pidp->IsRegistered()==true);


                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			// read the db  
			firp=new FileIR_t(*pidp,this_file);
		
			add_annotations(firp);	
		
			firp->WriteToDB();

			delete firp;
		}


		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cout<<"Done!"<<endl;

	delete pidp;
}

