#include <libIRDB-core.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <elf.h>
#include <algorithm>

#include <exeio.h>
#include "beaengine/BeaEngine.h"
#include "dwarf2.h"



using namespace std;
using namespace EXEIO;
using namespace libIRDB;

template <int ptrsize>
class eh_frame_util_t
{
	public: 
	template <class T> 
	static bool read_type(T &value, unsigned int &position, const uint8_t* const data, const int max)
	{
		if(position + sizeof(T) > max) return true;

		
		// typecast to the right type
		auto ptr=(const T*)&data[position];

		// set output parameters
		position+=sizeof(T);
		value=*ptr;

		return false;
		
	}
	template <class T> 
	static bool read_type_with_encoding
		(const uint8_t encoding, T &value, 
		unsigned int &position, 
		const uint8_t* const data, 
		const int max, 
		const uint64_t section_start_addr )
	{
		auto orig_position=position;
		auto encoding_lower8=encoding&0xf;
		auto encoding_upper8=encoding&0xf0;
		value=0;
		switch(encoding_lower8)
		{
			case DW_EH_PE_omit  :
				return true;


			case DW_EH_PE_uleb128:
			{
				auto newval=uint64_t(0);
				if(eh_frame_util_t<ptrsize>::read_uleb128(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_sleb128:
			{
				auto newval=int64_t(0);
				if(eh_frame_util_t<ptrsize>::read_sleb128(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_udata2 :
			{
				auto newval=uint16_t(0);
				if(eh_frame_util_t<ptrsize>::read_type(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_udata4 :
			{
				auto newval=uint32_t(0);
				if(eh_frame_util_t<ptrsize>::read_type(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_udata8 :
			{
				auto newval=uint64_t(0);
				if(eh_frame_util_t<ptrsize>::read_type(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_absptr:
			{
				if(ptrsize==8)
				{
					if(eh_frame_util_t<ptrsize>::read_type_with_encoding(DW_EH_PE_udata8, value, position, data, max, section_start_addr))
						return true;
					break;
				}
				else if(ptrsize==4)
				{
					if(eh_frame_util_t<ptrsize>::read_type_with_encoding(DW_EH_PE_udata4, value, position, data, max, section_start_addr))
						return true;
					break;
				}
				assert(0);
					
			}
			case DW_EH_PE_sdata2 :
			{
				auto newval=int16_t(0);
				if(eh_frame_util_t<ptrsize>::read_type(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_sdata4 :
			{
				auto newval=int32_t(0);
				if(eh_frame_util_t<ptrsize>::read_type(newval,position,data,max))
					return true;
				value=newval;
				break;
			}
			case DW_EH_PE_sdata8 :
			{
				auto newval=int64_t(0);
				if(read_type(newval,position,data,max))
					return true;
				value=newval;
				break;
			}

			case DW_EH_PE_signed :
			default:
				assert(0);
		};

		switch(encoding_upper8)
		{
			case DW_EH_PE_absptr:
				break; 
			case DW_EH_PE_pcrel  :
				value+=section_start_addr+orig_position;
				break;
			case DW_EH_PE_textrel:
			case DW_EH_PE_datarel:
			case DW_EH_PE_funcrel:
			case DW_EH_PE_aligned:
			case DW_EH_PE_indirect:
			default:
				assert(0);
				return true;
		}
		return false;
	}

	static bool read_string 
		(string &s, 
		unsigned int & position, 
		const uint8_t* const data, 
		const int max)
	{
		while(data[position]!='\0' && position < max)
		{
			s+=data[position];	
			position++;
		}

		position++;
		return (position>max);
	}


	// see https://en.wikipedia.org/wiki/LEB128
	static bool read_uleb128 
		( uint64_t &result, 
		uint32_t& position, 
		const uint8_t* const data, 
		const uint32_t max)
	{
		result = 0;
		auto shift = 0;
		while( position < max )
		{
			auto byte = data[position];
			position++;
			result |= ( ( byte & 0x7f ) << shift);
			if ( ( byte & 0x80) == 0)
				break;
			shift += 7;
		}
		return ( position > max );

	}
	// see https://en.wikipedia.org/wiki/LEB128
	static bool read_sleb128 ( 
		int64_t &result, 
		uint32_t & position, 
		const uint8_t* const data, 
		const uint32_t max)
	{
		result = 0;
		auto shift = 0;
		auto size = 64;  // number of bits in signed integer;
		auto byte=uint8_t(0);
		do
		{
			byte = data [position]; 
			position++;
			result |= ((byte & 0x7f)<< shift);
			shift += 7;
		} while( (byte & 0x80) != 0);

		/* sign bit of byte is second high order bit (0x40) */
		if ((shift < size) && ( (byte & 0x40) !=0 /* sign bit of byte is set */))
			/* sign extend */
			result |= - (1 << shift);
		return ( position > max );

	}
	
	static bool read_length(
		uint64_t &act_length, 
		unsigned int &position, 
		const uint8_t* const data, 
		const uint32_t max)
	{
		auto eh_frame_scoop_data=data;
		auto length=uint32_t(0);
		auto length_64bit=uint64_t(0);
		if(read_type(length,position, eh_frame_scoop_data, max))
			return true;

		if(length==0xffffffff)
		{
			if(read_type(length_64bit,position, eh_frame_scoop_data, max))
				return true;
			act_length=length_64bit;
		}
		else
			act_length=length;

		return false;
	}
};

template <int ptrsize>
class eh_program_insn_t 
{
	public: 

	void print() const
	{
		// make sure uint8_t is an unsigned char.	
		static_assert(std::is_same<unsigned char, uint8_t>::value, "uint8_t is not unsigned char");

		auto data=program_bytes.data();
		auto opcode=program_bytes[0];
		auto opcode_upper2=(uint8_t)(opcode >> 6);
		auto opcode_lower6=(uint8_t)(opcode & (0x3f));
		auto pos=uint32_t(1);
		auto max=program_bytes.size();

		switch(opcode_upper2)
		{
			case 1:
			{
				// case DW_CFA_advance_loc:
				cout<<"				cfa_advance_loc "<<dec<<+opcode_lower6<<endl;
				break;
			}
			case 2:
			{
				uint64_t uleb=0;
				if(eh_frame_util_t<ptrsize>::read_uleb128(uleb, pos, data, max))
					return;
				// case DW_CFA_offset:
				cout<<"				cfa_offset "<<dec<<uleb<<endl;
				break;
			}
			case 3:
			{
				// case DW_CFA_offset:
				cout<<"				cfa_restore"<<endl;
				break;
			}
			case 0:
			{
				switch(opcode_lower6)
				{
				
					case DW_CFA_nop:
						cout<<"				nop" <<endl;
						break;
					case DW_CFA_remember_state:
						cout<<"				remember_state" <<endl;
						break;
					case DW_CFA_restore_state:
						cout<<"				restore_state" <<endl;
						break;

					// takes single uleb128
					case DW_CFA_undefined:
						cout<<"				undefined" ;
						print_uleb_operand(pos,data,max); 
						cout<<endl;
						break;
			
					case DW_CFA_same_value:
						cout<<"				same_value ";
						print_uleb_operand(pos,data,max); 
						cout<<endl;
						break;
					case DW_CFA_restore_extended:
						cout<<"				restore_extended ";
						print_uleb_operand(pos,data,max); 
						cout<<endl;
						break;
					case DW_CFA_def_cfa_register:
						cout<<"				def_cfa_register ";
						print_uleb_operand(pos,data,max); 
						cout<<endl;
						break;
					case DW_CFA_GNU_args_size:
						cout<<"				GNU_arg_size ";
						print_uleb_operand(pos,data,max); 
						cout<<endl;
						break;
					case DW_CFA_def_cfa_offset:
						cout<<"				def_cfa_offset "; 
						print_uleb_operand(pos,data,max); 
						cout<<endl;
						break;

					case DW_CFA_set_loc:
					{
						auto arg=uintptr_t(0xDEADBEEF);
						switch(ptrsize)
						{
							case 4:
								arg=*(uint32_t*)data[pos]; break;
							case 8:
								arg=*(uint64_t*)data[pos]; break;
						}
						cout<<"				set_loc "<<hex<<arg<<endl;
						break;
					}
					case DW_CFA_advance_loc1:
					{
						auto loc=*(uint8_t*)(&data[pos]);
						cout<<"                             advance_loc1 "<<+loc<<endl;
						break;
					}

					case DW_CFA_advance_loc2:
					{
						auto loc=*(uint16_t*)(&data[pos]);
						cout<<"                             advance_loc1 "<<+loc<<endl;
						break;
					}

					case DW_CFA_advance_loc4:
					{
						auto loc=*(uint32_t*)(&data[pos]);
						cout<<"                             advance_loc1 "<<+loc<<endl;
						break;
					}
					case DW_CFA_offset_extended:
						cout<<"				offset_extended ";
						print_uleb_operand(pos,data,max);
						print_uleb_operand(pos,data,max);
						cout<<endl;
						break;
					case DW_CFA_register:
						cout<<"				register ";
						print_uleb_operand(pos,data,max);
						print_uleb_operand(pos,data,max);
						cout<<endl;
						break;
					case DW_CFA_def_cfa:
						cout<<"				def_cfa ";
						print_uleb_operand(pos,data,max);
						print_uleb_operand(pos,data,max);
						cout<<endl;
						break;
					case DW_CFA_def_cfa_sf:
						cout<<"				def_cfa_sf ";
						print_uleb_operand(pos,data,max);
						print_sleb_operand(pos,data,max);
						cout<<endl;
						break;

					case DW_CFA_def_cfa_expression:
					{
						auto uleb=uint64_t(0);
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb, pos, data, max))
							return ;
						cout<<"					def_cfa_expression "<<dec<<uleb;
						pos+=uleb;		// doing this old school for now, as we aren't printing the expression.
						break;
					}
					case DW_CFA_expression:
					{
						auto uleb1=uint64_t(0);
						auto uleb2=uint64_t(0);
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb1, pos, data, max))
							return ;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb2, pos, data, max))
							return ;
						cout<<"                              expression "<<dec<<uleb1<<" "<<uleb2<<endl;
						pos+=uleb2;
						break;
					}
					case DW_CFA_val_expression:
					{
						auto uleb1=uint64_t(0);
						auto uleb2=uint64_t(0);
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb1, pos, data, max))
							return ;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb2, pos, data, max))
							return ;
						cout<<"                              val_expression "<<dec<<uleb1<<" "<<uleb2<<endl;
						pos+=uleb2;
						break;
					}
					case DW_CFA_def_cfa_offset_sf:
					{
						auto leb=int64_t(0);
						if(eh_frame_util_t<ptrsize>::read_sleb128(leb, pos, data, max))
							return ;
						cout<<"					def_cfa_offset_sf "<<dec<<leb;
						break;
					}
					case DW_CFA_offset_extended_sf:
					{
						auto uleb1=uint64_t(0);
						auto sleb2=int64_t(0);
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb1, pos, data, max))
							return ;
						if(eh_frame_util_t<ptrsize>::read_sleb128(sleb2, pos, data, max))
							return ;
						cout<<"                              offset_extended_sf "<<dec<<uleb1<<" "<<sleb2<<endl;
						break;
					}


					/* SGI/MIPS specific */
					case DW_CFA_MIPS_advance_loc8:

					/* GNU extensions */
					case DW_CFA_GNU_window_save:
					case DW_CFA_GNU_negative_offset_extended:
					default:
						cout<<"Unhandled opcode cannot print. opcode="<<opcode<<endl;
				}
				break;
			}
		}

	}

	void push_byte(uint8_t c) { program_bytes.push_back(c); }

	static void print_uleb_operand(
		uint32_t pos, 
		const uint8_t* const data, 
		const uint32_t max) 
	{
		auto uleb=uint64_t(0xdeadbeef);
		eh_frame_util_t<ptrsize>::read_uleb128(uleb, pos, data, max);
		cout<<" "<<dec<<uleb;
	}

	static void print_sleb_operand(
		uint32_t pos, 
		const uint8_t* const data, 
		const uint32_t max) 
	{
		auto leb=int64_t(0xdeadbeef);
		eh_frame_util_t<ptrsize>::read_sleb128(leb, pos, data, max);
		cout<<" "<<dec<<leb;
	}

	bool parse_insn(
		uint8_t opcode, 
		uint32_t& pos, 
		const uint8_t* const data, 
		const uint32_t &max)
	{
		auto &eh_insn = *this;
		auto insn_start=pos-1;
		auto opcode_upper2=(uint8_t)(opcode >> 6);
		auto opcode_lower6=(uint8_t)(opcode & (0x3f));

		// calculate the end of the instruction, which is inherently per-opcode
		switch(opcode_upper2)
		{
			case 1:
			{
				// case DW_CFA_advance_loc:
				break;
			}
			case 2:
			{
				auto uleb=uint64_t(0);
				if(eh_frame_util_t<ptrsize>::read_uleb128(uleb, pos, data, max))
					return true;
				// case DW_CFA_offset:
				break;
			}
			case 3:
			{
				// case DW_CFA_offset:
				break;
			}
			case 0:
			{
				switch(opcode_lower6)
				{
				
					case DW_CFA_nop:
					case DW_CFA_remember_state:
					case DW_CFA_restore_state:
						break;

					// takes single uleb128
					case DW_CFA_undefined:
					case DW_CFA_same_value:
					case DW_CFA_restore_extended:
					case DW_CFA_def_cfa_register:
					case DW_CFA_GNU_args_size:
					case DW_CFA_def_cfa_offset:
					{
						uint64_t uleb=0;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb, pos, data, max))
							return true;
						break;
					}

					case DW_CFA_set_loc:
						pos+=ptrsize;
						break;

					case DW_CFA_advance_loc1:
						pos+=1;
						break;

					case DW_CFA_advance_loc2:
						pos+=2;
						break;

					case DW_CFA_advance_loc4:
						pos+=4;
						break;

					case DW_CFA_offset_extended:
					case DW_CFA_register:
					case DW_CFA_def_cfa:
					{
						uint64_t uleb1=1, uleb2=0;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb1, pos, data, max))
							return true;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb2, pos, data, max))
							return true;
						break;
					}
					case DW_CFA_def_cfa_sf:
					{
						uint64_t leb1=0;
						int64_t leb2=0;
						if(eh_frame_util_t<ptrsize>::read_uleb128(leb1, pos, data, max))
							return true;
						if(eh_frame_util_t<ptrsize>::read_sleb128(leb2, pos, data, max))
							return true;
						break;
					}

					case DW_CFA_def_cfa_expression:
					{
						uint64_t uleb=0;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb, pos, data, max))
							return true;
						pos+=uleb;	
						break;
					}
					case DW_CFA_expression:
    					case DW_CFA_val_expression:
					{
						uint64_t uleb1=0, uleb2=0;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb1, pos, data, max))
							return true;
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb2, pos, data, max))
							return true;
						pos+=uleb2;
						break;
					}
					case DW_CFA_def_cfa_offset_sf:
					{
						auto leb=int64_t(0);
						if(eh_frame_util_t<ptrsize>::read_sleb128(leb, pos, data, max))
							return true;
						break;
					}
					case DW_CFA_offset_extended_sf:
					{
						auto uleb1=uint64_t(0);
						auto sleb2=int64_t(0);
						if(eh_frame_util_t<ptrsize>::read_uleb128(uleb1, pos, data, max))
							return true;
						if(eh_frame_util_t<ptrsize>::read_sleb128(sleb2, pos, data, max))
							return true;
						break;
					}
					/* Dwarf 2.1 */
    					case DW_CFA_val_offset:
    					case DW_CFA_val_offset_sf:


					/* SGI/MIPS specific */
					case DW_CFA_MIPS_advance_loc8:

					/* GNU extensions */
					case DW_CFA_GNU_window_save:
					case DW_CFA_GNU_negative_offset_extended:
					default:
						// Unhandled opcode cannot xform this eh-frame
						cout<<"No decoder for opcode "<<+opcode<<endl;
						return true;
				}
				break;
			}
			default:
				cout<<"No decoder for opcode "<<+opcode<<endl;
				return true;
		}

		// insert bytes into the instruction.
		auto insn_end=pos;
		for_each( &data[insn_start], &data[insn_end], [&](const uint8_t c)
		{
			eh_insn.push_byte(c);
		});
		return false;
	}



	private:

	vector<uint8_t> program_bytes;
};

