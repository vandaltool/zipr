#include "Xeon_relocs.hpp"

using namespace libIRDB;
using namespace std;
using namespace Zipr_SDK;
using namespace ELFIO;
using namespace IRDBUtility;


// use this to determine whether a scoop has a given name.
static struct ScoopFinder : binary_function<const DataScoop_t*,const string,bool>
{
        // declare a simple scoop finder function that finds scoops by name
        bool operator()(const DataScoop_t* scoop, const string& name) const
        {
                return (scoop->GetName() == name);
        };
} scoop_finder;


static DataScoop_t* find_scoop(FileIR_t *firp,const string &name)
{
        auto it=find_if(firp->GetDataScoops().begin(), firp->GetDataScoops().end(), bind2nd(scoop_finder, name)) ;
        if( it != firp->GetDataScoops().end() )
                return *it;
        return NULL;
};


static void extract_fields(const string &reloc_string, int &sz, int &pos, string &val)
{
	sz=pos=0;
        val="";
        
	size_t loc=reloc_string.find('=');
	string nonce_value=reloc_string.substr(loc+1,reloc_string.size()-loc-1);
        
	// trying to parse a triple of form (a,b,c) into strings a,b,c
	// cfi_nonce=(pos=2,nv=0,sz=1)

	// find first comma
	size_t comma_pos=nonce_value.find(',');
	
        // parse out A
	string first=nonce_value.substr(5,comma_pos-1-4); // 1== strlen(","), 5==strlen("(pos=")

	//  parse out "b,c)"
	string rest=nonce_value.substr(comma_pos+1, string::npos); 	// 1 == strlen(",")

	// find next comma
	comma_pos=rest.find(',');

	// parse out B
	string second=rest.substr(3,comma_pos-3); // 3=strlen("nv=")

	// parse out "c)"
	rest=rest.substr(comma_pos+1, string::npos);	// 1==sizeof(",")

	// find closing paren.
	size_t paren_pos=rest.find(')');

	// pares out C
	string third=rest.substr(3,paren_pos-3); // 3==strlen("sz=")


	pos=(int)strtol(first.c_str(),0,10);	
	val=second;
	sz=(int)strtol(third.c_str(),0,10);
}

static string GetNonceValue(Relocation_t& reloc)
{
	int sz, pos;
	string val;
	extract_fields(reloc.GetType(), sz, pos, val);
	return val;
}

static int GetNonceSize(Relocation_t& reloc)
{
	int sz, pos;
	string val;
	extract_fields(reloc.GetType(), sz, pos, val);
	return sz;
}

static int GetNonceOffset(Relocation_t& reloc)
{
	int sz, pos;
	string val;
	extract_fields(reloc.GetType(), sz, pos, val);
	return pos;
}


// TODO: BAD COUPLING, SHOULD PASS REG WITH RELOC
void Xeon_relocs::AddDynLibCheckCode()
{
    // Decide what register to use
    string reg="ecx";	// 32-bit reg 
    if(firp->GetArchitectureBitWidth()==64)
	reg="r11";	// 64-bit reg.
    
    for( auto it=firp->GetInstructions().begin(); it!=firp->GetInstructions().end(); ++it)
    {
        const auto insn=*it;
        if(FindRelocation(insn, "insert_dynlib_check"))
        {
            Instruction_t* old_target = insn->GetTarget();
            insn->SetTarget(GetDynLibCheckCode(reg, old_target));
        }	
    }
    firp->AssembleRegistry();	// resolve all assembly into actual bits.
    firp->SetBaseIDS();		// assign a unique ID to each insn.
}


