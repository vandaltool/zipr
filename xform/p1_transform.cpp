#include <iostream>
#include <string.h>
#include <stdlib.h>

#include "p1_transform.h"

using namespace wahoo;

//
// P1 Transform
//
// This transform seeks to increase the stack frame size and move the local variables area
// within the padded region.
//
// author: Anh Nguyen-Tuong
// 

P1Transform::P1Transform(char *p_elf, char *p_annot, char *p_spri) : Rewriter(p_elf, p_annot, p_spri) 
{
  // Initialize regular expressions

  // # mov dword [ebp+eax*4-0x70] , edx
  // regex_t m_fancy_ebp_pattern;
  if (regcomp(&m_fancy_ebp_pattern, "(.*)(ebp[+].*-)(.*)(])(.*)", REG_EXTENDED | REG_ICASE) != 0)
  {
    fprintf(stderr,"Error: regular expression for <ebp+<stuff>*4-K> failed to compile\n");
    exit(1);
  }

  // match <anything>[ebp-<K>]<anything>
  if (regcomp(&m_stack_ebp_pattern, "(.+)[[:blank:]]*ebp[[:blank:]]*-(.+)[[:blank:]]*(])(.*)", REG_EXTENDED | REG_ICASE) != 0)
  {
    fprintf(stderr,"Error: regular expression for <ebp-K> failed to compile\n");
    exit(1);
  }

  // match <anything>[esp+<K>]<anything>
  if (regcomp(&m_stack_esp_pattern, "(.+)[[:blank:]]*esp[[:blank:]]*[+](.+)[[:blank:]]*(])(.*)", REG_EXTENDED | REG_ICASE) != 0)
  {
    fprintf(stderr,"Error: regular expression for <esp+K> failed to compile\n");
    exit(1);
  }

  // stack allocation: match sub esp , K
  if (regcomp(&m_stack_alloc_pattern, "[[:blank:]]*sub[[:blank:]]*esp[[:blank:]]*,[[:blank:]]*(.+)", REG_EXTENDED | REG_ICASE) != 0)
  {
    fprintf(stderr,"Error: regular expression for <sub esp, K> failed to compile\n");
    exit(1);
  }

  // stack deallocation: match add esp , K
  if (regcomp(&m_stack_dealloc_pattern, "[[:blank:]]*add[[:blank:]]*esp[[:blank:]]*,[[:blank:]]*(.+)", REG_EXTENDED | REG_ICASE) != 0)
  {
    fprintf(stderr,"Error: regular expression for <add esp, K> failed to compile\n");
    exit(1);
  }

  // match lea <anything> dword [<stuff>]
  if (regcomp(&m_lea_hack_pattern, "(.*lea.*,.*)dword(.*)", REG_EXTENDED | REG_ICASE) != 0)
  {
    fprintf(stderr,"Error: regular expression for lea hack failed to compile\n");
    exit(1);
  }
}

void P1Transform::rewrite(wahoo::Function *f, FILE* fp)
{
    app_iaddr_t addr = f->getAddress();

    fprintf(stderr,"#alloc site: %d\n", f->getStackAllocationInstructions().size());
    fprintf(stderr, "\n# function: %s\n", f->getName().c_str());

    int stack_frame_padding = getStackFramePadding(f);

    unsigned stack_frame_size = -1;
    unsigned new_stack_frame_size = -1;

    bool stackAlloc = false;
    bool stackDealloc = false;
    bool commitFunction = true;

    for (int j = 0; j < f->getInstructions().size(); ++j)
    {
      wahoo::Instruction *instr = f->getInstructions()[j];
      char buf[1024];
      sprintf(buf, "%s", instr->getAsm().c_str());
      fprintf(stderr, "# orig: 0x%08x %s\n", instr->getAddress(), buf);
     
      if (instr->isAllocSite())
      {
        regmatch_t pmatch[5];
        if(regexec(&m_stack_alloc_pattern, buf, 5, pmatch, 0)==0)
        {
          fprintf(stderr,"FOUND MATCH FOR STACK ALLOC PATTERN\n");
          char new_instr[2048];
          char matched[1024];
          if (pmatch[1].rm_so >= 0 && pmatch[1].rm_eo >= 0) 
          {
            int mlen = pmatch[1].rm_eo - pmatch[1].rm_so;
            strncpy(matched, &buf[pmatch[1].rm_so], mlen);
            matched[mlen] = '\0';

            sscanf(matched,"%x", &stack_frame_size);
            new_stack_frame_size = stack_frame_size + stack_frame_padding;
            sprintf(new_instr, "sub esp, 0x%x", new_stack_frame_size);
            addSimpleRewriteRule(f, buf, instr->getSize(), instr->getAddress(), new_instr);
            stackAlloc = true;
          }
        }
        else {
          fprintf(stderr,"WARNING: stack alloc but did not match pattern\n");
        }
      }
      else if (instr->isStackRef())
      {
        int k;
        regmatch_t pmatch[10];
        memset(pmatch, 0,sizeof(regmatch_t) * 10);
        if(regexec(&m_lea_hack_pattern, buf, 5, pmatch, 0)==0)
        {
          fprintf(stderr,"FOUND LEA HACK\n");
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
          fprintf(stderr,"FOUND MATCH FOR STACK EBP PATTERN\n");
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

          addSimpleRewriteRule(f, buf, instr->getSize(), instr->getAddress(), new_instr);
        }
        else if(regexec(&m_fancy_ebp_pattern, buf, 8, pmatch, 0)==0)
        {
          fprintf(stderr,"FOUND MATCH FOR FANCY EBP PATTERN\n");
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
              fprintf(stderr,"fancy %d: %s\n", k, matched);

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
          fprintf(stderr,"new_instr: %s\n", new_instr);
          addSimpleRewriteRule(f, buf, instr->getSize(), instr->getAddress(), new_instr);
        }
        else if(regexec(&m_stack_esp_pattern, buf, 5, pmatch, 0)==0)
        {
          fprintf(stderr,"FOUND MATCH FOR STACK ESP PATTERN\n");
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

          int sizeOutArgs = f->getOutArgsRegionSize();
          if (originalOffset >= sizeOutArgs)
            addSimpleRewriteRule(f, buf, instr->getSize(), instr->getAddress(), new_instr);
        }
        else if (strstr(buf, "ebp+")) // ignore ebp + anything
        {
        }
        else if (strstr(buf, "[esp]")) // ignore [esp]
        {
        }
        else
        {
          fprintf(stderr,"ERROR: should handle this case:\n");
          fprintf(stderr,"0x%08x: %s\n", instr->getAddress(), buf);
          commitFunction = false;
        }
      }
      else if (instr->isDeallocSite())
      {
        fprintf(stderr,"DEALLOC SITE\n");
        regmatch_t pmatch[5];
        if(regexec(&m_stack_dealloc_pattern, buf, 5, pmatch, 0)==0)
        {
          fprintf(stderr,"FOUND MATCH FOR STACK DEALLOC PATTERN\n");
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
            addSimpleRewriteRule(f, buf, instr->getSize(), instr->getAddress(), new_instr);
            stackDealloc = true;
          }
        }
        else if (strstr(buf, "leave"))
        {
          stackDealloc = true;
        }
      }
    }

    if (!stackAlloc)
    {
      fprintf(stderr,"Could not process function <%s>: no stack allocation routine found\n", f->getName().c_str());
    } 
    else if (!stackDealloc)
    {
      fprintf(stderr,"Could not process function <%s>: no stack deallocation routine found\n", f->getName().c_str());
    } 
    else if (!commitFunction)
    {
      fprintf(stderr,"Could not process function <%s>: missing rule for stack access\n", f->getName().c_str());
    }
    else {
      commitFn2SPRI(f, fp);
    }

}

