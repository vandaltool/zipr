#include <iostream>
#include <map>
#include "rewriter.h"

using namespace std;


int main(int argc, char **argv)
{
  if (argc < 5)
  {
    cerr << "usage: " << argv[0] << " <elfFile> <annotationFile> <coverageFile> <outputFile>" << endl;
    return 1;
  }

  cout << "elf file:" << argv[1] << endl;
  cout << "MEDS annotation file:" << argv[2] << endl;
  cout << "coverage file:" << argv[3] << endl;
  cout << "output file:" << argv[4] << endl;

  Rewriter *rewriter = new Rewriter(argv[1], argv[2]);

  map<wahoo::Function*, double> coverage = rewriter->getFunctionCoverage(argv[3]);
  if (coverage.empty())
  {
    cerr << "Warning: no functions found in: " << argv[0];
    return 1;
  }

  FILE *fp = fopen(argv[4],"w+"); // output file
  if (!fp) 
  {
    cerr << "Error opening output file: " << argv[4] << endl;
    return 1;
  }

  for (map<wahoo::Function*,double>::iterator it = coverage.begin(); it != coverage.end(); ++it)
  {
    int count, total;
    wahoo::Function *f = it->first;
    if (!f) continue;
    double coverage = f->getInstructionCoverage(&count, &total);
    fprintf(fp, "%s %f %d %d\n", f->getName().c_str(), coverage, count, total);
  }

  fclose(fp);

}