void Xeon_relocs::UpdateAddrRanges()
{
    RangeAddress_t  min_addr=m_memory_space->GetMinPlopped();
    RangeAddress_t  max_addr=m_memory_space->GetMaxPlopped();

    int offset=0;	// deal with REX prefix for 64-bit mode.
    if(firp->GetArchitectureBitWidth()==64)
	offset=1;

    InstructionSet_t::iterator it;
    for(it=min_addr_update.begin(); it!=min_addr_update.end(); ++it)
    {
	Instruction_t& insn=*(*it);
	RangeAddress_t insn_addr=(*final_insn_locations)[&insn];
	if(insn_addr)
	{
            if(m_elfio->get_type() == ET_EXEC)
            {
                cout<<"Updating min_addr for cmp at "<<hex<<insn_addr<<" to compare to "<<min_addr<<endl;
		m_memory_space->PlopBytes(insn_addr+2+offset,(const char*)&min_addr,4); // cmp insn has a 4 byte immed.
            }
            else if(m_elfio->get_type() == ET_DYN)
            {
            	// pcrel addressing is always trick.
		// effective address == instruction address + instruction size + offset
		// so we need offset=effective_addr-instruction_address-insn_size;
		RangeAddress_t offset=min_addr-insn_addr-insn.GetDataBits().size();
		// lea trick doesn't work on 32-bit.
		assert(firp->GetArchitectureBitWidth()==64);
		cout<<"Updating min_addr for "<<hex<<insn.GetBaseID()<<":lea at "<<hex<<insn_addr<<" to compare to "<<min_addr<<", offset="<<offset<<endl;
		m_memory_space->PlopBytes(insn_addr+3,(const char*)&offset,4); // cmp insn has a 4 byte immed.
            }
            else
		assert(0);
        }
        else
        {
            cout<<"No addr for  min_addr at "<<hex<<insn_addr<<" to compare to "<<min_addr<<endl;
	}
    }

    for(it=max_addr_update.begin(); it!=max_addr_update.end(); ++it)
    {
	Instruction_t& insn=*(*it);
	RangeAddress_t insn_addr=(*final_insn_locations)[&insn];
	assert(insn_addr);
	if(insn_addr)
	{
            if(m_elfio->get_type() == ET_EXEC)
            {
            	cout<<"Updating max_addr at "<<hex<<insn_addr<<" to compare to "<<max_addr<<endl;
		m_memory_space->PlopBytes(insn_addr+2+offset,(const char*)&max_addr,4);	// cmp insn with a 4-byte immed
            }
            else if(m_elfio->get_type() == ET_DYN)
            {
		// pcrel addressing is always trick.
		// effective address == instruction address + instruction size + offset
		// so we need offset=effective_addr-instruction_address-insn_size;
		RangeAddress_t offset=max_addr-insn_addr-insn.GetDataBits().size();
		// lea trick doesn't work on 32-bit.
		assert(firp->GetArchitectureBitWidth()==64);
		cout<<"Updating max_addr for "<<hex<<insn.GetBaseID()<<":lea at "<<hex<<insn_addr<<" to compare to "<<max_addr<<", offset="<<offset<<endl;
		m_memory_space->PlopBytes(insn_addr+3,(const char*)&offset,4); // cmp insn has a 4 byte immed.
            }
            else
		assert(0);
	}
	else
	{
            cout<<"No addr for  max_addr at "<<hex<<insn_addr<<" to compare to "<<max_addr<<endl;
	}
    }
}


ZiprOptionsNamespace_t *Xeon_relocs::RegisterOptions(Zipr_SDK::ZiprOptionsNamespace_t *global) 
{
	global->AddOption(&m_verbose);
	return NULL;
}


bool Xeon_relocs::WillPluginPlop(libIRDB::Instruction_t* insn)
{
	if(!m_on)
            return false;
        
	bool will_plop = false;
        
	if(FindExeNonceRelocation(insn))
            will_plop = true;

	if (m_verbose)
	{
	
		cout << "Xeon_relocs::WillPluginPlop:WillPlop=" <<std::boolalpha <<will_plop<<" for "
		     << hex << insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;
	}
	return will_plop;
}


size_t Xeon_relocs::DollopEntryOpeningSize(DollopEntry_t* entry)
{
	return 0;
}


size_t Xeon_relocs::DollopEntryClosingSize(DollopEntry_t* entry)
{

	if(!m_on)
		return 0;

	Instruction_t* insn=entry->Instruction();	
        int exeNonceSize = 0;
	for(
                RelocationSet_t::iterator rit=insn->GetRelocations().begin();
                rit!=insn->GetRelocations().end();
                rit++
	   )
        {
            Relocation_t& reloc=*(*rit);
            if(IsExeNonceRelocation(reloc))
            {
		exeNonceSize += GetNonceSize(reloc);		
            }
        }
	
	return exeNonceSize;
}


