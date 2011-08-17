#include "rewriter.h"

class NullTransform : public Rewriter
{
  public:
    NullTransform(char *p_elf, char *p_annot, char *p_spri) : Rewriter(p_elf, p_annot) {}

  public:
    void rewrite();
};