template <int ptrsize>
class eh_program_t
{
	public:
	void push_insn(const eh_program_insn_t<ptrsize> &i) { instructions.push_back(i); }

	void print() const
	{
		cout << "			Program:                  " << endl ;
		for_each(instructions.begin(), instructions.end(), [&](const eh_program_insn_t<ptrsize>& i)
		{ 
			i.print();
		});
	}

	bool parse_program(
		const uint32_t& program_start_position, 
		const uint8_t* const data, 
		const uint32_t &max_program_pos)
	{
		eh_program_t &eh_pgm=*this;
		auto max=max_program_pos;
		auto pos=program_start_position;
		while(pos < max_program_pos)
		{
			uint8_t opcode;
			if(eh_frame_util_t<ptrsize>::read_type(opcode,pos,data,max))
				return true;
			eh_program_insn_t<ptrsize> eh_insn;
			if(eh_insn.parse_insn(opcode,pos,data,max))
				return true;

			eh_pgm.push_insn(eh_insn);

		
		}

		auto program_end_position=pos;

		//cout<<endl;
		return false;
	}
	private:
	vector<eh_program_insn_t <ptrsize> > instructions;
};

template <int ptrsize>
class cie_contents_t : eh_frame_util_t<ptrsize>
{
	private:
	uint64_t cie_position;
	uint64_t length;
	uint8_t cie_id;
	uint8_t cie_version;
	string augmentation;
	uint64_t code_alignment_factor;
	uint64_t data_alignment_factor;
	uint64_t return_address_register_column;
	uint64_t augmentation_data_length;
	uint8_t personality_encoding;
	uint64_t personality;
	uint8_t lsda_encoding;
	uint8_t fde_encoding;
	eh_program_t<ptrsize> eh_pgm;

