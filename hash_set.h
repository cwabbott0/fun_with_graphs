#ifndef __HASH_SET_H__
#define __HASH_SET_H__

#include <stdbool.h>

typedef unsigned long (*hash_func)(void *elem);
typedef bool (*compare_func)(void *elem1, void *elem2);
typedef void (*delete_func)(void *elem);

typedef struct _bucket {
	void *ptr;
	struct _bucket *next;
} _bucket_t;

typedef struct {
	_bucket_t **buckets;
	unsigned num_buckets;
	unsigned size;
	hash_func hash;
	compare_func compare;
	delete_func delete;
} hash_set;

hash_set *hash_set_create(unsigned table_size, hash_func hash,
							compare_func compare, delete_func delete);
void hash_set_delete(hash_set *set);
bool hash_set_add(hash_set *set, void *ptr);
bool hash_set_remove(hash_set *set, void *elem);
bool hash_set_contains(hash_set *set, void *ptr);
unsigned hash_set_size(hash_set *set);

#endif