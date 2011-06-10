#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>

#include "p1_transform.h"


void goodTransform(char *progName, char *annot)
{
  P1Transform *p1GoodTransform = new P1Transform(progName, annot, "spri.out");

  p1GoodTransform->rewrite(basename(progName));

  // produce candidate & non-candidate files as well
  
  // output good candidate functions
  FILE *fp = fopen("p1.candidates","w+");
  vector<wahoo::Function*> candidateFunctions = p1GoodTransform->getCandidateFunctions();
  for (int i = 0; i < candidateFunctions.size(); ++i)
  {
    fprintf(fp,"%s\n", candidateFunctions[i]->getName().c_str());
  }
  fclose(fp);

  // output bad candidate functions
  FILE *fp2 = fopen("p1.non_candidates","w+");
  vector<wahoo::Function*> nonCandidateFunctions = p1GoodTransform->getNonCandidateFunctions();
  for (int i = 0; i < nonCandidateFunctions.size(); ++i)
  {
    fprintf(fp2,"%s\n", nonCandidateFunctions[i]->getName().c_str());
  }
  fclose(fp2);
}

void badTransform(char *progName, char *annot)
{
  P1Transform *p1BadTransform = new P1Transform(progName, annot, "spri.out.foobar");
  p1BadTransform->badRewrite(basename(progName));
}

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr << "usage: " << argv[0] << " <elfFile> <annotationFile>" << std::endl;
    return 1;
  }

  goodTransform(argv[1], argv[2]);
//  badTransform(argv[1], argv[2]); 
}

