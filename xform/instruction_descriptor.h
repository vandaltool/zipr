#ifndef _instruction_h_
#define _instruction_h_

#include <string>
#include <set>

#include "targ-config.h"

using namespace std;


namespace wahoo {

class Function;

enum IBProvenance { IB_PROVENANCE_UNKNOWN, IB_PROVENANCE_RETURN, IB_PROVENANCE_SWITCH_TABLE, IB_PROVENANCE_INDIRECT_CALL, IB_PROVENANCE_COMPUTED_GOTO };

class Instruction {
  public:
    Instruction();
    Instruction(app_iaddr_t, int p_size = -1, Function* = NULL);
    ~Instruction();

    void setSize(int p_size) { m_size = p_size; }
    void setFunction(Function *p_func) { m_function = p_func; }

    void markAllocSite();
    void markDeallocSite();
    void markStackRef();
    void markVarStackRef();

    app_iaddr_t     getAddress() const { return m_address; }
    app_iaddr_t     getIBTAddress() const { return m_ibt_address; }
    int             getSize() const { return m_size; }
    Function*       getFunction() const { return m_function; }
    string          getAsm() const { return m_asm; }
    void            setAsm(string p_str) { m_asm = p_str; }
    void            setData(void *dataPtr, int len);
    unsigned char*  getData() const { return m_data; }
    void            setData(void *data) { m_data = (unsigned char*) data; }
    void     	    setIBTAddress(app_iaddr_t v) { m_ibt_address=v; }

    bool isStackRef() const { return m_stackRef; }
    bool isVarStackRef() const { return m_varStackRef; }
    bool isAllocSite() const { return m_allocSite; }
    bool isDeallocSite() const { return m_deallocSite; }

    // keep track of whether instruction has been visited during execution
    void setVisited() { m_isVisited = true; }
    bool isVisited() const { return m_isVisited; }

    // include Indirect branch targets for insructions.
    void addIBT(Instruction* insn) { ibts.insert(insn); }
    const std::set<Instruction*>&  getIBTs() { return ibts; }
    void markIbComplete(bool complete=true) { m_ib_complete=complete; }
    bool isIbComplete() { return m_ib_complete; }
    void setIbProvenance(char *);
    void setIbProvenance(const IBProvenance p_provenance) { m_ib_provenance = p_provenance; }
    IBProvenance getIbProvenance() const { return m_ib_provenance; }

  private:
    app_iaddr_t     m_address;
    app_iaddr_t     m_ibt_address;
    int             m_size;
    Function*       m_function;
    string          m_asm;
    unsigned char*  m_data;

    bool            m_allocSite;
    bool            m_deallocSite;
    bool            m_stackRef;
    bool            m_varStackRef;

    bool            m_isVisited;

    std::set<Instruction*> ibts;
    bool m_ib_complete;
    IBProvenance m_ib_provenance;
};

}

#endif
