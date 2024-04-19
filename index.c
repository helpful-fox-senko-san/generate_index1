#include "index.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "util.h"

// Data stored in folderTable is an index in to folderFileLists, which stores per-folder file tables
HashTable* folderTable;
HashTable* pathTable;

HashTable* folderFileLists[MAX_FOLDERS];
uint32_t folderHashes[MAX_FOLDERS];
uint32_t folderCount = 0;

void index_startup(void)
{
	folderTable = hash_table_alloc_16k();
	pathTable = hash_table_alloc_64k();
}

HashTable* get_folder_file_table(uint32_t folderHash)
{
	// Playing with buckets to speed up check-and-insert operations
	HashBucket** bucket = hash_locate_bucket(folderTable, folderHash);
	BucketEntry* entry = hash_bucket_find_entry(bucket, folderHash);

	// Allocate a table for the new folder if its not already known
	if (entry->key == HASH_RECORD_INVALID)
	{
		if (folderCount >= MAX_FOLDERS)
		{
			fprintf(stderr, "Too many folders!\n");
			exit(1);
		}

		entry = hash_bucket_entry_put_data(bucket, entry, folderHash, folderCount);
		folderHashes[folderCount] = folderHash;
		folderFileLists[folderCount] = hash_table_alloc_256();
		++folderCount;
	}

	return folderFileLists[entry->data];
}

void clear_folder_file_table()
{
	for (uint32_t i = 0; i < folderCount; ++i)
	{
		hash_table_free(folderFileLists[i]);
		folderFileLists[i] = NULL;
	}

	folderCount = 0;
}