	public:

	cie_contents_t() :
		cie_position(0),
		length(0),
		cie_id(0),
		cie_version(0),
		code_alignment_factor(0),
		data_alignment_factor(0),
		return_address_register_column(0),
		augmentation_data_length(0),
		personality_encoding(0),
		personality(0),
		lsda_encoding(0),
		fde_encoding(0)
	{}
	

	string GetAugmentation() const { return augmentation; }
	uint8_t GetLSDAEncoding() const { return lsda_encoding;}
	uint8_t GetFDEEncoding() const { return fde_encoding;}

	bool parse_cie(
		const uint32_t &cie_position, 
		const uint8_t* const data, 
		const uint32_t max, 
		const uint64_t eh_addr)
	{
		auto &c=*this;
		const auto eh_frame_scoop_data= data;
		auto position=cie_position;
		auto length= uint64_t(0);

		if(this->read_length(length, position, eh_frame_scoop_data, max))
			return true;

		auto end_pos=position+length;

		auto cie_id=uint32_t(0);
		if(this->read_type(cie_id, position, eh_frame_scoop_data, max))
			return true;

		auto cie_version=uint8_t(0);
		if(this->read_type(cie_version, position, eh_frame_scoop_data, max))
			return true;

		if(cie_version==1) 
		{ } // OK
		else if(cie_version==3) 
		{ } // OK
		else
		    // Err.
			return true;	

		auto augmentation=string();
		if(this->read_string(augmentation, position, eh_frame_scoop_data, max))
			return true;

		auto code_alignment_factor=uint64_t(0);
		if(this->read_uleb128(code_alignment_factor, position, eh_frame_scoop_data, max))
			return true;
		
		auto data_alignment_factor=int64_t(0);
		if(this->read_sleb128(data_alignment_factor, position, eh_frame_scoop_data, max))
			return true;

		// type depends on version info.  can always promote to 64 bits.
		auto return_address_register_column=uint64_t(0);
		if(cie_version==1)
		{
			auto return_address_register_column_8=uint8_t(0);
			if(this->read_type(return_address_register_column_8, position, eh_frame_scoop_data, max))
				return true;
			return_address_register_column=return_address_register_column_8;
		}
		else if(cie_version==3)
		{
			auto return_address_register_column_64=uint64_t(0);
			if(this->read_uleb128(return_address_register_column_64, position, eh_frame_scoop_data, max))
				return true;
			return_address_register_column=return_address_register_column_64;
		}
		else
			assert(0);

		auto augmentation_data_length=uint64_t(0);
		if(augmentation.find("z") != string::npos)
		{
			if(this->read_uleb128(augmentation_data_length, position, eh_frame_scoop_data, max))
				return true;
		}
		auto personality_encoding=uint8_t(DW_EH_PE_omit);
		auto personality=uint64_t(0);
		if(augmentation.find("P") != string::npos)
		{
			if(this->read_type(personality_encoding, position, eh_frame_scoop_data, max))
				return true;

			// indirect is OK as a personality encoding, but we don't need to go that far.
			// we just need to record what's in the CIE, regardless of whether it's the actual
			// personality routine or it's the pointer to the personality routine.
			auto personality_encoding_sans_indirect = personality_encoding&(~DW_EH_PE_indirect);
			if(this->read_type_with_encoding(personality_encoding_sans_indirect, personality, position, eh_frame_scoop_data, max, eh_addr))
				return true;
		}

		auto lsda_encoding=uint8_t(DW_EH_PE_omit);
		if(augmentation.find("L") != string::npos)
		{
			if(this->read_type(lsda_encoding, position, eh_frame_scoop_data, max))
				return true;
		}
		auto fde_encoding=uint8_t(DW_EH_PE_omit);
		if(augmentation.find("R") != string::npos)
		{
			if(this->read_type(fde_encoding, position, eh_frame_scoop_data, max))
				return true;
		}
		if(eh_pgm.parse_program(position, eh_frame_scoop_data, end_pos))
			return true;


		c.cie_position=cie_position;
		c.cie_id=cie_id;
		c.cie_version=cie_version;
		c.augmentation=augmentation;
		c.code_alignment_factor=code_alignment_factor;
		c.data_alignment_factor=data_alignment_factor;
		c.return_address_register_column=return_address_register_column;
		c.augmentation_data_length=augmentation_data_length;
		c.personality_encoding=personality_encoding;
		c.personality=personality;
		c.lsda_encoding=lsda_encoding;
		c.fde_encoding=fde_encoding;

		// all OK
		return false;
	}
	void print() const 
	{
		cout << "["<<setw(6)<<hex<<cie_position<<"] CIE length="<<dec<<length<<endl;
		cout << "   CIE_id:                   " << +cie_id << endl;
		cout << "   version:                  " << +cie_version << endl;
		cout << "   augmentation:             \"" << augmentation << "\"" << endl;
		cout << "   code_alignment_factor:    " << code_alignment_factor << endl;
		cout << "   data_alignment_factor:    " << dec << data_alignment_factor << endl;
		cout << "   return_address_register:  " << dec << return_address_register_column << endl;
		cout << "   Augmentation data:        " << endl ;
		cout << "                             aug data len:         " << hex << +augmentation_data_length << endl;
		cout << "                             personality_encoding: " << hex << +personality_encoding << endl;
		cout << "                             personality:          " << hex << +personality << endl;
		cout << "                             lsda_encoding:        " << hex << +lsda_encoding << endl;
		cout << "                             fde_encoding:         " << hex << +fde_encoding << endl;
		cout << "   Program:        " << endl ;
		eh_pgm.print();
		
	}
};


