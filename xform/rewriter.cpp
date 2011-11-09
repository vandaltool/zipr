#include <iostream>
#include <string>
#include <set>
#include <stdlib.h>

#include "beaengine/BeaEngine.h"

#include "all.h"
#include "rewriter.h"

using namespace std;

Rewriter::Rewriter(char *p_elfPath, char *p_annotationFilePath)
{
  m_elfReader = new ElfReader(p_elfPath);

  // parse file and build up all the data structures
  readAnnotationFile(p_annotationFilePath);
  readElfFile(p_elfPath);
}

Rewriter::~Rewriter()
{
}

/*
* Read MEDS annotation file and populate relevant hash table & data structures
*/
void Rewriter::readAnnotationFile(char p_filename[])
{
	FILE* fin;
	app_iaddr_t addr = 0, prevPC = 0, prevStackDeallocPC = 0;
	union { int size, type;} size_type_u;
	char type[200];
	char scope[200];
	char remainder[200000];
	char * objname;
	int pid=0;
	int var_length=0;
	int bitvector_size_bits=0;
	int line=0;

	wahoo::Function *nullfn = new wahoo::Function("ThisIsNotAFunction", 0, 0);
	m_functions[0]=nullfn;

	constants_hash=	Hashtable_create(constants_compute_hash, constants_key_compare);
	stackrefs_hash=	Hashtable_create(stackrefs_compute_hash, stackrefs_key_compare);
	framerestores_hash=	Hashtable_create(framerestores_compute_hash, framerestores_key_compare);
	instrmaps_hash=	Hashtable_create(instrmaps_compute_hash, instrmaps_key_compare);
	funclists_hash=	Hashtable_create(funclists_compute_hash, funclists_key_compare);

	if(p_filename[0]==0)
		return;

	fin=fopen(p_filename, "r");

	if(!fin)
		fprintf(stderr,"Cannot open strata annotation file %s\n", p_filename);

	do 
	{
		fscanf(fin, "%x %d\n", &addr, &size_type_u);

		if(feof(fin))		// deal with blank lines at the EOF
			break;
		
		fscanf(fin, "%s%s", type,scope);

		int annot_type;
		int is_spec_annot=FALSE;
		if(size_type_u.type<-255)
		{
			annot_type=size_type_u.type+256;
			size_type_u.type=size_type_u.type+256;
			is_spec_annot=TRUE;
		}
		else
			annot_type=size_type_u.type;

//		fprintf(stderr,"main loop: addr 0x%x   scope: %s\n", addr, scope);

		/* if the size > 0, then this is a declaration of a variable */
		if(strcmp(type,"FUNC")==0)
		{
			prevStackDeallocPC = 0;

			char name[20000];
			/* found function declaration */
			if(strcmp(scope,"GLOBAL")==0)
			{
//   8048250     94 FUNC GLOBAL readString_xxx FUNC_UNSAFE USEFP RET    80482ad
				/* remaining parameters are name {USEFP, NOFP}  */

				funclist_hash_key_t *flhk=(funclist_hash_key_t*)spri_allocate_type(sizeof(funclist_hash_key_t));
				funclist_hash_value_t *flhv=(funclist_hash_value_t*)spri_allocate_type(sizeof(funclist_hash_value_t));
	
				fscanf(fin,"%s", name);
				flhk->name=spri_strdup(name);
				flhv->pc=addr;
//				fprintf(stderr, "Adding name=%s pc=%x to funclist hash table\n", flhk->name, flhv->pc);
				Hashtable_put(funclists_hash, flhk, flhv);	

				wahoo::Function *fn = new wahoo::Function(name, addr, size_type_u.size);
				fgets(remainder, sizeof(remainder), fin);
				if (strstr(remainder, "FUNC_SAFE"))
					fn->setSafe();
				else
					fn->setUnsafe();

				if (strstr(remainder, "USEFP"))
					fn->setUseFramePointer(true);

				m_functions[addr] = fn;

				line++;
				continue;

			}
			else if(strcmp(scope,"FRAMERESTORE")==0)
			{
				char zz[1000];
				int reg_num, reg_offset, reg_type;
				int reg=0;
				for( ; reg<8;reg++)
				{
					fscanf(fin, "%d %d %d", &reg_num, &reg_offset, &reg_type);
					assert(reg_num==reg);
					frame_restore_hash_add_reg_restore(addr,reg_num,reg_offset,reg_type);
				}
				fscanf(fin, "%s", zz);
				assert(strcmp("ZZ", zz)==0);
			}
			else if(strcmp(scope,"MMSAFENESS")==0)
			{
				char safeness[1000];
				fscanf(fin, "%s", &safeness);
				if(strcmp(safeness, "SAFE") == 0)
 				{
//
// 20110315 Anh
// bug in MEDS re. FUNCTION SAFENESS, the annotation file sometimes mark a function as
// SAFE when in fact it isn't
//					m_functions[addr]->setSafe();
					frame_restore_hash_set_safe_bit(addr,TRUE);
				}
				else if(strcmp(safeness, "SPECSAFE") == 0)
				{
//					m_functions[addr]->setSafe();
					frame_restore_hash_set_safe_bit(addr,TRUE);
				}
				else if(strcmp(safeness, "UNSAFE") == 0)
					frame_restore_hash_set_safe_bit(addr,FALSE);
				else
					fprintf(stderr,"Do not understand safeness terminology '%s' at line %d\n", safeness, line);
			}
			else
				fprintf(stderr,"Do not understand type terminology '%s' at line %d\n", type, line);
		}
		else if(strcmp(type,"MEMORYHOLE")==0)
		{
			char esp[1000];
			char plus[1000];
			int offset;
			char name[1000];
			/* found function declaration */
			assert(strcmp(scope,"STACK")==0);
			/* remaining parameters are "esp + <const> <name>"  */
			fscanf(fin, "%s%s%d%s", esp, plus, &offset, name);

			if(strcmp(name, "ReturnAddress")==0)
			{
				frame_restore_set_return_address(addr,offset);	
			}
	
//			printf("MEMORYHOLE, pc=%x offset=%d\n", addr, offset);
			/* ignoring for now */
		}
		else if(strcmp(type,"LOCALFRAME")==0 || strcmp(type,"INARGS")==0)
		{
			stackref_hash_key_t *sshk=(stackref_hash_key_t*)spri_allocate_type(sizeof(stackref_hash_key_t));
			stackref_hash_value_t *sshv=(stackref_hash_value_t*)spri_allocate_type(sizeof(stackref_hash_value_t));
			assert(strcmp(scope,"STACK")==0);
			/* remaining parameters are "esp + 0 {optional LocalVars} {optional instruction}" */
			

			/* add to hashtable, a name would be nice someday */
			sshk->pc=addr;
			sshv->size=size_type_u.size;
//			printf("Adding pc=%x size=%d to stackref hash table\n", sshk->pc, sshv->size);

			Hashtable_put(stackrefs_hash, sshk,sshv);	

		}
		else if(strcmp(type,"INSTR")==0)
		{
//fprintf(stderr, "At 0x%x, handling %s -- scope:%s\n", addr, type, scope);

			/* optimizing annotation about not needing heavyweight metadata update */
			/* ignore for now */
			
			// stash away instruction
			prevPC = addr;

            		/*
	 		 *  sudeep -- added support for flags
			 */
            		if (strcmp(scope, "RET_SAFE") == 0)
			{
			}
            		else if (strcmp(scope, "DEADREGS") == 0)
            		{
				stackref_hash_key_t sshk;
				stackref_hash_value_t *sshv;
				sshk.pc = addr;
				m_instructions[addr]->setSize(size_type_u.size);
				if (sshv = (stackref_hash_value_t*) Hashtable_get(stackrefs_hash, &sshk))
				{
                    		        instrmap_hash_key_t key;
                                        key.pc = addr;
                    		        instrmap_hash_value_t* val;
//					printf("STACK ALLOC INSTRUCTION CONFIRMED AT pc=0x%x size=%d\n", sshk.pc, size_type_u.size);
				        if (val = (instrmap_hash_value_t*) Hashtable_get(instrmaps_hash, &key))
                                        {
//					  printf("STACK ALLOC INSTRUCTION CONFIRMED: site alloc marked\n");
                                          val->site_alloc = 1; 
                                          val->size = size_type_u.size; 
                                        }

					m_instructions[addr]->markAllocSite();
					m_instructions[addr]->setSize(size_type_u.size);
				}
				else if (size_type_u.size == 1)
				{
					// keep track of 1 byte instruction
                    			instrmap_hash_key_t* key = (instrmap_hash_key_t*)spri_allocate_type(sizeof(instrmap_hash_key_t));
	                    		instrmap_hash_value_t* val = (instrmap_hash_value_t*)spri_allocate_type(sizeof(instrmap_hash_value_t));
       		             		key->pc = addr;
                       		        val->site_alloc = 0;
                                        val->size = size_type_u.size; 
	                    		Hashtable_put(instrmaps_hash,key,val); 

					fgets(remainder, sizeof(remainder), fin);
					// this is a *potential* stack deallocation instruction only
					if ((strstr(remainder,"leave") && strstr(remainder,"EFLAGS")) ||
 						strstr(remainder,"pop") && strstr(remainder,"ebp"))
					{
						prevStackDeallocPC = addr;
					}

					// b/c we ate up <remainder> we have to do this
					line++;
					continue;
						
				}
				
            		}
            		else if (strcmp(scope, "INDIRECTCALL") == 0)
			{
				/* ignore INSTR INDIRECTCALL annotations, profiler generated, analyzer uses only */
			}
			else if (strcmp(scope, "BELONGTO") == 0)
			{
				app_iaddr_t func_addr;
				fscanf(fin, "%x", &func_addr);
                    		instrmap_hash_key_t* key = (instrmap_hash_key_t*)spri_allocate_type(sizeof(instrmap_hash_key_t));
                    		instrmap_hash_value_t* val = (instrmap_hash_value_t*)spri_allocate_type(sizeof(instrmap_hash_value_t));
                    		key->pc = addr;
                    		val->func_addr = func_addr; 
                                val->site_alloc = 0;
                                val->size = 0;
                    		Hashtable_put(instrmaps_hash,key,val); 

				// rely on the fact that INST BELONGTO is the first INST annotation in a MEDS file (warning: but it is not required to be there)
				assert(m_functions[func_addr]);
				wahoo::Instruction* instr = new wahoo::Instruction(addr, -1, m_functions[func_addr]);
				m_instructions[addr] = instr;

				// associate instruction with its enclosing function
				m_functions[func_addr]->addInstruction(instr);
//fprintf(stderr,"0x%08x belongs to function %s\n", addr, m_functions[func_addr]->getName().c_str());
			} 
			else
			{

              			assert(strcmp(scope,"LOCAL")==0);

                        	if (!m_instructions[addr])
                        	{
                                	// unknown size, unknown function
                                	wahoo::Instruction* instr = new wahoo::Instruction(addr, -1, NULL);
                                	m_instructions[addr] = instr;
                        	}


                    		switch(annot_type)
                    		{
					/* No Meta Data Updates */
					case -1:	/* no meta data updates */
							/* remaining params: <reason> comment */
					{
						fgets(remainder, sizeof(remainder), fin);
						// this is a *potential* stack deallocation instruction only
						if (strstr(remainder,"add") && strstr(remainder,"esp")
							&& strstr(remainder,"1stSrcVia2ndSrc")
							&& strstr(remainder,"IMMEDNUM"))
						{
							prevStackDeallocPC = addr;
						}
						else if (strstr(remainder,"SafeFrameAlloc") 
							&& strstr(remainder,"sub")
							&& strstr(remainder,"esp"))
						{
							m_instructions[addr]->markAllocSite();
						}
						else if (strstr(remainder,"[ebp+") 
 							&& strstr(remainder,"var_"))
						{
/*
MEDS doesn't mark this as a stack reference
   8048281      0 INSTR BELONGTO 8048262
   8048281     -1 INSTR LOCAL MetadataRedundant mov     eax, [ebp+var_4]
*/
							m_instructions[addr]->markStackRef();
							m_instructions[addr]->markVarStackRef();
						}
						else if (strstr(remainder,"[esp+") 
 							&& (strstr(remainder,"var_") || strstr(remainder, "arg_")))
						{
							m_instructions[addr]->markStackRef();
							m_instructions[addr]->markVarStackRef();
						}
			

						// b/c we ate up <remainder> we have to do this
						line++;
						continue;
						
					}
	
					/* fast updates */
					case -2:	/* fast meta data updates, results = always number */
							/* remaining params: <reason> reg, {reg, ...} ZZ comment */
					{
					}

					/* skip warning message on this instruction, promised safe */
					case -3: 	/* remaining params: NoWarn comment */
					{
						break;
					}
					case -4 : // Introducing a new SizeOrtype id
						  // for safe returns	
					{
						// Safe returns come here not
						// SDT not handling them right
						// now 
						break;
					}
					default:
						fprintf(stderr,"Unknown optimizing annotation type %d at line %d of %s",
							size_type_u.type, line, p_filename);
				}
            		}
			
		
		
		}
                else if(strcmp(type,"PTRIMMEDEBP")==0 || 
			strcmp(type,"PTRIMMEDESP")==0 || 
			strcmp(type,"PTRIMMEDESP2")==0 || 
			strcmp(type,"PTRIMMEDABSOLUTE")==0
		       )
		{
                        int the_const;
                        int real_const=0;
			char field[100];
			constant_hash_key_t *chk=(constant_hash_key_t*)spri_allocate_type(sizeof(constant_hash_key_t));
			constant_hash_value_t *chv=(constant_hash_value_t*)spri_allocate_type(sizeof(constant_hash_value_t));
			assert(strcmp(scope,"STACK")==0 || strcmp(scope,"GLOBAL")==0);

			/* remaining params are <const> <field> <real_const_if_global> <comment> */ 
			fscanf(fin, "%d %s", &the_const, field);
			if( 	strcmp(type,"PTRIMMEDESP2")==0 || 
				strcmp(type,"PTRIMMEDABSOLUTE")==0
			  )
				fscanf(fin, "%x", &real_const);
			else
				real_const=the_const;

			/* set the addr, and const */
			chk->pc=addr;
			chk->the_const=the_const;

			/* set the field */
			if(strcmp(field,"displ")==0) chk->field=chf_DISPLACEMENT;
			else if(strcmp(field,"immed")==0) chk->field=chf_IMMEDIATE;
			else if(strcmp(field,"other")==0) chk->field=chf_OTHER;
			else 			     assert(0);

			/* set the type */
			if(strcmp(type,"PTRIMMEDEBP")==0 ) chv->type=cht_EBP;
			else if(strcmp(type,"PTRIMMEDESP")==0 ) chv->type=cht_ESP;
			else if(strcmp(type,"PTRIMMEDESP2")==0 ) chv->type=cht_ESP;
			else if(strcmp(type,"PTRIMMEDABSOLUTE")==0 ) chv->type=cht_GLOBAL;
			else assert(0);
		
			/* set the real constant */
			chv->real_const=real_const;
			
			/* add to hashtable */
			Hashtable_put(constants_hash, chk,chv);	
	
			if (!m_instructions[addr])
			{
 				// unknown size, unknown function
				wahoo::Instruction* instr = new wahoo::Instruction(addr, -1, NULL);
				m_instructions[addr] = instr;
			}

//			fprintf(stderr,"@ 0x%x marking instruction as stack reference\n", addr);
			m_instructions[addr]->markStackRef();
			m_instructions[addr]->setSize(size_type_u.size);

                        // see if we can pick up access to local variables
			// and access to arguments off esp
			fgets(remainder, sizeof(remainder), fin);
			if (strstr(remainder, "var_") || 
                            (strstr(remainder, "arg_") && strstr(remainder, "[esp")))
			{
				m_instructions[addr]->markVarStackRef();
 			}
			

			// b/c we ate up <remainder> we have to do this
			line++;
			continue;
		}
		else if(strcmp(type,"DATAREF")==0)
		{
			char name[1000], parent_child[1000], offset_str[1000];
			int id, parent_id, offset, parent_offset;
			if(size_type_u.size<=0)
			{
			}
			else if(strcmp(scope,"GLOBAL")==0)
			{
				/* remaining params id, addr, parent/child, name */
				fscanf(fin, "%d%x%s%s", &id, &addr, parent_child);

				if(strcmp(parent_child, "PARENT")==0)
				{
				}
				else if(strcmp(parent_child, "CHILDOF")==0)
				{
				}
				else
					assert(0);

			}
			else if(strcmp(scope,"STACK")==0)
			{
				char esp[1000], plus[1000], offset_str[1000];
				int esp_offset;

				/* remaining params id, addr, parent/child, name */
				fscanf(fin, "%d%s%s%d%s", &id, &esp, &plus, &esp_offset, parent_child);

				assert(strcmp(esp, "esp")==0 && strcmp(plus,"+")==0);

				if(strcmp(parent_child, "PARENT")==0)
				{
					/* add to the stackref hashtable, also record the id->stackref mapping so we can
					 * can easily lookup the id for any fields we find.
					 */
                        		stackref_hash_value_t *sshv=add_stack_ref(addr,size_type_u.size, esp_offset);
					
//					printf("New stack frame at: pc=0x%x size=0x%x  HT-count=%d\n", addr, sshv->size, stackrefs_hash->count);


				}
				else if(strcmp(parent_child, "CHILDOF")==0)
				{
/*
need to pick up size of outargsregion
any esp access within this region should not be transformed
any esp access outside this region (esp + K) >= (esp + size) can be xformed

   80482fd     28 DATAREF STACK 3123 esp + 0 CHILDOF 3122 OFFSET 0 OutArgsRegion OUTARGS
   80482fd      4 DATAREF STACK 3124 esp + 28 CHILDOF 3122 OFFSET 28 LOCALVAR var_20
   80482fd      4 DATAREF STACK 3125 esp + 32 CHILDOF 3122 OFFSET 32 LOCALVAR var_1C
   80482fd      4 DATAREF STACK 3126 esp + 36 CHILDOF 3122 OFFSET 36 LOCALVAR var_18
*/
					fgets(remainder, sizeof(remainder), fin);
					if (strstr(remainder,"OutArgsRegion"))
					{
//fprintf(stderr," found OutArgsRegion @ 0x%08x\n", addr);
						m_instructions[addr]->getFunction()->setOutArgsRegionSize(size_type_u.size);
					}
					line++;
					continue;

				}
				else
					assert(0);
			}

		}
		else if (strcmp(type,"DEALLOC")==0)
		{
			assert(strcmp(scope,"STACK")==0);
			/* remaining params: comment */

			// handle the two forms of   add %esp, X
			// for reclaiming stack space
			if (addr - prevStackDeallocPC == 3 || addr - prevStackDeallocPC == 6)
			{
//				fprintf(stderr, "Detected nice stack deallocation instruction at 0x%x\n", prevStackDeallocPC);
				assert(m_instructions[prevStackDeallocPC]);
				m_instructions[prevStackDeallocPC]->markDeallocSite();
				m_instructions[prevStackDeallocPC]->setSize(addr - prevStackDeallocPC);
			
			}
			// handle leave/ret
			else if (addr - prevStackDeallocPC == 1)
			{
				assert(m_instructions[prevStackDeallocPC]);
				m_instructions[prevStackDeallocPC]->markDeallocSite();
				m_instructions[prevStackDeallocPC]->setSize(1);
			}
		}
		else if(strcmp(type,"BLOCK")==0 )
		{
		}
		else
		{
			fprintf(stderr, "Fatal, Unknown type at line %d\n", line);
		}
	
		fgets(remainder, sizeof(remainder), fin);
		line++;
	} while(!feof(fin));
	fclose(fin);

  // for each instruction in a function, dissassemble and stash away assembly string
	dissassemble();
}

