#ifndef HASH_H
#define HASH_H

#include <stdint.h>

// This value is invalid for both the hash and data values
#define HASH_RECORD_INVALID ((uint32_t)0xFFFFFFFF)

typedef struct BucketEntry { uint32_t key; uint32_t data; } BucketEntry;

typedef struct HashBucket
{
	unsigned len;
	BucketEntry entries[0];
} HashBucket;

typedef struct HashTable
{
	unsigned len;
	unsigned prefix_bits;
	HashBucket* buckets[0];
} HashTable;

#define hash_table_alloc_16() hash_table_alloc(4)
#define hash_table_alloc_64() hash_table_alloc(6)
#define hash_table_alloc_256() hash_table_alloc(8)
#define hash_table_alloc_4k() hash_table_alloc(12)
#define hash_table_alloc_16k() hash_table_alloc(14)
#define hash_table_alloc_64k() hash_table_alloc(16)

HashTable* hash_table_alloc(unsigned prefix_bits);
void hash_table_clear(HashTable* table);
void hash_table_free(HashTable* table);

typedef struct pair_uint32_t {
	uint32_t first;
	uint32_t second;
} pair_uint32_t;

uint32_t hash_get_data(HashTable* table, uint32_t hash);
// Get up to 2 values with the same hash
pair_uint32_t hash_get_data2(HashTable* table, uint32_t hash);
void hash_put_data(HashTable* table, uint32_t hash, uint32_t data);

// HashBucket** parameters should always point to a HashBucket* element inside of a HashTable

HashBucket** hash_locate_bucket(HashTable* table, uint32_t hash);
// If the hash isn't present in the bucket, the result will point at the end of the bucket (sentinel/terminator record)
BucketEntry* hash_bucket_find_entry(HashBucket** bucket, uint32_t hash);
uint32_t hash_bucket_get_data(HashBucket** bucket, uint32_t hash);

// entry should point within the given bucket
// potentially returns a new address for entry if the bucket had to grow
BucketEntry* hash_bucket_entry_put_data(HashBucket** bucket, BucketEntry* entry, uint32_t hash, uint32_t data);


#endif // HASH_H