template <int ptrsize>
class lsda_call_site_action_t : private eh_frame_util_t<ptrsize>
{
	private:
	int64_t action;

	public:
	lsda_call_site_action_t() :
		action(0)
	{}

	int64_t GetAction() const { return action;}

	bool parse_lcsa(uint32_t& pos, const uint8_t* const data, const uint64_t max, bool &end)
	{
		end=false;
		if(this->read_sleb128(action, pos, data, max))
			return true;

		assert(action>=0);

		auto next_action=pos;
		auto next_pos_offset=int64_t(0);
		if(this->read_sleb128(next_pos_offset, pos, data, max))
			return true;
	
		if(next_pos_offset==0)
			end=true;
		else
			pos=next_action+next_pos_offset;
		return false;
	}
	void print() const
	{
		cout<<"					"<<action<<endl;
	}
};
template <int ptrsize>
bool operator< (const lsda_call_site_action_t <ptrsize> &lhs, const lsda_call_site_action_t <ptrsize> &rhs)
{ 	
	return lhs.GetAction() < rhs.GetAction(); 
}

template <int ptrsize>
class lsda_type_table_entry_t: private eh_frame_util_t<ptrsize>
{
	private:
	uint64_t pointer_to_typeinfo;

	public:
	lsda_type_table_entry_t() : 
		pointer_to_typeinfo(0)
	{}

