#include <regex.h>
#include "rewriter.h"

class P1Transform : public Rewriter
{
  public:
    P1Transform(char *p_elf, char *p_annot, char *p_spri);

    void rewrite(char *); 
    void rewrite(wahoo::Function*, FILE *); 

    void badRewrite(wahoo::Function*, FILE*);
    void badRewrite(char *);

    virtual vector<wahoo::Function*> getCandidateFunctions();
    virtual vector<wahoo::Function*> getNonCandidateFunctions();

  private:
    bool isCandidate(wahoo::Function*);
    int getStackFramePadding(wahoo::Function*);

  private:
    regex_t m_stack_ebp_pattern;
    regex_t m_stack_alloc_pattern;
    regex_t m_stack_dealloc_pattern;
    regex_t m_lea_hack_pattern;
    regex_t m_fancy_ebp_pattern;
    regex_t m_stack_esp_pattern;
    regex_t m_fancy_esp_pattern;
};

