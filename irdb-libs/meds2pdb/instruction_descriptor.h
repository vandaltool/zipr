#ifndef _instruction_h_
#define _instruction_h_

#include <string>
#include <set>
#include <irdb-core>

#include <irdb-core>

using namespace std;


namespace wahoo {

class Function;


class Instruction {
  public:
    Instruction();
    Instruction(IRDB_SDK::VirtualOffset_t, int p_size = -1, Function* = NULL);
    ~Instruction();

    void setSize(int p_size) { m_size = p_size; }
    void setFunction(Function *p_func) { m_function = p_func; }

    void markAllocSite();
    void markDeallocSite();
    void markStackRef();
    void markVarStackRef();

    IRDB_SDK::VirtualOffset_t     getAddress() const { return m_address; }
    int             getSize() const { return m_size; }
    Function*       getFunction() const { return m_function; }
    string          getAsm() const { return m_asm; }
    void            setAsm(string p_str) { m_asm = p_str; }
    void            setData(void *dataPtr, int len);
    unsigned char*  getData() const { return m_data; }
    void            setData(void *data) { m_data = (unsigned char*) data; }

    bool isStackRef() const { return m_stackRef; }
    bool isVarStackRef() const { return m_varStackRef; }
    bool isAllocSite() const { return m_allocSite; }
    bool isDeallocSite() const { return m_deallocSite; }

    // keep track of whether instruction has been visited during execution
    void setVisited() { m_isVisited = true; }
    bool isVisited() const { return m_isVisited; }

  private:
    IRDB_SDK::VirtualOffset_t     m_address;
    int             m_size;
    Function*       m_function;
    string          m_asm;
    unsigned char*  m_data;

    bool            m_allocSite;
    bool            m_deallocSite;
    bool            m_stackRef;
    bool            m_varStackRef;

    bool            m_isVisited;

};

}

#endif
