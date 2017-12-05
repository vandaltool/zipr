#include <string>
#include <vector>
#include <string>

#include "targ-config.h"
#include "instruction_descriptor.h"

//class wahoo::Instruction;

using namespace std;

namespace wahoo {

class Function
{
  public:
    Function();
    Function(app_iaddr_t);
    Function(string, app_iaddr_t, int);
    ~Function();

    string            getName() const { return m_name; }
    void              setName(const string p_name) { m_name = p_name; }
    app_iaddr_t       getAddress() const { return m_address; }
    void              setAddress(const app_iaddr_t p_address) { m_address = p_address; }
    int               getSize() const { return m_size; }
    void              setSize(const int p_size) { m_size = p_size; }
    int               getFrameSize() const { return m_frameSize; }
    void              setFrameSize(const int p_size) { m_frameSize = p_size; }
    int               getFunctionID() const { return m_functionID; }
    void              setFunctionID(const int id) { m_functionID = id; }

	
    bool operator == (const Function &);
    bool operator == (const app_iaddr_t);
    bool operator != (const Function &);
    bool operator != (const app_iaddr_t);

    bool isSafe() const { return m_isSafe; }
    void setSafe() { m_isSafe = true; }
    void setUnsafe() { m_isSafe = false; }

    void setOutArgsRegionSize(const int p_size) { m_outArgsRegionSize = p_size; }
    int getOutArgsRegionSize() const { return m_outArgsRegionSize; }

    void setUseFramePointer(const bool p_useFP) { m_useFP = p_useFP; }
    bool getUseFramePointer() const { return m_useFP; }

    void addInstruction(wahoo::Instruction *);
    void addStackAllocationInstruction(wahoo::Instruction *);
    void addStackDeallocationInstruction(wahoo::Instruction *);
    void addStackReferenceInstruction(wahoo::Instruction *);
    void addRewriteRule(string p_rule) { m_rewrites.push_back(p_rule); }

    vector<wahoo::Instruction*> getInstructions();
    vector<wahoo::Instruction*> getStackAllocationInstructions();
    vector<wahoo::Instruction*> getStackDeallocationInstructions();
    vector<wahoo::Instruction*> getStackReferencesInstructions();

    vector<string> getRewrites() { return m_rewrites; }

    double getInstructionCoverage();
    double getInstructionCoverage(int*, int*);

  private:
    void _init();

    int            m_functionID;
    string         m_name;
    app_iaddr_t    m_address;
    int            m_size;
    int            m_frameSize;
    bool           m_isSafe;
    bool           m_useFP;
    int            m_outArgsRegionSize;

    vector<wahoo::Instruction*> m_allInstructions;
    vector<wahoo::Instruction*> m_stackRefsInstructions;
    vector<wahoo::Instruction*> m_stackAllocInstructions;
    vector<wahoo::Instruction*> m_stackDeallocInstructions;

    vector<string>              m_rewrites; // keep track of rewrite rules
};

}
