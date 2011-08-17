#include "beaengine/BeaEngine.h"
#include "p1transform.h"
#include "transformutils.h"

using namespace libIRDB;
using namespace std;

/*
static set<std::string> getFunctionList(char *p_filename)
{
	set<std::string> functionList;

	ifstream candidateFile;
	candidateFile.open(p_filename);

	if(candidateFile.is_open())
	{
		while(!candidateFile.eof())
		{
			string functionName;
			getline(candidateFile, functionName);

			functionList.insert(functionName);
		}

		candidateFile.close();
	}

	return functionList;
}
*/

P1Transform::P1Transform()
{
  /* Initialize regular expressions */
  /* # mov dword [ebp+eax*4-0x70] , edx */
  /* regex_t m_fancy_ebp_pattern; */
  if (regcomp(&m_fancy_ebp_pattern, "(.*)(ebp[+].*-)(.*)(])(.*)", REG_EXTENDED | REG_ICASE) != 0)
    {
      fprintf(stderr,"Error: regular expression for <ebp+<stuff>*4-K> failed to compile\n");
      exit(1);
    }
  
  /* match <anything>[ebp-<K>]<anything>*/
  if (regcomp(&m_stack_ebp_pattern, "(.+)[[:blank:]]*ebp[[:blank:]]*-(.+)[[:blank:]]*(])(.*)", REG_EXTENDED | REG_ICASE) != 0)
    {
      fprintf(stderr,"Error: regular expression for <ebp-K> failed to compile\n");
      exit(1);
    }
  
  /* match <anything>[esp+<K>]<anything>*/
  if (regcomp(&m_stack_esp_pattern, "(.+)[[:blank:]]*esp[[:blank:]]*[+](.+)[[:blank:]]*(])(.*)", REG_EXTENDED | REG_ICASE) != 0)
    {
      fprintf(stderr,"Error: regular expression for <esp+K> failed to compile\n");
      exit(1);
    }
  
  /* stack allocation: match sub esp , K*/
  if (regcomp(&m_stack_alloc_pattern, "[[:blank:]]*sub[[:blank:]]*esp[[:blank:]]*,[[:blank:]]*(.+)", REG_EXTENDED | REG_ICASE) != 0)
    {
      fprintf(stderr,"Error: regular expression for <sub esp, K> failed to compile\n");
      exit(1);
    }
  
  /* stack deallocation: match add esp , K*/
  if (regcomp(&m_stack_dealloc_pattern, "[[:blank:]]*add[[:blank:]]*esp[[:blank:]]*,[[:blank:]]*(.+)", REG_EXTENDED | REG_ICASE) != 0)
    {
      fprintf(stderr,"Error: regular expression for <add esp, K> failed to compile\n");
      exit(1);
    }
  
  /* match lea <anything> dword [<stuff>]*/
  if (regcomp(&m_lea_hack_pattern, "(.*lea.*,.*)dword(.*)", REG_EXTENDED | REG_ICASE) != 0)
    {
      fprintf(stderr,"Error: regular expression for lea hack failed to compile\n");
      exit(1);
    }
}