/*
* Read MEDS annotation file and populate relevant hash table & data structures
*/
void Rewriter::readElfFile(char p_filename[])
{
	char buf[1000];
	sprintf(buf, "objdump -d --prefix-addresses %s | grep \"^[0-9]\"", p_filename);
	FILE* pin=popen(buf, "r");
	int addr;

	assert(pin);

	fscanf(pin, "%x", &addr);
	fgets(buf,sizeof(buf),pin);
	do 
	{
		if(m_instructions[addr]==NULL)
			m_instructions[addr]=new wahoo::Instruction(addr,-1,NULL);
		fscanf(pin,"%x", &addr);
		fgets(buf,sizeof(buf),pin);
	} while(!feof(pin));

	pclose(pin);

	dissassemble();
}


/*
*   for all instructions, dissassemble them using the BeaEngine
*/
void Rewriter::dissassemble()
{
  	// for every instruction, grab from ELF
  	// disassemble

	vector<wahoo::Instruction*> instructions=getAllInstructions(); 

    	for (int j = 0; j < instructions.size(); ++j)
    	{
      		wahoo::Instruction *instr = instructions[j];

      		// disassemble using BeaEngine
      		DISASM disasm;
      		memset(&disasm, 0, sizeof(DISASM));

      		disasm.Options = NasmSyntax + PrefixedNumeral;
      		disasm.Archi = 32;
      		disasm.EIP = (UIntPtr) getElfReader()->getInstructionBuffer(instr->getAddress());
      		disasm.VirtualAddr = instr->getAddress();

      		int instr_len = Disasm(&disasm);

      		instr->setAsm(string(disasm.CompleteInstr));  

      		instr->setSize(instr_len);
      		instr->setData((void*)disasm.EIP);
    	}
}