RangeAddress_t Xeon_relocs::PlopDollopEntry(Zipr_SDK::DollopEntry_t *de,
	RangeAddress_t & placed_addr,
	RangeAddress_t & target_addr,
	size_t instruction_size,
	bool &instruction_placed)
{
	
	//placed_addr=de->Place();
	RangeAddress_t ret=placed_addr; // relocs currently account for instruction size in offset
	// TODO: Need to update ret to =instruction size + placed addr for calls if plopping on calls
	Instruction_t* insn=de->Instruction();

	if (m_verbose)
	{
		cout << "Xeon_relocs::Plopping Instruction  : " << hex 
		     << de->Instruction()->GetBaseID() <<":"<<de->Instruction()->getDisassembly()<<endl;
		cout << "placed_address  : " << std::hex << placed_addr << endl;
		cout << "de->Place()     : " << std::hex << de->Place() << endl;
		cout << "instruction_size: " << std::dec << instruction_size << endl;
	}

	cout<<"Xeon_relocs::plopping "<<de->Instruction()->getDisassembly()<<" at "<<hex<<placed_addr<<endl;
        
        RangeAddress_t baseOffset = ret;
        for(
                RelocationSet_t::iterator rit=insn->GetRelocations().begin();
                rit!=insn->GetRelocations().end();
                rit++
	   )
        {
            Relocation_t& reloc=*(*rit);
            if(IsExeNonceRelocation(reloc))
            {
		int offset=GetNonceOffset(reloc);
                int size=GetNonceSize(reloc);
		if(offset+size+baseOffset > ret)
                {
                    ret = offset+size+baseOffset;
                }
                
                string value=GetNonceValue(reloc);
                
                cout<<"Xeon_relocs::Placing nonce:"<<std::hex<<value<<" at "<<hex<<ret<<endl;
                // Places in big endian order (it assumes the reloc has value in little-endian where appropriate)
                for(int i = 0; i < size; ++i)
                {
		    (*m_memory_space)[baseOffset+offset]=value.at(i);
		    ++offset;
                }
            }
        }
        
	return ret;	
}


////////////////////////////////////////////////////////////////////////////////

// minor helpers

Relocation_t* Xeon_relocs::FindRelocation(Instruction_t* insn, string type)
{
	RelocationSet_t::iterator rit;
	for( rit=insn->GetRelocations().begin(); rit!=insn->GetRelocations().end(); ++rit)
	{
		Relocation_t& reloc=*(*rit);
		if(reloc.GetType()==type)
		{
			return &reloc;
		}
	}
	return NULL;
}


Relocation_t* Xeon_relocs::FindExeNonceRelocation(Instruction_t* insn)
{
        RelocationSet_t::iterator rit;
	for( rit=insn->GetRelocations().begin(); rit!=insn->GetRelocations().end(); ++rit)
	{
		Relocation_t* reloc=*rit;
                if(strstr(reloc->GetType().c_str(),"cfi_exe_nonce=")!=NULL)                    
                    return reloc;
	}
        return NULL;
}


bool Xeon_relocs::IsExeNonceRelocation(Relocation_t& reloc)
{
	if(strstr(reloc.GetType().c_str(),"cfi_exe_nonce=")==NULL)
		return false;
	return true;
}


////////////////////////////////////////////////////////////////////////////////

// main workers

Instruction_t* Xeon_relocs::GetDynLibCheckCode(string reg, Instruction_t* target)
{
	string rspreg="esp";	// 32-bit reg 
        if(firp->GetArchitectureBitWidth()==64)
	    rspreg="rsp";	// 64-bit reg.
	
	static Instruction_t* bounds_start=NULL;
	Instruction_t* bounds_end=NULL;
        
        if(bounds_start != NULL)
            return bounds_start;

	// we need a dispatch handler in case the range check fails
	// this range check should be working with multimodule support enabled.
	Instruction_t* out_of_range_handler=NULL;
	out_of_range_handler = GenerateOutOfRangeHandler();

	// before the individual checks on the slow path
	// we want to insert a range check.
	//	cmp ecx/r11, start_segement_addr
	//	jlt out_of_range_handler
	//	cmp ecx/r11, end_segment_addr
	//	jgt out_of_range_handler

	// statically loaded exe.
	if(m_elfio->get_type() == ET_EXEC)
	{
		Instruction_t* tmp=NULL;
		bounds_start =
		tmp = addNewAssembly(firp,NULL,"cmp "+reg+", 0x12345678");
		min_addr_update.insert(tmp);
		// cross library jump detected
		tmp = insertAssemblyAfter(firp,tmp,"jl 0",out_of_range_handler);        
		// cross library jump detected
		tmp = insertAssemblyAfter(firp,tmp,"cmp "+reg+", 0x87654321");
		max_addr_update.insert(tmp);
		tmp = insertAssemblyAfter(firp,tmp,"jg 0",out_of_range_handler);        // cross library jump detected
		bounds_end=tmp;
	}
	// dynamically loaded object (pie exec or pic)
	else if(m_elfio->get_type() == ET_DYN)
	{
		Instruction_t* tmp=NULL;
		Instruction_t* oor_pop=addNewAssembly(firp, NULL, "pop "+reg);		// SYS_exit
		oor_pop->SetFallthrough(out_of_range_handler);

		string leastring1("\x4c\x8d\x1d\x78\x56\x34\x12",7);
		string leastring2("\x4c\x8d\x1d\x12\x34\x56\x78",7);

		bounds_start=
		tmp = addNewAssembly(firp,NULL,"push "+reg);	// save target on stack
		tmp = insertDataBitsAfter(firp,tmp, leastring1);        	// load lower bound
                min_addr_update.insert(tmp);
		tmp = insertAssemblyAfter(firp,tmp,"cmp [rsp], "+reg);        	// cmp to lower bound to target.
		tmp = insertAssemblyAfter(firp,tmp,"jl 0", oor_pop);        		// cross library jump detected
		tmp = insertDataBitsAfter(firp,tmp, leastring2);       	// load upper bound
                max_addr_update.insert(tmp);
		tmp = insertAssemblyAfter(firp,tmp,"cmp [rsp], "+reg);        	// compare upper bound to target.
		tmp = insertAssemblyAfter(firp,tmp,"jg 0", oor_pop);        		// cross lib jmp detected.
		tmp = insertAssemblyAfter(firp,tmp,"pop "+reg);        		// restore ecx/r11.
		bounds_end=tmp;
	}
	else
		assert(0); // what?

	assert(bounds_start && bounds_end);
        Instruction_t* jmp = addNewAssembly(firp, NULL, "jmp 0");
        jmp->SetTarget(target);
	bounds_end->SetFallthrough(jmp);
	return bounds_start;
}