bool P1Transform::rewrite(libIRDB::VariantIR_t *virp, libIRDB::Function_t *f, std::map<Instruction_t*, std::string> & undoList)
{
  int stack_frame_padding = getStackFramePadding(f);
  int stack_frame_size = -1;
  int new_stack_frame_size = -1;
  
  bool stackAlloc = false;
  bool stackDealloc = false;      
  bool rewriteFunction = false;
  
  for(
      set<Instruction_t*>::const_iterator it=f->GetInstructions().begin();
      it!=f->GetInstructions().end();
      ++it)
    {
      Instruction_t* instr=*it;
      char buf[1024];

      DISASM disasm;
      instr->Disassemble(disasm);
      sprintf(buf, "%s", disasm.CompleteInstr);
  
      int k = 10;
      regmatch_t pmatch[k];
      memset(pmatch, 0,sizeof(regmatch_t) * k);
      
      /* AllocSite Section */	
      if(regexec(&m_stack_alloc_pattern, buf, 5, pmatch, 0)==0)
	{
	  char new_instr[2048];
	  char matched[1024];
	  /* extract K from: sub esp, K */
	  if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
	    {
	      int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
	      strncpy(matched, &buf[pmatch[1].rm_so], mlen);
	      matched[mlen] = '\0';
	      /* extract K */
	      sscanf(matched,"%x", &stack_frame_size);
	      /* add padding */
	      new_stack_frame_size = stack_frame_size + stack_frame_padding;
	      sprintf(new_instr, "sub esp, 0x%x", new_stack_frame_size); 
	      /* assemble the instruction into raw bits */
              undoList[instr] = instr->GetDataBits();
              if (!instr->Assemble(new_instr)) 
		{
		  return false;
		} 
	      stackAlloc=true;
	    }
	}
      /* matches dealloc site: add esp, K */
      else if(regexec(&m_stack_dealloc_pattern, buf, 5, pmatch, 0)==0)
	{
	  char new_instr[2048];
	  char matched[1024];
	  if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
	    {
	      int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
	      strncpy(matched, &buf[pmatch[1].rm_so], mlen);
	      matched[mlen] = '\0';
	      
	      sscanf(matched,"%x", &stack_frame_size);
	      new_stack_frame_size = stack_frame_size + stack_frame_padding;
	      sprintf(new_instr, "add esp, 0x%x", new_stack_frame_size);
	      
	      undoList[instr] = instr->GetDataBits();
	      if (!instr->Assemble(new_instr)) 
		{
		  return false;
		}
	      
              instr->SetComment(instr->GetComment() + " p1-xformed");
	      stackDealloc = true;
	    }
	}
      else if (strstr(buf, "leave"))
	{
	  stackDealloc = true;
	}
      /* matched: lea <anything> dword [<stuff>] */
      else {
	if(regexec(&m_lea_hack_pattern, buf, 5, pmatch, 0)==0)
	  {
	    int k;
	    char tmp[1024];
	    char matched[1024];
	    memset(matched, 0,1024);
	    for (k = 0; k < 5; ++k)
	      {
		if (pmatch[k].rm_so >= 0 && pmatch[k].rm_eo >= 0) 
		  {
		    int mlen = pmatch[k].rm_eo - pmatch[k].rm_so;
		    strncpy(matched, &buf[pmatch[k].rm_so], mlen);
		    matched[mlen] = '\0';
		    
		    if (k == 1)
		      strcpy(tmp, matched);
		    else if (k == 2)
		      strcat(tmp, matched);
		  }
	      }
	    strcpy(buf, tmp);
	  }
	if(regexec(&m_stack_ebp_pattern, buf, 5, pmatch, 0)==0)
	  {
	    char new_instr[2048];
	    for (k = 0; k < 8; ++k)
	      {
		char matched[1024];
		if (pmatch[k].rm_so >= 0 && pmatch[k].rm_eo >= 0) 
		  {
		    int mlen = pmatch[k].rm_eo - pmatch[k].rm_so;
		    strncpy(matched, &buf[pmatch[k].rm_so], mlen);
		    matched[mlen] = '\0';
		    
		    if (k == 1) {
		      strcpy(new_instr, matched);
		    }
		    else if (k == 2) {
		      unsigned offset;
		      char offset_str[128];
		      sscanf(matched,"%x", &offset);
		      offset += stack_frame_padding;
		      sprintf(offset_str,"ebp - 0x%0x", offset);
		      strcat(new_instr, offset_str);
		    }
		    else if (strlen(matched) > 0)
		      {
			strcat(new_instr, matched);
		      }
		  }
	      }
	    undoList[instr] = instr->GetDataBits();
	    if (!instr->Assemble(new_instr)) 
              {
                return false;
              }
	    
	    instr->SetComment(instr->GetComment() + " p1-xformed");
	  }
	else if(regexec(&m_fancy_ebp_pattern, buf, 8, pmatch, 0)==0)
	  {
	    char matched[1024];
	    char new_instr[1024];
	    memset(matched, 0,1024);
	    for (k = 0; k < 8; ++k)
	      {
		if (pmatch[k].rm_so >= 0 && pmatch[k].rm_eo >= 0) 
		  {
		    int mlen = pmatch[k].rm_eo - pmatch[k].rm_so;
		    strncpy(matched, &buf[pmatch[k].rm_so], mlen);
		    matched[mlen] = '\0';
		    
		    if (mlen <= 0)
		      continue;
		    
		    if (k == 0) {;}
		    else if (k == 1)
		      strcpy(new_instr, matched);
		    else if (k == 3)
		      {
			unsigned offset;
			char offset_str[128];
			sscanf(matched,"%x", &offset);
			offset += stack_frame_padding;
			sprintf(offset_str,"0x%0x", offset);
			strcat(new_instr, offset_str);
		      }
		    else
		      strcat(new_instr, matched);
		  }
	      }
	    undoList[instr] = instr->GetDataBits();
	    
	    if (!instr->Assemble(new_instr)) 
              {
                return false;
              }
	    instr->SetComment(instr->GetComment() + " p1-xformed");
	  }
	else if(regexec(&m_stack_esp_pattern, buf, 5, pmatch, 0)==0 && !f->GetUseFramePointer())
	  {
	    char new_instr[2048];
	    unsigned originalOffset = 0;
	    for (k = 0; k < 8; ++k)
	      {
		char matched[1024];
		if (pmatch[k].rm_so >= 0 && pmatch[k].rm_eo >= 0) 
		  {
		    int mlen = pmatch[k].rm_eo - pmatch[k].rm_so;
		    strncpy(matched, &buf[pmatch[k].rm_so], mlen);
		    matched[mlen] = '\0';
		    
		    if (k == 1) {
		      strcpy(new_instr, matched);
		    }
		    else if (k == 2) {
		      unsigned offset = 0;
		      char offset_str[128];
		      sscanf(matched,"%x", &offset);
		      originalOffset = offset;
		      offset += stack_frame_padding;
		      sprintf(offset_str,"esp + 0x%0x", offset);
		      strcat(new_instr, offset_str);
		    }
		    else if (strlen(matched) > 0)
		      {
			strcat(new_instr, matched);
		      }
		  }
	      }
	    
	    int sizeOutArgs = f->GetOutArgsRegionSize();
	    if (originalOffset >= sizeOutArgs){
              undoList[instr] = instr->GetDataBits();
	      
              if (!instr->Assemble(new_instr)) 
		{
		  return false;
		}	    
              instr->SetComment(instr->GetComment() + " p1-xformed");
	    }
	  }
      }
    }
  
  if (!stackAlloc)
    {
      fprintf(stderr,"Could not process function <%s>: no stack allocation routine found\n", f->GetName().c_str());
      return false;
    } 
  else if (!stackDealloc)
    {
      fprintf(stderr,"Could not process function <%s>: no stack deallocation routine found\n", f->GetName().c_str());
      return false;
    } 
  else{
    rewriteFunction=true;
  }
  return rewriteFunction;
}

