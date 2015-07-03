#ifndef EXEIO_ELF_H
#define EXEIO_ELF_H

#include <iostream>
#include <vector>
#include <assert.h>


#include "targ-config.h"
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

class exeio_backend;
class exeio_section;


namespace EXEIO
{
	class exeio_elf_section_t : public exeio_section_t
	{
		public:
			exeio_elf_section_t(ELFIO::section *the_s) : s(the_s) { assert(s); }

			bool isLoadable() const { return (s->get_flags() & SHF_ALLOC) == SHF_ALLOC; }
			bool isExecutable() const { return (s->get_flags() & SHF_EXECINSTR) == SHF_EXECINSTR; }
			bool isBSS() const { return (s->get_type() & SHT_NOBITS) == SHT_NOBITS; }
			const char* get_data() const { return s->get_data(); }
			std::string get_name() const { return s->get_name(); }
			int get_size() const { return s->get_size(); }
			EXEIO::virtual_offset_t get_address() const { return s->get_address(); }
			bool mightContainStrings() const { assert(0); }

		private:
			ELFIO::section *s;
	};

	class exeio_elf_backend_t : public exeio_backend_t
	{
		public:
			exeio_elf_backend_t() : e(NULL), main(NULL) {}
			~exeio_elf_backend_t()
			{
				if(!main)
					return;
				// remove subclasses.
				for(int i=0;i<e->sections.size(); ++i)
				{
					// copy each section into the main class' structure.
					delete main->sections[i];
				}

				// remove the elfio class.
				delete e;
			}
			void load(exeio* the_main, char* filename)
			{
				main=the_main;
				e=new ELFIO::elfio;	
				int res=e->load(filename);
				assert(res);

				// resize the sections.
				for(int i=0;i<e->sections.size(); ++i)
				{
					// copy each section into the main class' structure.
					main->sections.add_section(new EXEIO::exeio_elf_section_t(e->sections[i]));
				}
	
			}
                        virtual void dump_header(std::ostream& stream) 
			{
				assert(e);
				ELFIO::dump::header(stream,*e);
			}
                        virtual void dump_section_headers(std::ostream& stream) 
			{
				assert(e);
				ELFIO::dump::section_headers(stream,*e);
			}
                        virtual execlass_t get_class() 
			{
				assert(e);
				switch(e->get_class())  
				{ 
					case ELFCLASS64: return ELF64;
					case ELFCLASS32: return ELF32;
					default: assert(0);
				};
			}

			// get entry point of function.
                        virtual virtual_offset_t get_entry()
			{
				assert(e);
				return (virtual_offset_t)e->get_entry();
			}

			virtual void* get_elfio() { return (void*)e; }

	
		private:  
			ELFIO::elfio* e;
			EXEIO::exeio* main;
	};
	

}

#endif
