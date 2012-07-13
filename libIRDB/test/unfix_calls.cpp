

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <iostream>
#include <stdlib.h>
#include "beaengine/BeaEngine.h"
#include <assert.h>
#include <string.h>



using namespace libIRDB;
using namespace std;

void unfix_call(Instruction_t* insn, FileIR_t *virp);
bool is_push(Instruction_t* insn);

void unfix_calls(FileIR_t* virp)
{
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{

		Instruction_t* insn=*it;
		unfix_call(insn, virp);
	}
}

void unfix_call(Instruction_t* insn, FileIR_t *virp)
{
	/* we need a push instruction */
	assert(insn);
	if(!is_push(insn))
		return;
	/* that has a fallthrough */
	Instruction_t* nextinsn=insn->GetFallthrough();
	if(!nextinsn)
		return;

	/* to a jump */
	Instruction_t* target=nextinsn->GetTarget();
	if(!target)
		return;

	/* that isn't conditional */
	Instruction_t* fallthrough=nextinsn->GetFallthrough();
	if(fallthrough)
		return;

	/* and the target has a function */
	Function_t *target_func=target->GetFunction();
	if(!target_func)
		return;

	/* more checks?  that the value pushed is an original program address?  and one that originally follows the call? */

	/* it seems that this instruction has met its prereqs. for converting to a call*/ 
	ControlFlowGraph_t *target_cfg=new ControlFlowGraph_t(target_func);

	cout<<"Built CFG for func "<< target_func->GetName() << endl;

	cout<<*target_cfg<<endl;

	/* cleanup */
	delete target_cfg;

}


//
// return true if insn is a call
//
bool is_push(Instruction_t* insn)
{
	return insn->GetDataBits()[0]==0x68;
}


//
// main rountine; convert calls into push/jump statements 
//
main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: ilr <id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t *virp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		// read the db  
		virp=new FileIR_t(*pidp);


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);

	cout<<"Adjusting push/jmp -> call in variant "<<*pidp<< "." <<endl;
	
	unfix_calls(virp);

	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	virp->WriteToDB();

	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete virp;
	delete pidp;
}