	bool parse(
		const uint64_t tt_encoding, 	
		const uint64_t tt_pos, 	
		const uint64_t index,
		const uint8_t* const data, 
		const uint64_t max,  
		const uint64_t data_addr
		)
	{
		auto size=uint32_t(0);
		switch(tt_encoding & 0xf) // get just the size field
		{
			case DW_EH_PE_udata4:
			case DW_EH_PE_sdata4:
				size=4;
				break;
			default:
				assert(0);
		}
		auto act_pos=uint32_t(tt_pos+(-index*size));
		if(this->read_type_with_encoding(tt_encoding, pointer_to_typeinfo, act_pos, data, max, data_addr))
			return true;

		return false;
	}

	void print() const
	{
		cout<<"				pointer_to_typeinfo: 0x"<<hex<<pointer_to_typeinfo<<endl;
	}

	
};


template <int ptrsize>
class lsda_call_site_t : private eh_frame_util_t<ptrsize>
{
	private:
	uint64_t call_site_offset;
	uint64_t call_site_addr;
	uint64_t call_site_length;
	uint64_t call_site_end_addr;
	uint64_t landing_pad_offset;
	uint64_t landing_pad_addr;
	uint64_t action;
	uint64_t action_table_offset;
	uint64_t action_table_addr;

	vector<lsda_call_site_action_t <ptrsize> > action_table;

	public:
	lsda_call_site_t() :
		call_site_offset(0),
		call_site_addr(0),
		call_site_length(0),
		call_site_end_addr(0),
		landing_pad_offset(0),
		landing_pad_addr(0),
		action(0),
		action_table_offset(0),
		action_table_addr(0)
	{}

	int64_t GetMaxTypeTableIndex() const
	{
		if(action_table.size()==0)
			return 0;
		return max_element(action_table.begin(), action_table.end())->GetAction();
	}
	

	bool parse_lcs(	
		const uint64_t action_table_start_addr, 	
		const uint64_t cs_table_start_addr, 	
		const uint8_t cs_table_encoding, 
		uint32_t &pos, 
		const uint8_t* const data, 
		const uint64_t max,  /* call site table max */
		const uint64_t data_addr, 
		const uint64_t landing_pad_base_addr,
		const uint64_t gcc_except_table_max)
	{
		
		if(this->read_type_with_encoding(cs_table_encoding, call_site_offset, pos, data, max, data_addr))
			return true;
		call_site_addr=landing_pad_base_addr+call_site_offset;
		if(this->read_type_with_encoding(cs_table_encoding, call_site_length, pos, data, max, data_addr))
			return true;
		call_site_end_addr=call_site_addr+call_site_length;
		if(this->read_type_with_encoding(cs_table_encoding, landing_pad_offset, pos, data, max, data_addr))
			return true;

		// calc the actual addr.
		if(landing_pad_offset == 0)
			landing_pad_addr=0;
		else
			landing_pad_addr=landing_pad_base_addr+landing_pad_offset;

		if(this->read_uleb128(action, pos, data, max))
			return true;

		if(action == 0)
		{ /* no action table */ }
		else
		{
			action_table_offset=action-1;
			action_table_addr=action_table_start_addr+action-1;

			// parse action tables
			bool end=false;
			auto act_table_pos=uint32_t(action_table_addr-data_addr);
			while(!end)
			{
				lsda_call_site_action_t<ptrsize> lcsa;
				if(lcsa.parse_lcsa(act_table_pos, data, gcc_except_table_max, end))
					return true;
				action_table.push_back(lcsa);
				
			}
		
		}
	
		return false;
	}