Instruction_t* Xeon_relocs::GenerateOutOfRangeHandler(void)
{
	string reg="ecx";
	if(firp->GetArchitectureBitWidth()==64)
		reg="r11";
	unsigned ptrsize=firp->GetArchitectureBitWidth()/8;

	// find some scoops we'll need
	DataScoop_t* dynstr_scoop=find_scoop(firp, ".dynstr");
	DataScoop_t* dispatch_scoop =find_scoop(firp, "zest_cfi_dispatch");

	// if we can't find the cfi multimodule support, don't do multimodule protection
	if(!dynstr_scoop || dynstr_scoop->GetContents().find("zestcfi") == string::npos)
	{
		Instruction_t *jmp=addNewAssembly(firp, NULL, "jmp "+reg);	// jmp ecx/r11 
		return jmp;
	}

	// if it was transformed with multimodule support, all these scoops better exist.
	assert(dispatch_scoop);


	// for now, just do the simple thing of calling zest_cfi_dispatch to check if the transfer is allowed.
	// if the function returns, then the transfer is OK, and we should cache it.
	// again, for now, just allow the xfer.
	if(ptrsize==8)
	{
		// call <pc-6> is "call 0" in pcrel-ness.  we'll hang a reloc off this to indicate 
		// which scoop it should really go to.		
		char jmp_neg6[]="\xff\x25\xfa\xff\xff\xff";	 
		string jmp_neg6_str(jmp_neg6,sizeof(jmp_neg6)-1);


		Instruction_t *dispatchjmp=NULL;

		dispatchjmp=addNewDatabits(firp, NULL, jmp_neg6_str);	// jmp zest_cfi_dispatch with r11==target

		Relocation_t* dispatchjmp_reloc=new Relocation_t(BaseObj_t::NOT_IN_DATABASE, 0, "pcrel", dispatch_scoop);
		firp->GetRelocations().insert(dispatchjmp_reloc);
		dispatchjmp->GetRelocations().insert(dispatchjmp_reloc);


		// need to update the calls to use pcrel addressing
		return dispatchjmp;
	}
	else if(ptrsize==4)
	{
		// fixme later.
		assert(0);
	}
	else
		assert(0);
}


extern "C"
Zipr_SDK::ZiprPluginInterface_t* GetPluginInterface(
        Zipr_SDK::Zipr_t* zipr_object)
{
        Zipr_SDK::MemorySpace_t *p_ms=zipr_object->GetMemorySpace();
        ELFIO::elfio *p_elfio=zipr_object->GetELFIO();
        libIRDB::FileIR_t *p_firp=zipr_object->GetFileIR();
        Zipr_SDK::InstructionLocationMap_t *p_fil=zipr_object->GetLocationMap();

	return new Xeon_relocs(p_ms,p_elfio,p_firp,p_fil,zipr_object);
}
