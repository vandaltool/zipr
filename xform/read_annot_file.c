/*
 * shadow.c - see below.
 *
 * Copyright (c) 2000, 2001, 2010 - University of Virginia 
 *
 * This file is part of the Memory Error Detection System (MEDS) infrastructure.
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

// First pass, we'll handle the following annotations:
//
// FUNC               - function
// INSTR              - instruction
// MEMORYHOLE         - not allowed to write into these
//
// DATAREF STACK      - allocate stack
// DEALLOC STACK      - deallocate stack
//
// PTRIMMEDESP        - stack references off ESP
// PTRIMMEDEBP        - stack references off EBP

// #include <stdio.h>
// #include <string.h>
#include "all.h"

/*
 *  read_annot_file - read the annotations file provided by IDA Pro.
 */
void read_annot_file(char fn[])
{
	FILE* fin;
	app_iaddr_t addr;
	union { int size, type;} size_type_u;
	char type[200];
	char scope[200];
	char remainder[200000];
	char * objname;
	int pid=0;
	int var_length=0;
	int bitvector_size_bits=0;
	int line=0;


	constants_hash=	Hashtable_create(constants_compute_hash, constants_key_compare);
	stackrefs_hash=	Hashtable_create(stackrefs_compute_hash, stackrefs_key_compare);
	framerestores_hash=	Hashtable_create(framerestores_compute_hash, framerestores_key_compare);
	instrmaps_hash=	Hashtable_create(instrmaps_compute_hash, instrmaps_key_compare);
	funclists_hash=	Hashtable_create(funclists_compute_hash, funclists_key_compare);

	if(fn[0]==0)
		return;

	fin=fopen(fn, "r");

	if(!fin)
	{
		fprintf(stderr,"Cannot open strata annotation file %s\n", fn);
		return;
	}



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


		/* check for speculative annotations */
//		if(is_spec_annot && strata_opt_do_smp_verify_profile)
		if(is_spec_annot)
		{
			/* skip this annotation */
			fgets(remainder, sizeof(remainder), fin);
			line++;
			continue;
		}

		/* if the size > 0, then this is a declaration of a variable */
		if(strcmp(type,"FUNC")==0)
		{
			char name[20000];
			/* found function declaration */
			if(strcmp(scope,"GLOBAL")==0)
			{
				/* remaining parameters are name {USEFP, NOFP}  */

				funclist_hash_key_t *flhk=(funclist_hash_key_t*)spri_allocate_type(sizeof(funclist_hash_key_t));
				funclist_hash_value_t *flhv=(funclist_hash_value_t*)spri_allocate_type(sizeof(funclist_hash_value_t));
	
				fscanf(fin,"%s", name);
				flhk->name=spri_strdup(name);
				flhv->pc=addr;
//				STRATA_LOG("annot","Adding name=%s pc=%x to funclist hash table\n", flhk->name, flhv->pc);
				printf("Adding name=%s pc=%x to funclist hash table\n", flhk->name, flhv->pc);
				Hashtable_put(funclists_hash, flhk, flhv);	
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
				if(strcmp(safeness, "SAFE"))
					frame_restore_hash_set_safe_bit(addr,TRUE);
				else if(strcmp(safeness, "SPECSAFE"))
					frame_restore_hash_set_safe_bit(addr,TRUE);
				else if(strcmp(safeness, "UNSAFE"))
					frame_restore_hash_set_safe_bit(addr,FALSE);
				else
					fprintf(stderr,"Do not understand safeness terminoligy '%s' at line %d\n", safeness, line);
			}
			else
				fprintf(stderr,"Do not understand type terminoligy '%s' at line %d\n", type, line);
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
	
			printf("MEMORYHOLE, pc=%x offset=%d\n", addr, offset);
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
			printf("Adding pc=%x size=%d to stackref hash table\n", sshk->pc, sshv->size);

//			STRATA_LOG("annot","Adding pc=%x size=%d to stackref hash table\n", sshk->pc, sshv->size);
			Hashtable_put(stackrefs_hash, sshk,sshv);	

		}
		else if(strcmp(type,"INSTR")==0)
		{
			/* optimizing annotation about not needing heavyweight metadata update */
			/* ignore for now */


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
				if (Hashtable_get(stackrefs_hash, &addr))
				{
					printf("STACK ALLOC INSTRUCTION CONFIRMED AT pc=0x%x size=%d\n", sshk.pc, size_type_u);
				}
				
#ifdef OLD_MEDS
                    		char regname[100];
                    		int value = SAVE_EBP | SAVE_EDI | SAVE_ESI | SAVE_EDX | SAVE_ECX | SAVE_EBX | SAVE_EAX | SAVE_EFLAGS;
                    		do 
				{
                            		fscanf(fin, "%s", &regname);
                            		if (strcmp(regname, "EFLAGS") == 0)
                                    		value ^= SAVE_EFLAGS;
                            		else if (strcmp(regname, "EAX") == 0)
                                    		value ^= SAVE_EAX;
                            		else if (strcmp(regname, "EBX") == 0)
                                    		value ^= SAVE_EBX;
                            		else if (strcmp(regname, "ECX") == 0)
                                    		value ^= SAVE_ECX;
                            		else if (strcmp(regname, "EDX") == 0)
                                    		value ^= SAVE_EDX;
                            		else if (strcmp(regname, "ESI") == 0)
                                    		value ^= SAVE_ESI;
                            		else if (strcmp(regname, "EDI") == 0)
                                    		value ^= SAVE_EDI;
                            		else if (strcmp(regname, "EBP") == 0)
                                    		value ^= SAVE_EBP;
                    		} while (strcmp(regname, "ZZ"));
                    		register_hash_key_t* key = spri_allocate_type(sizeof(register_hash_key_t));
                    		register_hash_value_t* val = spri_allocate_type(sizeof(register_hash_value_t));
                    		key->pc = addr;
                    		val->value = value; 
                    		Hashtable_put(register_hash,key,val); 
#endif
            		}
            		else if (strcmp(scope, "INDIRECTCALL") == 0)
			{
				/* ignore INSTR INDIRECTCALL annotations, profiler generated, analyzer uses only */
			}
#ifdef OLD_MEDS
            		else if (strcmp(scope, "FAULT") == 0)
			{
				int count;
				fscanf(fin, "%d", &count);
				recordfault_add_fault(addr,count);
			}
            		else if (strcmp(scope, "CHILDACCESS") == 0)
			{
				char start[100];
				char end[100];
				int istart, iend;
				do
				{
					fscanf(fin, "%s", start);
					if(strcmp(start,"ZZ")==0)
						break;
					fscanf(fin, "%s", end);
					istart=atoi(start);
					iend=atoi(end);
					add_fgrange(addr, istart, istart+iend-1);
				}
				while(1);
				
			}
#endif
            		else if (strcmp(scope, "BELONGTO") == 0)
			{
				app_iaddr_t func_addr;
				fscanf(fin, "%x", &func_addr);
                    		instrmap_hash_key_t* key = (instrmap_hash_key_t*)spri_allocate_type(sizeof(instrmap_hash_key_t));
                    		instrmap_hash_value_t* val = (instrmap_hash_value_t*)spri_allocate_type(sizeof(instrmap_hash_value_t));
                    		key->pc = addr;
                    		val->func_addr = func_addr; 
                    		Hashtable_put(instrmaps_hash,key,val); 
			}
#ifdef OLD_MEDS
            		else if (strcmp(scope, "PROF_FIELD") == 0)
			{
				int num_elems_in_hex_rep=0;
				int i=0;
				int j=0;
				unsigned int cur_element=0;
				profiler_data_hash_value_t* pdhv=NULL;
				bitvector_t * the_read_in_bitvec=NULL;
				char * the_bitvector=NULL;
				char temp_obj_name[1000];
				/* This case catches profiler fields information */
	
				/*
					Read the rest of the line (format below):
					PC 0 PROF_FIELD objname pid var_length bitvector_size_bits bitvector
			 	*/
	
				/* copy scope to objname */
				fscanf(fin, "%s", temp_obj_name);
				objname=spri_strdup(temp_obj_name);
	
				/* lookup in hash table */
				pdhv=get_profiler_data(addr, objname);
	
	
	/* FIXME:  if bit vector dumped as hex */
	
				/* allocate memory for the bitvector */
				fscanf(fin, "%d%d", &pid, &var_length);
				if(var_length>0)
				{
					fscanf(fin, "%d%d", &bitvector_size_bits, &num_elems_in_hex_rep);
	
					the_bitvector= spri_allocate_type(((bitvector_size_bits/8)+1)*sizeof(char) + 4);
	
					/* read the values in to the bit vector */
					for(i=0, j=0; i < num_elems_in_hex_rep; i++, j+=4)
					{
						/* read an int */
						fscanf(fin, "%x", &the_bitvector[j]);
					
					}
					/* create a bitvector structure to hold the info */
					the_read_in_bitvec=spri_allocate_type(sizeof(bitvector_t));
					the_read_in_bitvec->the_bits=the_bitvector;
					the_read_in_bitvec->size=bitvector_size_bits;
					the_read_in_bitvec->num_bytes=(bitvector_size_bits/8)+1;
	
				}
	
	
	
				/* merge the bit vector with the bitvector found in the hash table, if found */
				/* if found */
				if(pdhv)
				{
					if(var_length>0)
					{
						int i=0;
						for (i=0; i<bitvector_size_bits;i++)
						{
							/* if the bit is 1, then set it 
					 		* in the merged version
					 		*/
							if(get_bit(the_read_in_bitvec,i))
							{
								set_bitvector(pdhv->data_bitvector, i);
							}
		
						}
						/* in this case, we can free the_red_in_bitvec */
						free_bitvector(the_read_in_bitvec);
					}
				}
				else /* not found, add it to the hash table */
				{
					/* add to the profiler_data_hash table */
					add_profiler_data(addr, objname, var_length, the_read_in_bitvec);
				}


			}
#endif
            		else 
            		{
              			assert(strcmp(scope,"LOCAL")==0);

                    		switch(annot_type)
                    		{
					/* No Meta Data Updates */
					case -1:	/* no meta data updates */
							/* remaining params: <reason> comment */
					{
#ifdef OLD_MEDS
						nometa_hash_key_t *nmhk=spri_allocate_type(sizeof(nometa_hash_key_t));
						nometa_hash_value_t *nmhv
							=spri_allocate_type(sizeof(nometa_hash_value_t));
	
						/* add to hashtable, a name would be nice someday */
						nmhk->pc=addr;
//						STRATA_LOG("annot","Adding nometa=%x to nowarn hash\n", nmhk->pc);
						Hashtable_put(nometas_hash, nmhk,nmhv);	
						break;
#endif
					}
	
					/* fast updates */
					case -2:	/* fast meta data updates, results = always number */
							/* remaining params: <reason> reg, {reg, ...} ZZ comment */
					{
#ifdef OLD_MEDS
						char n_buffer[1000], reg_buffer[1000];
						reg_buffer[0]=0;
						int reg_map=0;

						fscanf(fin, "%s", n_buffer);
						assert(strcmp(n_buffer,"n")==0);

						while(1)	// loop until we find a ZZ 
						{
							fscanf(fin, "%s", reg_buffer);
							if(strcmp(reg_buffer,"EAX")==0 || strcmp(reg_buffer,"AL")==0
							   || strcmp(reg_buffer,"AH")==0 || strcmp(reg_buffer,"AX")==0)
							{
								reg_map|=SET_EAX;
							}
							else if(strcmp(reg_buffer,"EBX")==0 || strcmp(reg_buffer,"BL")==0
							   || strcmp(reg_buffer,"BH")==0 || strcmp(reg_buffer,"BX")==0)
							{
								reg_map|=SET_EBX;
							}
							else if(strcmp(reg_buffer,"ECX")==0 || strcmp(reg_buffer,"CL")==0
							   || strcmp(reg_buffer,"CH")==0 || strcmp(reg_buffer,"CX")==0)
							{
								reg_map|=SET_ECX;
							}
							else if(strcmp(reg_buffer,"EDX")==0 || strcmp(reg_buffer,"DL")==0
							   || strcmp(reg_buffer,"DH")==0 || strcmp(reg_buffer,"DX")==0)
							{
								reg_map|=SET_EDX;
							}
							else if(strcmp(reg_buffer,"EDI")==0)
							{
								reg_map|=SET_EDI;
							}
							else if(strcmp(reg_buffer,"ESI")==0)
							{
								reg_map|=SET_ESI;
							}
							else if(strcmp(reg_buffer,"EBP")==0)
							{
								reg_map|=SET_EBP;
							}
							else if(strcmp(reg_buffer,"ZZ")==0)
							{
								break;
							}
							else
								assert(0);
						}
                    				fast_update_hash_key_t* key = spri_allocate_type(sizeof(fast_update_hash_key_t));
                    				fast_update_hash_value_t* val = spri_allocate_type(sizeof(fast_update_hash_value_t));
                    				key->pc = addr;
                    				val->value = reg_map; 
                    				Hashtable_put(fast_update_hash,key,val); 
						
						break;
#endif
					}

					/* skip warning message on this instruction, promised safe */
					case -3: 	/* remaining params: NoWarn comment */
					{
#ifdef OLD_MEDS
						nowarn_hash_key_t *nwhk=spri_allocate_type(sizeof(nowarn_hash_key_t));
						nowarn_hash_value_t *nwhv
							=spri_allocate_type(sizeof(nowarn_hash_value_t));
	
						/* add to hashtable, a name would be nice someday */
						nwhk->pc=addr;
						STRATA_LOG("annot","Adding nowarn=%x to nowarn hash\n", nwhk->pc);
						Hashtable_put(nowarns_hash, nwhk,nwhv);	
#endif
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
							size_type_u.type, line, fn);
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
//			STRATA_LOG("annot","Adding %x:%d type=%d to constant hash table\n", chk->pc, chk->the_const, chv->type );
			Hashtable_put(constants_hash, chk,chv);	
			
		}
		else if(strcmp(type,"DATAREF")==0)
		{
			char name[1000], parent_child[1000], offset_str[1000];
			int id, parent_id, offset, parent_offset;
			if(size_type_u.size<=0)
			{
//				STRATA_LOG("warn", "Found DATAREF of size <=0 at line %d of annot file\n", line);
			}
			else if(strcmp(scope,"GLOBAL")==0)
			{
				/* remaining params id, addr, parent/child, name */
				fscanf(fin, "%d%x%s%s", &id, &addr, parent_child);

				if(strcmp(parent_child, "PARENT")==0)
				{
#ifdef OLD_MEDS
					fscanf(fin, "%s", name);
					add_referent(spri_strdup(name),addr,size_type_u.size, 0, 0);

					if(strata_opt_do_smp_fine_grain && !STRATA_LOG_IS_ON("no_fine_grain_static"))
						add_to_referent_id_map(id,find_referent(addr));
#endif
				}
				else if(strcmp(parent_child, "CHILDOF")==0)
				{
#ifdef OLD_MEDS
					if(strata_opt_do_smp_fine_grain && !STRATA_LOG_IS_ON("no_fine_grain_static"))
					{
						referent_object_t* new_ref;
						fscanf(fin, "%d%s%d", &parent_id, offset_str, &parent_offset);
						assert(strcmp("OFFSET", offset_str)==0);
						referent_object_t *refnt=get_referent_from_id_map(parent_id);
						new_ref=add_referent_field(refnt, parent_offset, addr, size_type_u.size);
						add_to_referent_id_map(id,new_ref);
					}
#endif
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
					
					printf("New stack frame at: pc=0x%x size=0x%x\n", addr, sshv->size);

#ifdef OLD_MEDS
					//if(strata_opt_do_smp_fine_grain && !STRATA_LOG_IS_ON("no_fine_grain_stack"))
					if(!STRATA_LOG_IS_ON("no_fine_grain_stack"))
						add_to_stackref_id_map(id,sshv);

                        		/* add to hashtable, a name would be nice someday */
                        		STRATA_LOG("annot","Adding pc=%x size=%d to stackref hash table\n", addr, sshv->size);
#endif
				}
				else if(strcmp(parent_child, "CHILDOF")==0)
				{
#ifdef OLD_MEDS
					if(strata_opt_do_smp_fine_grain && !STRATA_LOG_IS_ON("no_fine_grain_stack"))
					{
						fscanf(fin, "%d%s%d", &parent_id, offset_str, &parent_offset);
						assert(strcmp("OFFSET", offset_str)==0);
                        			stackref_hash_value_t *sshv=get_stackref_from_id_map(parent_id);
                        			stackref_hash_value_t *new_sshv=add_stack_ref_field(sshv, addr, 
							size_type_u.size, parent_offset);
						add_to_stackref_id_map(id,new_sshv);
					}
#endif
				}
				else
					assert(0);
			}

		}
		else if (strcmp(type,"DEALLOC")==0)
		{
			assert(strcmp(scope,"STACK")==0);
			/* remaining params: comment */
		}
		else if(strcmp(type,"PROFILEDNUMERIC")==0)
		{
#ifdef OLD_MEDS
			/* profiler generated information */
			profile_loads_add_profile_data(addr,atoll(scope),FALSE,FALSE);
#endif
		}
		else if(strcmp(type,"PROFILEDPOINTER")==0 )
		{
#ifdef OLD_MEDS
			/* profiler generated information */
			profile_loads_add_profile_data(addr,FALSE,atoll(scope),FALSE);
#endif
		}
		else if(strcmp(type,"PROFILEDOTHER")==0 )
		{
#ifdef OLD_MEDS
			/* profiler generated information */
			profile_loads_add_profile_data(addr,FALSE,FALSE,atoll(scope));
#endif
		}
		else if(strcmp(type,"BLOCK")==0 )
		{
#ifdef OLD_MEDS
			if(strcmp(scope,"PROFILECOUNT")==0)
			{
				/* record that we need to profile this block */
				profileblocks_add_profile_data(addr);
			}
			else if(strcmp(scope,"COUNT")==0)
			{
				/* do nothing */
			}
			else	
			{
//				strata_log("fatal", "Fatal, Unknown scope at line %d\n", line);
				fprintf(stderr, "Fatal, Unknown scope at line %d\n", line);
			}
				

#endif
		}
		else
		{
		//	strata_log("fatal", "Fatal, Unknown type at line %d\n", line);
			fprintf(stderr, "Fatal, Unknown type at line %d\n", line);
		}
	
		fgets(remainder, sizeof(remainder), fin);
		line++;
	} while(!feof(fin));
	fclose(fin);
}