	void print() const
	{
		cout<<"				CS Offset        : 0x"<<hex<<call_site_offset<<endl;
		cout<<"				CS len           : 0x"<<hex<<call_site_length<<endl;
		cout<<"				landing pad off. : 0x"<<hex<<landing_pad_offset<<endl;
		cout<<"				action (1+addr)  : 0x"<<hex<<action<<endl;
		cout<<"				---interpreted---"<<endl;
		cout<<"				CS Addr          : 0x"<<hex<<call_site_addr<<endl;
		cout<<"				CS End Addr      : 0x"<<hex<<call_site_end_addr<<endl;
		cout<<"				landing pad addr : 0x"<<hex<<landing_pad_addr<<endl;
		cout<<"				act-tab off      : 0x"<<hex<<action_table_offset<<endl;
		cout<<"				act-tab addr     : 0x"<<hex<<action_table_addr<<endl;
		cout<<"				act-tab 	 : "<<endl;
		for_each(action_table.begin(), action_table.end(), [&](const lsda_call_site_action_t<ptrsize>& p)
		{
			p.print();
		});
	}
};

template <int ptrsize>
class lsda_t : private eh_frame_util_t<ptrsize>
{
	private:
	uint8_t landing_pad_base_encoding;
	uint64_t landing_pad_base_addr; // often ommitted. when ommitted, filled in from FDE region start.
	uint8_t type_table_encoding;
	uint64_t type_table_offset;
	uint64_t type_table_addr;
	uint8_t cs_table_encoding;
	uint64_t cs_table_start_offset;
	uint64_t cs_table_start_addr;
	uint64_t cs_table_length;
	uint64_t cs_table_end_addr;
	uint64_t action_table_start_addr;
	vector<lsda_call_site_t <ptrsize> > call_site_table;
	vector<lsda_type_table_entry_t <ptrsize> > type_table;

	public:
	
	lsda_t() :
		landing_pad_base_encoding(0),
		landing_pad_base_addr(0),
		type_table_encoding(0),
		type_table_offset(0),
		type_table_addr(0),
		cs_table_encoding(0),
		cs_table_start_offset(0),
		cs_table_start_addr(0),
		cs_table_length(0),
		cs_table_end_addr(0),
		action_table_start_addr(0)
	{}
		
	bool parse_lsda(const uint64_t lsda_addr, const DataScoop_t* gcc_except_scoop, const uint64_t fde_region_start)
	{
		// make sure there's a coop and that we're in the range.
		if(!gcc_except_scoop)
			return true;
		if(lsda_addr<gcc_except_scoop->GetStart()->GetVirtualOffset())
			return true;
		if(lsda_addr>=gcc_except_scoop->GetEnd()->GetVirtualOffset())
			return true;

		const auto data=(const uint8_t* const)gcc_except_scoop->GetContents().data();
		const auto data_addr=gcc_except_scoop->GetStart()->GetVirtualOffset();
		const auto max=gcc_except_scoop->GetContents().size();
		auto pos=uint32_t(lsda_addr-data_addr);
		auto start_pos=pos;

		if(this->read_type(landing_pad_base_encoding, pos, data, max))
			return true;
		if(landing_pad_base_encoding!=DW_EH_PE_omit)
		{
			if(this->read_type_with_encoding(landing_pad_base_encoding,landing_pad_base_addr, pos, data, max, data_addr))
				return true;
		}
		else
			landing_pad_base_addr=fde_region_start;

		if(this->read_type(type_table_encoding, pos, data, max))
			return true;

		auto type_table_pos=0;
		if(type_table_encoding!=DW_EH_PE_omit)
		{
			if(this->read_uleb128(type_table_offset, pos, data, max))
				return true;
			type_table_addr=lsda_addr+type_table_offset+(pos-start_pos);
			type_table_pos=pos+type_table_offset;
		}
		else
			type_table_addr=0;

		if(this->read_type(cs_table_encoding, pos, data, max))
			return true;

		if(this->read_uleb128(cs_table_length, pos, data, max))
			return true;

		auto cs_table_end=pos+cs_table_length;
		auto cs_table_start_pos=pos;
		cs_table_start_offset=pos;
		cs_table_start_addr=lsda_addr+pos-start_pos;
		cs_table_end_addr=cs_table_start_addr+cs_table_length;

		// action table comes immediately after the call site table.
		action_table_start_addr=cs_table_start_addr+cs_table_length;
		auto max_tt_entry=uint64_t(0);
		while(1)
		{
			lsda_call_site_t<ptrsize> lcs;
			if(lcs.parse_lcs(action_table_start_addr,
				cs_table_start_addr,cs_table_encoding, pos, data, cs_table_end, data_addr, landing_pad_base_addr, max))
			{
				return true;
			}

			call_site_table.push_back(lcs);
			max_tt_entry=std::max(max_tt_entry, (uint64_t)lcs.GetMaxTypeTableIndex());
			
			if(pos>=cs_table_end)
				break;	
		}

		if(type_table_encoding!=DW_EH_PE_omit)
		{
			// 1-based indexing because of odd backwards indexing of type table.
			for(int i=1;i<=max_tt_entry;i++)
			{
				lsda_type_table_entry_t <ptrsize> ltte;
				auto type_table_encoding_sans_indirect = type_table_encoding&(~DW_EH_PE_indirect);
				if(ltte.parse(type_table_encoding_sans_indirect, type_table_pos, i, data, max, data_addr ))
					return true;
				type_table.push_back(ltte);
			}
		}

		return false;
	}
	void print() const
	{
		cout<<"		LSDA:"<<endl;
		cout<<"			LP base encoding   : 0x"<<hex<<+landing_pad_base_encoding<<endl;
		cout<<"			LP base addr	   : 0x"<<hex<<+landing_pad_base_addr<<endl;
		cout<<"			TypeTable encoding : 0x"<<hex<<+type_table_encoding<<endl;
		cout<<"			TypeTable offset   : 0x"<<hex<<type_table_offset<<endl;
		cout<<"			TypeTable addr     : 0x"<<hex<<+type_table_addr<<endl;
		cout<<"			CS tab encoding    : 0x"<<hex<<+cs_table_encoding<<endl;
		cout<<"			CS tab addr        : 0x"<<hex<<+cs_table_start_addr<<endl;
		cout<<"			CS tab offset      : 0x"<<hex<<+cs_table_start_offset<<endl;
		cout<<"			CS tab length      : 0x"<<hex<<+cs_table_length<<endl;
		cout<<"			CS tab end addr    : 0x"<<hex<<+cs_table_end_addr<<endl;
		cout<<"			Act tab start_addr : 0x"<<hex<<+action_table_start_addr<<endl;
		cout<<"			CS tab :"<<endl;
		int i=0;
		for_each(call_site_table.begin(), call_site_table.end(), [&](const lsda_call_site_t<ptrsize>& p)
		{
			cout<<"			[ "<<hex<<i++<<"] call site table entry "<<endl;
			p.print();
		});
		i=0;
		for_each(type_table.begin(), type_table.end(), [&](const lsda_type_table_entry_t<ptrsize>& p)
		{
			cout<<"			[ "<<hex<<i++<<"] Type table entry "<<endl;
			p.print();
		});
	}
};