void Rewriter::addSimpleRewriteRule(wahoo::Function* p_func, char *p_origInstr, int p_origSize, app_iaddr_t p_origAddress, char *p_newInstr)
{
  char buf[1024];
  char aspri[2048];
  
  aspri[0] = '\0';

  sprintf(buf,"# orig(%d): %s\n", p_origSize, p_origInstr);
  strcpy(aspri, buf);
  sprintf(buf,"0x%08x -> .\n", p_origAddress);
  strcat(aspri, buf);
  sprintf(buf,". ** %s\n", p_newInstr);
  strcat(aspri, buf);
  sprintf(buf,". -> 0x%08x\n", p_origAddress + p_origSize);
  strcat(aspri, buf);

  fprintf(stderr, "WOULD EMIT RULE: %s\n", aspri);
  p_func->addRewriteRule(string(aspri));
}

// commit function to SPRI file
void Rewriter::commitFn2SPRI(wahoo::Function *p_func, FILE *p_fp)
{
  if (!p_func) return;

  for (int i = 0; i < p_func->getRewrites().size(); ++i)
  {
    string rewriteRule = p_func->getRewrites()[i];
    fprintf(p_fp, "%s\n", rewriteRule.c_str());
  }
}

vector<wahoo::Function*> Rewriter::getCandidateFunctions()
{
  vector<wahoo::Function*> candidates;
  for (map<app_iaddr_t, wahoo::Function*>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
  {
    app_iaddr_t addr = it->first;
    wahoo::Function* f = it->second;

    // null fn or fn marked as SAFE by MEDS -- we don't xform those
    if (!f || f->isSafe()) continue;

    candidates.push_back(f);
  }

  return candidates;
}

vector<wahoo::Function*> Rewriter::getNonCandidateFunctions()
{
  vector<wahoo::Function*> nonCandidates;
  for (map<app_iaddr_t, wahoo::Function*>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
  {
    app_iaddr_t addr = it->first;
    wahoo::Function* f = it->second;

    // null fn or fn marked as SAFE by MEDS -- we don't xform those
    if (!f) continue;
    if (f->isSafe())
      nonCandidates.push_back(f);
  }

  return nonCandidates;
}

vector<wahoo::Function*> Rewriter::getAllFunctions()
{
  vector<wahoo::Function*> allFunctions;

  for (map<app_iaddr_t, wahoo::Function*>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
  {
    app_iaddr_t addr = it->first;
    wahoo::Function* f = it->second;

    if (!f) continue;

    allFunctions.push_back(f);
  }

  return allFunctions;
}

vector<wahoo::Instruction*> Rewriter::getAllInstructions()
{
  vector<wahoo::Instruction*> allInstructions;

  for (map<app_iaddr_t, wahoo::Instruction*>::iterator it = m_instructions.begin(); it != m_instructions.end(); ++it)
  {
    app_iaddr_t addr = it->first;
    wahoo::Instruction* instr = it->second;

    if (!instr) continue;

    allInstructions.push_back(instr);
  }

  return allInstructions;
}

map<wahoo::Function*, double> Rewriter::getFunctionCoverage(char *p_instructionFile)
{
  map<wahoo::Function*, double> coverage;

  ifstream infile;
  infile.open(p_instructionFile, ifstream::in);

  if(!infile.is_open())
  {
    cerr << "File containing instructions visited not found:" << p_instructionFile << endl;
    return coverage;
  }

  set<app_iaddr_t> visitedInstructions;

  infile.seekg(0,ios::end);
  size_t size = infile.tellg();
  infile.seekg(ios::beg);
  if( size == 0)
  {
      cerr << "File containing instructions visited is empty is empty\n"<<endl;
      return coverage;
  }

  while (infile.good())
  {
    int address = 0;
    
    infile>>hex>>address;

    visitedInstructions.insert((app_iaddr_t) address);
  }

/*

  FILE *fp = fopen(p_instructionFile, "r");
  if (!fp) {
    cerr << "File containing instructions visited not found:" << p_instructionFile << endl;
    return coverage;
  }

  cerr<<"func cover checkpoint2"<<endl;

  set<app_iaddr_t> visitedInstructions;

  while (!feof(fp))
  {
    int address = 0;
    fscanf(fp, "%x\n", &address);

    cerr<<"address = "<<address<<endl;

    visitedInstructions.insert((app_iaddr_t) address);
  }

  cerr<<"func cover checkpoint3"<<endl;

  fclose(fp);
*/

  vector<wahoo::Instruction*> allInstructions = getAllInstructions();

  for (int i = 0; i < allInstructions.size(); ++i)
  {
    wahoo::Instruction* instr = allInstructions[i];
    if (!instr) continue;

    if (visitedInstructions.count(instr->getAddress()))
      instr->setVisited();
  }

  // now all instructions have been marked as visited or not

  vector<wahoo::Function*> allFunctions = getAllFunctions();
  for (int i = 0; i < allFunctions.size(); ++i)
  {
    coverage[allFunctions[i]] = allFunctions[i]->getInstructionCoverage();
  }

  return coverage;
}
