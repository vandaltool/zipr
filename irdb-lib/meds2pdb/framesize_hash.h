#ifndef framesize_hash_h
#define framesize_hash_h

#include "meds_all.h"


extern Hashtable *framesizes_hash;
struct framesize_hash_key
{
        int pc;
};
typedef struct framesize_hash_key framesize_hash_key_t;

struct framesize_hash_value
{
	int frame_size;
	int var_sized;
};
typedef struct framesize_hash_value framesize_hash_value_t;

long framesizes_compute_hash(void* key1);

long framesizes_key_compare(void* key1, void* key2);

void set_frame_size(int pc, int frame_size);
int is_var_sized_frame(int pc);


#endif