template <int ptrsize>
class fde_contents_t : eh_frame_util_t<ptrsize>
{
	uint32_t fde_position;
	uint32_t cie_position;
	uint64_t length;
	uint8_t id;
	uint64_t fde_start_addr;
	uint64_t fde_end_addr;
	uint64_t fde_range_len;
	uint64_t lsda_addr;


	lsda_t<ptrsize> lsda;
	eh_program_t<ptrsize> eh_pgm;
	cie_contents_t<ptrsize> cie_info;

	public:
	fde_contents_t() :
		fde_position(0),
		cie_position(0),
		length(0),
		id(0),
		fde_start_addr(0),
		fde_end_addr(0),
		fde_range_len(0),
		lsda_addr(0)
	{}

	cie_contents_t<ptrsize> GetCIE() const { return cie_info; }
	eh_program_t<ptrsize> GetProgram() const { return eh_pgm; }

	bool parse_fde(
		const uint32_t &fde_position, 
		const uint32_t &cie_position, 
		const uint8_t* const data, 
		const uint64_t max, 
		const uint64_t eh_addr,
		const DataScoop_t* gcc_except_scoop)
	{
		auto &c=*this;
		const auto eh_frame_scoop_data=data;

		if(cie_info.parse_cie(cie_position, data, max, eh_addr))
			return true;

		auto pos=fde_position;
		auto length=uint64_t(0);
		if(this->read_length(length, pos, eh_frame_scoop_data, max))
			return true;

	
		auto end_pos=pos+length;
		auto end_length_position=pos;

		auto cie_id=uint32_t(0);
		if(this->read_type(cie_id, pos, eh_frame_scoop_data, max))
			return true;

		auto fde_start_addr=uint64_t(0);
		if(this->read_type_with_encoding(c.GetCIE().GetFDEEncoding(),fde_start_addr, pos, eh_frame_scoop_data, max, eh_addr))
			return true;

		auto fde_range_len=uint64_t(0);
		if(this->read_type_with_encoding(c.GetCIE().GetFDEEncoding() & 0xf /* drop pc-rel bits */,fde_range_len, pos, eh_frame_scoop_data, max, eh_addr))
			return true;

		auto fde_end_addr=fde_start_addr+fde_range_len;

		auto augmentation_data_length=uint64_t(0);
		if(c.GetCIE().GetAugmentation().find("z") != string::npos)
		{
			if(this->read_uleb128(augmentation_data_length, pos, eh_frame_scoop_data, max))
				return true;
		}
		auto lsda_addr=uint64_t(0);
		if(c.GetCIE().GetLSDAEncoding()!= DW_EH_PE_omit)
		{
			if(this->read_type_with_encoding(c.GetCIE().GetLSDAEncoding(), lsda_addr, pos, eh_frame_scoop_data, max, eh_addr))
				return true;
			if(c.lsda.parse_lsda(lsda_addr,gcc_except_scoop, fde_start_addr))
				return true;
		}

		if(c.eh_pgm.parse_program(pos, eh_frame_scoop_data, end_pos))
			return true;

		c.fde_position=fde_position;
		c.cie_position=cie_position;
		c.length=length;
		c.id=id;
		c.fde_start_addr=fde_start_addr;
		c.fde_end_addr=fde_end_addr;
		c.fde_range_len=fde_range_len;
		c.lsda_addr=lsda_addr;

		return false;
	}

