#include <stdio.h>
#include <stdlib.h>

#include "a2bspri.h"

/*
*   usage: a2bspri <input assembly spri file> <output binary spri file>
*/

void usage()
{
  fprintf(stderr,"usage: a2bspri <assembly SPRI file> <name of output binary SPRI file>\n");
  fprintf(stderr,"   a2bspri takes as input an assembly SPRI file and outputs a binary SPRI file\n");
  exit(1);
}

int main(int argc, char **argv)
{
  FILE *aspri = NULL;
  FILE *bspri = NULL;

  if (argc != 3)
  {
    usage();
  }

  aspri = fopen(argv[1], "r");
  if (!aspri)
  {
    fprintf(stderr,"Error opening assembly SPRI input file: %s\n", argv[1]);
    exit(1);
  }

  bspri = fopen(argv[2], "w+");
  if (!bspri)
  {
    fprintf(stderr,"Error creating binary SPRI output file: %s\n", argv[2]);
    exit(1);
  }

  a2bspri(aspri, bspri);

  fclose(aspri);
  fclose(bspri);
}
