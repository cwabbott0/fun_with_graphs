#include "hash_set.h"
#include <stdlib.h>


static _bucket_t *bucket_create(void *ptr)
{
	_bucket_t *ret = malloc(sizeof(_bucket_t));
	if (!ret)
		return NULL;
	
	ret->ptr = ptr;
	ret->next = NULL;
	return ret;
}

static void bucket_delete(_bucket_t *bucket, delete_func delete)
{
	while (bucket != NULL)
	{
		_bucket_t *old_bucket = bucket;
		bucket = bucket->next;
		delete(old_bucket->ptr);
		free(old_bucket);
	}
}

static bool bucket_add(_bucket_t *bucket, void *ptr, compare_func compare)
{
	while (true)
	{
		if (compare(bucket->ptr, ptr))
			return true;
		if (!bucket->next)
			// This is the last bucket in the list,
			// break early so we can add to the list
			break;
		bucket = bucket->next;
	}
	
	bucket->next = bucket_create(ptr);
	if (!bucket->next)
		return false;
	return true;
}


static bool bucket_contains(_bucket_t* bucket, void* ptr, compare_func compare)
{
	while (bucket)
	{
		if (compare(bucket->ptr, ptr))
			return true;
		bucket = bucket->next;
	}
	return false;
}

hash_set *hash_set_create(unsigned table_size, hash_func hash,
							compare_func compare, delete_func delete)
{
	hash_set *set = malloc(sizeof(hash_set));
	if(!set)
		return NULL;
	set->num_buckets = table_size;
	set->buckets = calloc(table_size, sizeof(_bucket_t*));
	if(!set->buckets)
	{
		free(set);
		return NULL;
	}
	set->size = 0;
	set->hash = hash;
	set->compare = compare;
	set->delete = delete;
	return set;
}

void hash_set_delete(hash_set* set)
{
	unsigned i;
	for (i = 0; i < set->num_buckets; i++)
		bucket_delete(set->buckets[i], set->delete);
	free(set->buckets);
	free(set);
}

bool hash_set_add(hash_set *set, void *elem)
{
	unsigned long hash = set->hash(elem) % set->num_buckets;
	if(!set->buckets[hash])
	{
		set->buckets[hash] = bucket_create(elem);
		if(!set->buckets[hash])
			return false;
	}
	else
	{
		if(!bucket_add(set->buckets[hash], elem, set->compare))
			return false;
	}
	set->size++;
	return true;
}

bool hash_set_contains(hash_set *set, void *ptr)
{
	unsigned long hash = set->hash(ptr) % set->num_buckets;
	return bucket_contains(set->buckets[hash], ptr, set->compare);
}

unsigned hash_set_size(hash_set *set)
{
	return set->size;
}