	void print() const
	{

		cout << "["<<setw(6)<<hex<<fde_position<<"] FDE length="<<dec<<length;
		cout <<" cie=["<<setw(6)<<hex<<cie_position<<"]"<<endl;
		cout<<"		FDE len addr:		"<<dec<<length<<endl;
		cout<<"		FDE Start addr:		"<<hex<<fde_start_addr<<endl;
		cout<<"		FDE End addr:		"<<hex<<fde_end_addr<<endl;
		cout<<"		FDE len:		"<<dec<<fde_range_len<<endl;
		cout<<"		FDE LSDA:		"<<hex<<lsda_addr<<endl;
		eh_pgm.print();
		if(GetCIE().GetLSDAEncoding()!= DW_EH_PE_omit)
			lsda.print();
		else
			cout<<"		No LSDA for this FDE."<<endl;
	}


};

template <int ptrsize>
class split_eh_frame_t
{
	private: 


	FileIR_t* firp;
	DataScoop_t* eh_frame_scoop;
	DataScoop_t* eh_frame_hdr_scoop;
	DataScoop_t* gcc_except_table_scoop;
	map<virtual_offset_t,const Instruction_t*> offset_to_insn_map;
	vector<cie_contents_t <ptrsize> > cies;
	vector<fde_contents_t <ptrsize> > fdes;

	public:

	split_eh_frame_t(FileIR_t* p_firp)
		: firp(p_firp),
		  eh_frame_scoop(NULL),
		  eh_frame_hdr_scoop(NULL),
		  gcc_except_table_scoop(NULL)
	{
		assert(firp!=NULL);

		// function to find a scoop by name.
		auto lookup_scoop_by_name=[&](const string &name) -> DataScoop_t* 
		{
			auto scoop_it=find_if(firp->GetDataScoops().begin(), firp->GetDataScoops().end(), [name](DataScoop_t* scoop)
			{
				return scoop->GetName()==name;
			});

			if(scoop_it!=firp->GetDataScoops().end())
				return *scoop_it;
			return NULL;
		};

		eh_frame_scoop=lookup_scoop_by_name(".eh_frame");
		eh_frame_hdr_scoop=lookup_scoop_by_name(".eh_frame_hdr");
		gcc_except_table_scoop=lookup_scoop_by_name(".gcc_except_table");

	}

	bool init_offset_map()
	{
		for_each(firp->GetInstructions().begin(), firp->GetInstructions().end(), [&](const Instruction_t* i)
		{
			offset_to_insn_map[i->GetAddress()->GetVirtualOffset()]=i;
		});
		return false;
	}

	bool iterate_fdes()
	{
		auto eh_frame_scoop_data=(const uint8_t* const)eh_frame_scoop->GetContents().c_str();
		auto data=eh_frame_scoop_data;
		auto eh_addr= eh_frame_scoop->GetStart()->GetVirtualOffset();
		auto max=eh_frame_scoop->GetContents().size();
		auto position=uint32_t(0);

		//cout << "----------------------------------------"<<endl;
		while(1)
		{
			auto old_position=position;
			auto act_length=uint64_t(0);

			if(eh_frame_util_t<ptrsize>::read_length(act_length, position, eh_frame_scoop_data, max))
				break;

			auto next_position=position + act_length;
			auto cie_offset=uint32_t(0);
			auto cie_offset_position=position;

			if(eh_frame_util_t<ptrsize>::read_type(cie_offset,position, eh_frame_scoop_data, max))
				break;

			//cout << " [ " << setw(6) << hex << old_position << "] " ;
			if(act_length==0)
			{
				//cout << "Zero terminator " << endl;
				break;
			}
			else if(cie_offset==0)
			{
				//cout << "CIE length="<< dec << act_length << endl;
				cie_contents_t<ptrsize> c;
				if(c.parse_cie(old_position, data, max, eh_addr))
					return true;
				cies.push_back(c);
			}
			else
			{
				fde_contents_t<ptrsize> f;
				auto cie_position = cie_offset_position - cie_offset;
				//cout << "FDE length="<< dec << act_length << " cie=[" << setw(6) << hex << cie_position << "]" << endl;
				if(f.parse_fde(old_position, cie_position, data, max, eh_addr, gcc_except_table_scoop))
					return true;
				fdes.push_back(f);
			}
			//cout << "----------------------------------------"<<endl;
			

			// next CIE/FDE
			assert(position<=next_position); 	// so we don't accidentally over-read a CIE/FDE
			position=next_position;
		}
		return false;
	}

	bool execute()
	{
		if(eh_frame_scoop==NULL)
			return true; // no frame info in this binary


		if(init_offset_map())
			return true;

		if(iterate_fdes())
			return true;

		return false;
	}

	void print() const
	{
		for_each(cies.begin(), cies.end(), [&](const cie_contents_t<ptrsize>  &p)
		{
			p.print();
		});
		for_each(fdes.begin(), fdes.end(), [&](const fde_contents_t<ptrsize>  &p)
		{
			p.print();
		});
	}
};

void split_eh_frame(FileIR_t* firp)
{
	auto found_err=false;
	if( firp->GetArchitectureBitWidth()==64)
	{
		split_eh_frame_t<8> eh_frame_splitter(firp);
		found_err=eh_frame_splitter.execute();
		eh_frame_splitter.print();
	}
	else
	{
		split_eh_frame_t<4> eh_frame_splitter(firp);
		found_err=eh_frame_splitter.execute();
		eh_frame_splitter.print();
	}
	assert(!found_err);
}
