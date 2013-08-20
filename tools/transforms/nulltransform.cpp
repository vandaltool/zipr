#include <iostream>
#include "null_transform.h"

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr << "usage: " << argv[0] << " <elfFile> <annotationFile> [<spriFile>]" << std::endl;
    return 1;
  }

  std::cout << "Reading elf file:" << argv[1] << std::endl;
  std::cout << "Reading MEDS annotation file:" << argv[2] << std::endl;

  NullTransform *nullTransform;

  if (argc == 3)
    nullTransform = new NullTransform(argv[1], argv[2], (char*)"spri.out");
  else
    nullTransform = new NullTransform(argv[1], argv[2], argv[3]);

  nullTransform->rewrite();

  vector<wahoo::Function*> ncf = nullTransform->getNonCandidateFunctions();
  vector<wahoo::Function*> cf = nullTransform->getCandidateFunctions();
  vector<wahoo::Function*> af = nullTransform->getAllFunctions();

  std::cout << "#functions: " << af.size() << std::endl;
  std::cout << "#candidate functions: " << cf.size() << std::endl;
  std::cout << "#non-candidate functions: " << ncf.size() << std::endl;
}