// return stack frame size padding
// sure we pad by at least 8 bytes
int P1Transform::getStackFramePadding(libIRDB::Function_t *p_fn)
{
  // @todo: add some random variation
  int stack_frame_padding = p_fn->GetStackFrameSize();
  if (stack_frame_padding < 8) stack_frame_padding = 8;
  
  return stack_frame_padding;
}

static void undo(map<libIRDB::Instruction_t*, string> undoList)
{
  // rollback any changes
  for(
      map<Instruction_t*, std::string>::const_iterator mit=undoList.begin();
      mit != undoList.end();
      ++mit)
    {
      Instruction_t* insn = mit->first;
      std::string dataBits = mit->second;
  
      DISASM disasm;
      insn->Disassemble(disasm);
      insn->SetDataBits(dataBits);
    }
}

int main(int argc, char **argv)
{
  if(argc!=3)
    {
      cerr<<"Usage: [the executable] <variantid> [file containing name of blacklisted functions]"<<endl;
      exit(-1);
    }
  
  VariantID_t *pidp=NULL;
  VariantIR_t *virp=NULL;
  
  int progid = atoi(argv[1]);
  
  //setup the interface to the sql server 
  pqxxDB_t pqxx_interface;
  BaseObj_t::SetInterface(&pqxx_interface);
  
  try
    {
      // read the variant ID using variant id number = atoi(argv[1])
      pidp=new VariantID_t(atoi(argv[1]));
      
      // verify that we read it correctly.
      assert(pidp->IsRegistered()==true);
      
      // read the IR from the db
      virp=new VariantIR_t(*pidp);
    }
  catch (DatabaseError_t pnide)
    {
      cout<<"Unexpected database error: "<<pnide<<endl;
      exit(-1);
    }
  
  P1Transform *p1Transform = new P1Transform();

	set<std::string> blackListOfFunctions;

	if (argc == 3)
	{
		blackListOfFunctions = getFunctionList(argv[2]);
	}

  vector<std::string> functionsTransformed;
  try {
    //iterate through the functions that compose a particular variant
    for(
	set<Function_t*>::const_iterator it=virp->GetFunctions().begin();
	it!=virp->GetFunctions().end();
	++it
	)
      {
	Function_t* func=*it;
	map<libIRDB::Instruction_t*, std::string> undoList;
	
	if (blackListOfFunctions.find(func->GetName()) != blackListOfFunctions.end())
		continue;

	cerr << "P1: Looking at function: " << func->GetName() << endl;
	
	//perform the p1 transform on the given variant's function
	bool rewriteFunction = p1Transform->rewrite(virp,func,undoList); 
	
	if (!rewriteFunction)
	  {
	    fprintf(stderr,"P1: undo transform: %d instructions to rollback for function %s\n", undoList.size(), func->GetName().c_str());
	    undo(undoList);
	  }
	else {
	  string dirname = "p1.xform/" + func->GetName();
	  string cmd = "mkdir -p " + dirname;
	  system(cmd.c_str());
	  
	  string filename = dirname + "/a.irdb.aspri";
	  ofstream aspriFile;
	  aspriFile.open(filename.c_str());
	  if(!aspriFile.is_open())
	    {
	      fprintf(stderr, "P1: Could not open: %s\n", filename.c_str());
	      continue;
	    }
	  
	  fprintf(stderr, "P1: generating aspri file: %s\n", filename.c_str());
	  virp->generate_spri(aspriFile); // p1.xform/<function_name>/a.irdb.aspri
	  aspriFile.close();
	  
	  char new_instr[1024];
	  //This script generates the aspri and bspri files; it also runs BED
	  sprintf(new_instr, "$PEASOUP_HOME/tools/p1xform_v2.sh %d %s", progid, func->GetName().c_str());
	  
	  //If OK=BED(func), then commit 
	  fprintf (stderr, "P1: about to execute\n", new_instr);
	  
	  int rt=system(new_instr);
	  int actual_exit = -1, actual_signal = -1;
	  if (WIFEXITED(rt)) actual_exit = WEXITSTATUS(rt);
	  else actual_signal = WTERMSIG(rt);
	  int retval = actual_exit;
	  
	  if(retval == 0){
	    //Run BED; if passed, commit to DB
	    virp->WriteToDB();
	    functionsTransformed.push_back(func->GetName()); 
	  }     
	  else {
	    undo(undoList);
	  }
	}
      }
    pqxx_interface.Commit();      
  }
  catch (DatabaseError_t pnide)
    {
      cout<<"Unexpected database error: "<<pnide<<endl;
      exit(-1);
    }
  
  cout << "List of functions transformed: count = " << functionsTransformed.size() << endl;
  for (int i = 0; i < functionsTransformed.size(); ++i)
    cout << "function: " << functionsTransformed[i] << endl;
  
  return 0;
}

 
