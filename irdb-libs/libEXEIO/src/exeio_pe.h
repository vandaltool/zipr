#ifndef EXEIO_PE_H
#define EXEIO_PE_H

#ifndef SOLARIS
#include <iostream>
#include <vector>
#include <assert.h>
#include <fstream>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <pe_bliss.h>
#pragma GCC diagnostic pop


class exeio_backend;
class exeio_section;


namespace EXEIO
{
	class exeio_pe_section_t : public exeio_section_t
	{
		public:
			exeio_pe_section_t(const pe_bliss::section *the_s, const pe_bliss::pe_base *the_b) 
				: s(the_s),b(the_b) { assert(s); assert(b);}

			bool isLoadable() const { return s->readable(); }
			bool isExecutable() const { return s->executable(); }
			bool isWriteable() const { return s->writeable(); }
			bool isReadable() const { return s->readable(); }
			bool isBSS() const  { return s->empty(); }
			const char* get_data() const  { return s->get_virtual_data(b->get_section_alignment()).c_str(); }
			std::string get_name() const { return s->get_name(); }
			int get_size() const  { return s->get_virtual_size(); }
			int get_type() const  { assert(0); } // not imp'd yet
			EXEIO::virtual_offset_t get_address() const { 
				EXEIO::virtual_offset_t base = b->get_image_base_64();
				return base + s->get_virtual_address(); 
			}
			bool mightContainStrings() const { assert(0); }

		private:
			const pe_bliss::section *s;
			const pe_bliss::pe_base *b;
	};

	class exeio_pe_backend_t : public exeio_backend_t
	{
		public:
			exeio_pe_backend_t() 
				: 
				e(NULL), 
				pe_sections(NULL),
				main(NULL)
			{
			}

			~exeio_pe_backend_t()
			{
				if(!main)
					return;
				// remove subclasses.
				for(int i=0;i<main->sections.size(); ++i)
				{
					// copy each section into the main class' structure.
					EXEIO::exeio_pe_section_t* sec=dynamic_cast<EXEIO::exeio_pe_section_t*>(main->sections[i]);			
					delete sec;
				}

				// remove the pe_bliss class.
				delete e;
			}
			void load(exeio* the_main, const char* filename)
			{
				main=the_main;

				std::ifstream pe_file(filename, std::ios::in | std::ios::binary);
        			if(!pe_file)
        			{
                			std::cerr << "Cannot open " << filename << std::endl;
                			assert(0);
					exit(1);
        			}


				e=new pe_bliss::pe_base(pe_bliss::pe_factory::create_pe(pe_file));
				assert(e);

				pe_sections=new pe_bliss::section_list(e->get_image_sections());

                		for(pe_bliss::section_list::const_iterator it = pe_sections->begin(); 
					it != pe_sections->end(); ++it)
                		{
                        		const pe_bliss::section& s = *it; 
					main->sections.add_section(new EXEIO::exeio_pe_section_t(&s,e));
				}

	
			}
                        virtual void dump_header(std::ostream& stream) 
			{
				assert(e);

                		std::cout << "EP : " <<std::hex<< e->get_ep() << std::endl;
                		std::cout << "EP section name: " << e->section_from_rva(e->get_ep()).get_name() << std::endl;
                		std::cout << "EP section data length: " << 
					e->section_data_length_from_rva(e->get_ep()) << std::endl;

                		if(e->has_imports())
				{
                        		std::cout << "Import section name: " << 
						e->section_from_directory(pe_bliss::pe_win
						::image_directory_entry_import).get_name() 
						<< std::endl;
				}
			}
                        virtual void dump_section_headers(std::ostream& stream) 
			{
				assert(e);
                		for(pe_bliss::section_list::const_iterator it = pe_sections->begin(); 
					it != pe_sections->end(); ++it)
                		{
                        		const pe_bliss::section& s = *it; //Секция
                        		std::cout << "Section [" << s.get_name() << "]" << std::endl //Имя секции
                                		<< "Characteristics: " << s.get_characteristics() << std::endl 
                                		<< "Size of raw data: " << s.get_size_of_raw_data() << std::endl 
                                		<< "Virtual address: " << s.get_virtual_address() << std::endl 
                                		<< "Virtual size: " << s.get_virtual_size() << std::endl 
                                		<< std::endl;
                		}

			}
			virtual MachineType_t getMachineType() const
			{
				assert(e);
                                switch(e->get_machine())
                                {
                                        case 0x14c  : return mtI386;
                                        case 0x8664 : return mtX86_64;
                                        default: assert(0);
                                }
                                assert(0);

			}

                        virtual execlass_t get_class() 
			{
				assert(e);
        			if(e->get_pe_type() == pe_bliss::pe_type_32)
					return PE32;
        			if(e->get_pe_type() == pe_bliss::pe_type_64)
					return PE64;
				assert(0);
			}

			// get entry point of function.
                        virtual virtual_offset_t get_entry()
			{
				assert(e);
				/* works for 32-bit too */
				return (virtual_offset_t)e->get_image_base_64() + e->get_ep();
			}

			virtual bool isDLL() { return true; } 

                        virtual void* get_pebliss() { assert(e); return static_cast<void*>(e); }
	
		private:  
			pe_bliss::pe_base* e;
			pe_bliss::section_list* pe_sections;
			EXEIO::exeio* main;
	};
	

}
#endif // solaris
#endif // exeio_pe_h
