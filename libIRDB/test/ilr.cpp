

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: ilr <id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	VariantIR_t *virp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		// read the db  
		virp=new VariantIR_t(*pidp);


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);

	cout<<"Applying ILR to variant "<<*pidp<< "." <<endl;

	long long unmoved_instr=0, moved_instr=0;

	set<AddressID_t*> newaddressset;
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
		AddressID_t *newaddr=new AddressID_t;
		newaddr->SetFileID(insn->GetAddress()->GetFileID());
		insn->SetAddress(newaddr);

		if (insn->GetIndirectBranchTargetAddress())
		{
			unmoved_instr++;
			newaddressset.insert(insn->GetIndirectBranchTargetAddress());
		}
		else
			moved_instr++;

		newaddressset.insert(newaddr);
	}

	virp->GetAddresses()=newaddressset;


	cout << "# ATTRIBUTE unmoved_instructions="<<std::dec<<unmoved_instr<<endl;
	cout << "# ATTRIBUTE moved_instructions="<<std::dec<<moved_instr<<endl;
	cout << "# ATTRIBUTE moved_ratio="<<std::dec<<(float)moved_instr/(moved_instr+unmoved_instr)<<endl;

	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	virp->WriteToDB();

	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete pidp;
	delete virp;
}