// rewrite all candidate functions
void P1Transform::rewrite(char *p_filenamePrefix)
{
  int num_func_successful = 0;
  // only transform instructions contained in well-defined functions

  vector<wahoo::Function*> candidates = getCandidateFunctions();
  for (int i = 0; i < candidates.size(); ++i)
  {
    char filename[1024];
    sprintf(filename, "p1.%s.aspri", candidates[i]->getName().c_str());
    FILE* fp = fopen(filename,"w+");
    rewrite(candidates[i], fp);
    fclose(fp);
  }
}

// returns true if function is a candidate for the P1 transform
bool P1Transform::isCandidate(wahoo::Function *p_fn)
{
  if (!p_fn) return false;
  if (p_fn->isSafe()) return false;

  // strict policy: we only attempt the P1 transform on functions
  // that we think have exactly 1 allocation and deallocation site
  int numStackAllocSite = p_fn->getStackAllocationInstructions().size();
  int numStackDeallocSite = p_fn->getStackDeallocationInstructions().size();
  if (numStackAllocSite == 1 && numStackDeallocSite == 1)
    return true;

  return false;
}

// return candidate functions for the P1 transform
vector<wahoo::Function*> P1Transform::getCandidateFunctions()
{
  vector<wahoo::Function*> p1Candidates;
  // first get the candidate functions off the base class
  vector<wahoo::Function*> candidates = Rewriter::getCandidateFunctions();

  for (int i = 0; i < candidates.size(); ++i)
  {
    wahoo::Function* f = candidates[i];

    if (isCandidate(f))
      p1Candidates.push_back(f);
  }

  return p1Candidates;
}

// return non-candidate functions for the P1 transform
vector<wahoo::Function*> P1Transform::getNonCandidateFunctions()
{
  vector<wahoo::Function*> p1NonCandidates;
  // first get the candidate functions off the base class
  vector<wahoo::Function*> candidates = Rewriter::getCandidateFunctions();

  for (int i = 0; i < candidates.size(); ++i)
  {
    if (!isCandidate(candidates[i]))
      p1NonCandidates.push_back(candidates[i]);
  }

  return p1NonCandidates;
}

// return new stack frame size
// we currently allocate 2x the original space and make
// sure we pad by at least 8 bytes
int P1Transform::getStackFramePadding(wahoo::Function *p_fn)
{
  // @todo: add some random variation
  int stack_frame_padding = 2 * p_fn->getSize();
  if (stack_frame_padding < 8) stack_frame_padding = 8;

  return stack_frame_padding;
}
    
//
// badly rewrite 1 function
// the goals behind a bad transforms are:
//   - to make sure BED flags the fn
//   - to assess confidence in the fn (TSET)
//
void P1Transform::badRewrite(wahoo::Function *p_fn, FILE *p_aspri)
{
  addSimpleRewriteRule(p_fn, "", 1, p_fn->getAddress(), "hlt");
  commitFn2SPRI(p_fn, p_aspri);
}

void P1Transform::badRewrite(char *p_filenamePrefix)
{
  // get all candidate functions
  vector<wahoo::Function*> candidates = getCandidateFunctions();

  // transform them using a bad P1 transform
  // each bad transform is stored in a separate file
  for (int i = 0; i < candidates.size(); ++i)
  {
    char filename[1024];
    sprintf(filename, "p1.bad.%s.aspri", candidates[i]->getName().c_str());
    FILE* fp = fopen(filename,"w+");
    badRewrite(candidates[i], fp);
    fclose(fp);
  }
}
