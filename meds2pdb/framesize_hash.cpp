#include "framesize_hash.h"

Hashtable *framesizes_hash=NULL;

long framesizes_compute_hash(void* key1)
{
        framesize_hash_key_t * a_key=(framesize_hash_key_t *)key1;

        return a_key->pc;
}

long framesizes_key_compare(void* key1, void* key2)
{
        framesize_hash_key_t * a_key=(framesize_hash_key_t *)key1;
        framesize_hash_key_t * b_key=(framesize_hash_key_t *)key2;

        return a_key->pc == b_key->pc;
}

void set_frame_size(int pc, int frame_size)
{
	framesize_hash_key_t fshk={pc};
	framesize_hash_value_t *fshv=(framesize_hash_value_t*)Hashtable_get(framesizes_hash, &fshk);

	if(fshv)
	{
		if(fshv->frame_size != frame_size)
		{
			fshv->var_sized=TRUE;
		}
		return;
	}

	framesize_hash_key_t *fshk2=(framesize_hash_key_t*)spri_allocate_type(sizeof(*fshk2));
	framesize_hash_value_t *fshv2=(framesize_hash_value_t*)spri_allocate_type(sizeof(*fshv2));

	*fshk2=fshk;
	fshv2->frame_size=frame_size;
	fshv2->var_sized=FALSE;

	Hashtable_put(framesizes_hash, fshk2, fshv2);

}


int is_var_sized_frame(int pc)
{
	framesize_hash_key_t fshk={pc};
	framesize_hash_value_t *fshv=(framesize_hash_value_t*)Hashtable_get(framesizes_hash, &fshk);

	if(fshv)
	{
		return fshv->var_sized;
	}

	/* if we didn't find it, it's not var-sized yet */
	return FALSE;
}



