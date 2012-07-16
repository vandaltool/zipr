

#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>

using namespace libIRDB;
using namespace std;

void do_ilr(VariantID_t *pidp, FileIR_t* firp)
{

	assert(firp && pidp);

	cout<<"Applying ILR to variant "<<*pidp<< "." <<endl;

	long long unmoved_instr=0, moved_instr=0;

	set<AddressID_t*> newaddressset;
	for(
		set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
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

	firp->GetAddresses()=newaddressset;


	cout << "# ATTRIBUTE filename="<<firp->GetFile()->GetURL()<<endl;
	cout << "# ATTRIBUTE unmoved_instructions="<<std::dec<<unmoved_instr<<endl;
	cout << "# ATTRIBUTE moved_instructions="<<std::dec<<moved_instr<<endl;
	cout << "# ATTRIBUTE moved_ratio="<<std::dec<<(float)moved_instr/(moved_instr+unmoved_instr)<<endl;

	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	firp->WriteToDB();
}
main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: ilr <id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));
		assert(pidp->IsRegistered()==true);


                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			// read the db  
			firp=new FileIR_t(*pidp,this_file);
			
			// do the ILRing. 
			do_ilr(pidp, firp);

			delete firp;
		}


		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	cout<<"Done!"<<endl;

	delete pidp;
}

