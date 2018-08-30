#include "hook_start.hpp"

#include "Rewrite_Utility.hpp"
#include <assert.h>
#include <stdexcept>

using namespace libTransform;
using namespace ELFIO;
using namespace libIRDB;
using namespace IRDBUtility;

HookStart::HookStart(FileIR_t *p_variantIR) :
	Transform(NULL, p_variantIR, NULL),
	m_callback_name("zipr_hook_start")
{
}

HookStart::~HookStart() 
{
}

void HookStart::LoadElf()
{
	unsigned int elfoid=0;
	pqxxDB_t *interface=NULL;

	if (m_elfiop)
		return;

	elfoid = getFileIR()->GetFile()->GetELFOID();
	interface = dynamic_cast<pqxxDB_t*>(BaseObj_t::GetInterface());
		
	assert(interface);

	m_file_object.reset(
		new pqxx::largeobjectaccess(interface->GetTransaction(),
		                            elfoid,
		                            std::ios::in));
	
	m_file_object->to_file("tmp.exe");

	m_elfiop.reset(new ELFIO::elfio);
	m_elfiop->load("tmp.exe");
}

Instruction_t *HookStart::add_instrumentation(Instruction_t *site)
{
	Relocation_t *zipr_reloc = new Relocation_t;
	//FileIR_t *firp = getFileIR();
	virtual_offset_t postCallbackReturn = getAvailableAddress();
	char pushRetBuf[100],
	     movIdBuf[100],
	     movRaxBuf[100],
	     movRspBuf[100];
	sprintf(pushRetBuf,"push qword 0x%lx", postCallbackReturn);
	sprintf(movIdBuf,"mov rdi, 0x0");
	sprintf(movRaxBuf,"mov rsi, rax");
	sprintf(movRspBuf,"mov rdx, rsp");

	cout << "postCallbackReturn: " << std::hex << postCallbackReturn << endl;

	zipr_reloc->SetType("zipr_value");

	Instruction_t *tmp=site,
	              *callback=NULL,
	              *post_callback=NULL;

	site=insertAssemblyBefore(tmp,"push rsp");
	tmp=insertAssemblyAfter(tmp,"push rbp");
	tmp=insertAssemblyAfter(tmp,"push rdi");
	tmp=insertAssemblyAfter(tmp,"push rsi");
	tmp=insertAssemblyAfter(tmp,"push rdx");
	tmp=insertAssemblyAfter(tmp,"push rcx");
	tmp=insertAssemblyAfter(tmp,"push rbx");
	tmp=insertAssemblyAfter(tmp,"push rax");
	tmp=insertAssemblyAfter(tmp,"push r8");
	tmp=insertAssemblyAfter(tmp,"push r9");
	tmp=insertAssemblyAfter(tmp,"push r10");
	tmp=insertAssemblyAfter(tmp,"push r11");
	tmp=insertAssemblyAfter(tmp,"push r12");
	tmp=insertAssemblyAfter(tmp,"push r13");
	tmp=insertAssemblyAfter(tmp,"push r14");
	tmp=insertAssemblyAfter(tmp,"push r15");
	tmp=insertAssemblyAfter(tmp,"pushf");
	tmp=insertAssemblyAfter(tmp,movIdBuf);
	/*
	 * Let's put a relocation on here!
	 */
	tmp->GetRelocations().insert(zipr_reloc);
	tmp=insertAssemblyAfter(tmp,movRaxBuf);
	tmp=insertAssemblyAfter(tmp,movRspBuf);
	/*
	 * The "bogus" return address that we push here
	 * will be popped by the callback handler 
	 * invocation code in zipr.
	 */
	tmp=insertAssemblyAfter(tmp,pushRetBuf);	 // push <ret addr>

	callback=tmp=insertAssemblyAfter(tmp,"call 0");
	callback->SetTarget(callback);
	callback->SetCallback(m_callback_name);

	post_callback=tmp=insertAssemblyAfter(tmp,"popf");
	post_callback->GetAddress()->SetVirtualOffset(postCallbackReturn);

	tmp=insertAssemblyAfter(tmp,"pop r15");
	tmp=insertAssemblyAfter(tmp,"pop r14");
	tmp=insertAssemblyAfter(tmp,"pop r13");
	tmp=insertAssemblyAfter(tmp,"pop r12");
	tmp=insertAssemblyAfter(tmp,"pop r11");
	tmp=insertAssemblyAfter(tmp,"pop r10");
	tmp=insertAssemblyAfter(tmp,"pop r9");
	tmp=insertAssemblyAfter(tmp,"pop r8");
	tmp=insertAssemblyAfter(tmp,"pop rax");
	tmp=insertAssemblyAfter(tmp,"pop rbx");
	tmp=insertAssemblyAfter(tmp,"pop rcx");
	tmp=insertAssemblyAfter(tmp,"pop rdx");
	tmp=insertAssemblyAfter(tmp,"pop rsi");
	tmp=insertAssemblyAfter(tmp,"pop rdi");
	tmp=insertAssemblyAfter(tmp,"pop rbp");
	tmp=insertAssemblyAfter(tmp,"lea rsp, [rsp+8]");

	tmp->SetFallthrough(site);

	return tmp;
}

int HookStart::execute()
{
	LoadElf();
	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;
		for(
		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  it!=func->GetInstructions().end();
		  ++it)
		{
			Instruction_t *insn = *it;
			if (insn->GetAddress() && 
			    insn->GetAddress()->GetVirtualOffset()==m_elfiop->get_entry())
			{
				add_instrumentation(insn);
			}
		}
	}
	return true;
}
