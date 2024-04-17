#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

// 64 - 32768 bytes... If a single hash bucket ever grows larger than this something is going wrong
#if UINTPTR_MAX > 0xFFFFFFFF
#define INITIAL_BUCKET_SIZE (1U<<3)
#else
#define INITIAL_BUCKET_SIZE (1U<<4)
#endif
#define MAX_BUCKET_SIZE (1U<<12)

HashTable* hash_table_alloc(unsigned prefix_bits)
{
	assert(prefix_bits > 0 && prefix_bits < 32);
	unsigned len = (1 << prefix_bits);
	HashTable* table = calloc(1, sizeof(HashTable) + sizeof(HashBucket*) * len);
	assert(table != NULL);
	table->len = len;
	table->prefix_bits = prefix_bits;
	return table;
}

static void hash_table_free_buckets(HashTable* table)
{
	for (unsigned i = 0; i < table->len; ++i)
	{
		if (table->buckets[i] != NULL)
			free(table->buckets[i]);
	}
}

void hash_table_clear(HashTable* table)
{
	hash_table_free_buckets(table);
	memset(&table->buckets[0], 0, sizeof(HashBucket*) * table->len);
}

void hash_table_free(HashTable* table)
{
	hash_table_free_buckets(table);
	free(table);
}

uint32_t hash_get_data(HashTable* table, uint32_t hash)
{
	HashBucket** bucket_ptr = hash_locate_bucket(table, hash);
	BucketEntry* entry = hash_bucket_find_entry(bucket_ptr, hash);
	return entry->data;
}

pair_uint32_t hash_get_data2(HashTable* table, uint32_t hash)
{
	HashBucket** bucket_ptr = hash_locate_bucket(table, hash);
	BucketEntry* entry = hash_bucket_find_entry(bucket_ptr, hash);
	if (entry->key == HASH_RECORD_INVALID)
		return ((struct pair_uint32_t){HASH_RECORD_INVALID, HASH_RECORD_INVALID});
	else if ((entry+1)->key != hash)
		return ((struct pair_uint32_t){entry->data, HASH_RECORD_INVALID});
	else
		return ((struct pair_uint32_t){entry->data, (entry+1)->data});
}

void hash_put_data(HashTable* table, uint32_t hash, uint32_t data)
{
	HashBucket** bucket_ptr = hash_locate_bucket(table, hash);
	BucketEntry* entry = hash_bucket_find_entry(bucket_ptr, hash);
	hash_bucket_entry_put_data(bucket_ptr, entry, hash, data);
}

HashBucket** hash_locate_bucket(HashTable* table, uint32_t hash)
{
	size_t prefix = hash >> (32 - table->prefix_bits);

	if (table->buckets[prefix] == NULL)
	{
		table->buckets[prefix] = malloc(sizeof(HashBucket) + sizeof(BucketEntry*) * INITIAL_BUCKET_SIZE);
		assert(table->buckets[prefix] != NULL);
		table->buckets[prefix]->len = INITIAL_BUCKET_SIZE;
		table->buckets[prefix]->entries[0].key = HASH_RECORD_INVALID;
		table->buckets[prefix]->entries[0].data = HASH_RECORD_INVALID;
	}

	return &table->buckets[prefix];
}

BucketEntry* hash_bucket_find_entry(HashBucket** bucket_ptr, uint32_t hash)
{
	HashBucket* bucket = *bucket_ptr;
	BucketEntry* entry = &bucket->entries[0];

	while (entry->key != hash && entry->key != HASH_RECORD_INVALID)
		++entry;

	return entry;
}

BucketEntry* hash_bucket_entry_put_data(HashBucket** bucket_ptr, BucketEntry* entry, uint32_t hash, uint32_t data)
{
	HashBucket* bucket = *bucket_ptr;
	unsigned len = bucket->len;

	// Entry must point within the bucket
	assert(entry >= &(*bucket_ptr)->entries[0]);
	assert(entry <= &(*bucket_ptr)->entries[len - 1]);

	// Table is full (entry points to the end of the bucket entry array)
	if (bucket->entries[len - 1].key == HASH_RECORD_INVALID)
	{
		fflush(stdout);
		unsigned idx = (entry - &bucket->entries[0]);
		len = (len << 1);
		assert(len <= MAX_BUCKET_SIZE);
		bucket = realloc(bucket, sizeof(HashBucket) + sizeof(BucketEntry*) * len);
		assert(bucket != NULL);
		*bucket_ptr = bucket;
		bucket->len = len;
		entry = &bucket->entries[idx];
	}

	// We're appending rather than overwriting
	if (entry->key == HASH_RECORD_INVALID)
	{
		entry->key = hash;
		entry->data = data;

		// Move terminator record forward one index
		(entry+1)->key = HASH_RECORD_INVALID;
		(entry+1)->data = HASH_RECORD_INVALID;

		return entry;
	}
	else
	{
		//fprintf(stderr, "HASH COLLISION: %08X\n", hash);
		//fflush(stdout);

		// Inserting a duplicate key, first seek to the end of any matching hashes, then shift the entire table forwards
		while (entry->key == hash)
			++entry;

		int tailBytes = ((&bucket->entries[len - 1]) - entry) * sizeof(BucketEntry*);
		memmove(entry + 1, entry, tailBytes);
		entry->key = hash;
		entry->data = data;
		return entry;
	}
}
