#ifndef EXEIO_H
#define EXEIO_H

#include <ostream>
#include <vector>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>


namespace EXEIO
{
	class exeio_t; // forward decl

	typedef enum { ELF64, ELF32, PE32, PE64 } execlass_t;
	typedef enum {  mtX86_64, mtI386, mtArm32, mtAarch64, mtMips32, mtMips64 } MachineType_t; 

	typedef uintptr_t virtual_offset_t;


	// create type to match elfio
	class exeio_section_t
	{
		public: 
			virtual ~exeio_section_t() {}  
			virtual bool isLoadable() const =0;
			virtual bool isExecutable() const =0;
			virtual bool isWriteable() const =0;
			virtual bool isReadable() const =0;
			virtual bool isBSS() const =0;
			virtual bool isThreadLocal() const { return false; }
			virtual const char* get_data() const =0;
			virtual std::string get_name() const =0;
			virtual int get_size() const =0;
			virtual int get_type() const =0;
			virtual EXEIO::virtual_offset_t get_address() const =0;
			virtual bool mightContainStrings() const =0;
	};		

	// break naming rule for elfio compatibility
	typedef exeio_section_t section;
		
	class exeio_backend_t
	{
		public:
			virtual ~exeio_backend_t() { }
			virtual void dump_header(std::ostream& stream) =0;
			virtual void dump_section_headers(std::ostream& stream) =0;
			virtual void load(exeio_t* main, const char* filename) =0;
                        virtual execlass_t get_class() =0;
                        virtual MachineType_t getMachineType() const =0;
			virtual virtual_offset_t get_entry() =0;
			virtual void* get_elfio()   { return NULL; }
			virtual void* get_pebliss() { return NULL; }
			virtual bool isDLL() =0;
			virtual bool isDynamicallyLinked() { return true; }

	};
	
	class exeio_sections_t
	{
		public:
			exeio_section_t* operator[](int i) { return the_sections[i]; }
			exeio_section_t* operator[](const std::string& name) 
			{ 
				for(auto s: the_sections)
					if(s->get_name()==name)
							return s;
				return NULL;
			}
			exeio_section_t* findByAddress(const uint64_t addr)
			{ 
				for(auto s: the_sections)
				{
					if(s->get_address() <= addr &&  addr < (s->get_address()+s->get_size()))
						return s;
				}
				return NULL;
			}
			int size() const { return (int)the_sections.size(); }

			void add_section(exeio_section_t* sec)
			{
				the_sections.push_back(sec);
			}
		private:
		
			std::vector<exeio_section_t*> the_sections;

		friend class exeio_backend_t;
		
	};


	class exeio_t
	{
		public:
			// constructors
			exeio_t()  { Init(); }
			exeio_t(const char* filename) { Init(); load(filename); }
			exeio_t(const std::string& filename) { Init(); load(filename.c_str()); }
			virtual ~exeio_t() { delete backend; }

			virtual void load(const std::string filename) { load((char*)filename.c_str()); }

			// load the file
			virtual void load(const char* fn);

			// trying to do minimal rewriting of code that uses
			// ELFIO namespace.  
			// This slightly odd construction of classes
			// is used by ELFIO because it makes for nice looking code when using ELFIO.
			// I quite like the results and there's a lot of code, so I'm mimicking it here.
			// Unfortunately it means a less-than-clean interface for the class writer, 
			// but we'll manage.
			exeio_sections_t sections;

			virtual virtual_offset_t get_entry() { assert(backend); return backend->get_entry(); }
		
			virtual void dump_header(std::ostream& stream) { assert(backend); backend->dump_header(stream); }
			virtual void dump_section_headers(std::ostream& stream) { assert(backend); backend->dump_section_headers(stream); }
                        virtual execlass_t get_class() { assert(backend); return backend->get_class(); }
                        virtual MachineType_t getMachineType() const { assert(backend); return backend->getMachineType(); }
			virtual void* get_elfio()   { assert(backend); return backend->get_elfio()  ; }
			virtual void* get_pebliss() { assert(backend); return backend->get_pebliss(); }
			virtual bool isDLL() { assert(backend); return backend->isDLL(); }
			virtual bool isDynamicallyLinked() { assert(backend); return backend->isDynamicallyLinked(); }

		private:
			void Init() { backend=NULL; }

			exeio_backend_t* backend;

		friend class exeio_backend;
			
	};

	// intentionally breaking _t rule for ELFIO compatibility.
	typedef exeio_t exeio;

	// breaking _t rule for ELFIO compatibility
	class dump_t
	{
		public:
		static void header(std::ostream& stream, EXEIO::exeio_t &to_print) { to_print.dump_header(stream); }
		static void section_headers(std::ostream& stream, EXEIO::exeio_t &to_print) { to_print.dump_section_headers(stream); }
	};
	
	typedef dump_t dump;

}
#endif
