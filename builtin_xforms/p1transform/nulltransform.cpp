/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
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

#include <iostream>
#include "targ-config.h"

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

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