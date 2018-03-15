

#define _GNU_SOURCE
#include <link.h>
#include <stddef.h>
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>

struct auditstate {
    uintptr_t cookie;
    unsigned int bindflags;
};


typedef struct {
    pthread_mutex_t mutex;
} __rtld_lock_recursive_t;


struct unique_sym_table {
    __rtld_lock_recursive_t lock;
    struct unique_sym *entries;
    size_t size;
    size_t n_elements;
    void (*free)(void *);
};

struct my_link_map {
    ElfW(Addr) l_addr;
    char *l_name;
    ElfW(Dyn) *l_ld;
    struct my_link_map *l_next;
    struct my_link_map *l_prev;
    struct my_link_map *l_real;
    Lmid_t l_ns;
    struct libname_list *l_libname;
    ElfW(Dyn) *l_info[76];
    const ElfW(Phdr) *l_phdr;
    ElfW(Addr) l_entry;
    ElfW(Half) l_phnum;
    ElfW(Half) l_ldnum;
};

struct link_namespaces {
    struct my_link_map *_ns_loaded;
    unsigned int _ns_nloaded;
    struct r_scope_elem *_ns_main_searchlist;
    size_t _ns_global_scope_alloc;
    struct unique_sym_table _ns_unique_sym_table;
    struct r_debug _ns_debug;
};

typedef unsigned long long hp_timing_t;

struct my_rtld_global {
    struct link_namespaces _dl_ns[16];
#if 0
    size_t _dl_nns;
    __rtld_lock_recursive_t _dl_load_lock;
    __rtld_lock_recursive_t _dl_load_write_lock;
    unsigned long long _dl_load_adds;
    struct link_map *_dl_initfirst;
    hp_timing_t _dl_cpuclock_offset;
    struct link_map *_dl_profile_map;
    unsigned long _dl_num_relocations;
    unsigned long _dl_num_cache_relocations;
    struct r_search_path_elem *_dl_all_dirs;
    void **(*_dl_error_catch_tsd)(void);
    struct link_map _dl_rtld_map;
    struct auditstate audit_data[16];
    void (*_dl_rtld_lock_recursive)(void *);
    void (*_dl_rtld_unlock_recursive)(void *);
    int (*_dl_make_stack_executable_hook)(void **);
    Elf64_Word _dl_stack_flags;
    _Bool _dl_tls_dtv_gaps;
    size_t _dl_tls_max_dtv_idx;
    struct dtv_slotinfo_list *_dl_tls_dtv_slotinfo_list;
    size_t _dl_tls_static_nelem;
    size_t _dl_tls_static_size;
    size_t _dl_tls_static_used;
    size_t _dl_tls_static_align;
    void *_dl_initial_dtv;
    size_t _dl_tls_generation;
    void (*_dl_init_static_tls)(struct link_map *);
    void (*_dl_wait_lookup_done)(void);
    struct dl_scope_free_list *_dl_scope_free_list;
#endif
};

extern struct my_rtld_global _rtld_global;


#define GL(a) _rtld_global._##a
int
zestcfi__dl_iterate_phdr (int (*callback) (struct dl_phdr_info *info,
                                    size_t size, void *data), void *data)
{
  struct my_link_map *l=NULL;
  struct dl_phdr_info info;
  int ret = 0;

  /* Make sure nobody modifies the list of loaded objects.  */
//  __rtld_lock_lock_recursive (GL(dl_load_write_lock));
//  __libc_cleanup_push (cancel_handler, NULL);

  /* We have to determine the namespace of the caller since this determines
     which namespace is reported.  */
  //size_t nloaded = GL(dl_ns)[0]._ns_nloaded;
  Lmid_t ns = 0;
#ifdef SHARED
  const void *caller = RETURN_ADDRESS (0);
  for (Lmid_t cnt = GL(dl_nns) - 1; cnt > 0; --cnt)
    for (struct my_link_map *l = GL(dl_ns)[cnt]._ns_loaded; l; l = l->l_next)
      {
        /* We have to count the total number of loaded objects.  */
        nloaded += GL(dl_ns)[cnt]._ns_nloaded;

        if (caller >= (const void *) l->l_map_start
            && caller < (const void *) l->l_map_end
            && (l->l_contiguous
                || _dl_addr_inside_object (l, (ElfW(Addr)) caller)))
          ns = cnt;
      }
#endif

  for (l = GL(dl_ns)[ns]._ns_loaded; l != NULL; l = l->l_next)
    {
      info.dlpi_addr = l->l_real->l_addr;
      info.dlpi_name = l->l_real->l_name;
      info.dlpi_phdr = l->l_real->l_phdr;
      info.dlpi_phnum = l->l_real->l_phnum;
//      info.dlpi_adds = GL(dl_load_adds);
//      info.dlpi_subs = GL(dl_load_adds) - nloaded;
//      info.dlpi_tls_data = NULL;
//      info.dlpi_tls_modid = l->l_real->l_tls_modid;
//      if (info.dlpi_tls_modid != 0)
//        info.dlpi_tls_data = GLRO(dl_tls_get_addr_soft) (l->l_real);
      ret = callback (&info, sizeof (struct dl_phdr_info), data);
      if (ret)
        break;
    }

  /* Release the lock.  */
//  __libc_cleanup_pop (0);
//  __rtld_lock_unlock_recursive (GL(dl_load_write_lock));

  return ret;
}

#ifdef TEST

int test_callback (struct dl_phdr_info *info, size_t size, void *data)
{
	printf("In test_callback with info=%p, module=%s\n", info, info->dlpi_name);
	return 0;
}

main()
{
	printf("My impl:\t");
	zestcfi__dl_iterate_phdr (test_callback, NULL);
	printf("real impl:\t");
	dl_iterate_phdr (test_callback, NULL);
}
#endif
