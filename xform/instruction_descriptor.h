#ifndef _instruction_h_
#define _instruction_h_

#include <string>

#include "config.h"

using namespace std;


namespace wahoo {

class Function;

class Instruction {
  public:
    Instruction();
    Instruction(app_iaddr_t, int, Function* = NULL);
    ~Instruction();

    void setSize(int p_size) { m_size = p_size; }
    void setFunction(Function *p_func) { m_function = p_func; }

    void markAllocSite();
    void markDeallocSite();
    void markStackRef();
    void markVarStackRef();

    app_iaddr_t     getAddress() { return m_address; }
    int             getSize() { return m_size; }
    Function*       getFunction() { return m_function; }
    string          getAsm() { return m_asm; }
    void            setAsm(string p_str) { m_asm = p_str; }
    void            setData(void *dataPtr, int len);
    unsigned char*  getData() { return m_data; }
    void            setData(void *data) { m_data = (unsigned char*) data; }

    bool isStackRef() { return m_stackRef; }
    bool isVarStackRef() { return m_varStackRef; }
    bool isAllocSite() { return m_allocSite; }
    bool isDeallocSite() { return m_deallocSite; }

    // keep track of whether instruction has been visited during execution
    void setVisited() { m_isVisited = true; }
    bool isVisited() { return m_isVisited; }

  private:
    app_iaddr_t     m_address;
    int             m_size;
    Function*       m_function;
    string          m_asm;
//    unsigned char m_data[128];
    unsigned char*  m_data;

    bool            m_allocSite;
    bool            m_deallocSite;
    bool            m_stackRef;
    bool            m_varStackRef;

    bool            m_isVisited;
};

}

#endif
