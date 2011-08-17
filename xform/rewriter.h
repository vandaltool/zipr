#include <map>
#include "elfreader.h"
#include "function_descriptor.h"

using namespace std;

class Rewriter
{
  public:
    Rewriter(char *p_elf, char *p_annotationFile);
    ~Rewriter();

    virtual vector<wahoo::Function*> getAllFunctions();
    virtual vector<wahoo::Function*> getCandidateFunctions();
    virtual vector<wahoo::Function*> getNonCandidateFunctions();

    virtual vector<wahoo::Instruction*> getAllInstructions();

    map<wahoo::Function*, double> getFunctionCoverage(char *p_instFile); 

  protected:
    void readAnnotationFile(char []);
    void readElfFile(char []);
    ElfReader *getElfReader() { return m_elfReader; }
    FILE* getAsmSpri() { return m_spri; };
    void setAsmSpri(FILE *p_spri) { m_spri = p_spri; };

    // dissassemble all functions
    void dissassemble();

    // one instruction modification
    void addSimpleRewriteRule(wahoo::Function* p_func, char *p_origInstr, int p_origSize, app_iaddr_t p_origAddress, char *p_newInstr);

    // commit function to AsmSPRI file
    void commitFn2SPRI(wahoo::Function* p_func, FILE *p_file);

  protected:
    map<app_iaddr_t, wahoo::Function*> m_functions;
    map<app_iaddr_t, wahoo::Instruction*> m_instructions;

  private:
    ElfReader*    m_elfReader;   
    FILE*         m_spri; 
};
