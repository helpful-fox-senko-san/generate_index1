#include "index_conv.h"

#include <stdio.h>

#include "hash.h"
#include "hashlist_db.h"
#include "index.h"

int generatedCount = 0;
int skippedCount = 0;

void generate_index1_data(uint32_t indexId)
{
	HashTable* table = pathTable;

	for (unsigned tableBucketIdx = 0; tableBucketIdx < table->len; ++tableBucketIdx)
	{
		HashBucket* bucket = table->buckets[tableBucketIdx];
		if (bucket == NULL)
			continue;
		for (BucketEntry* entry = &bucket->entries[0]; entry->data != HASH_RECORD_INVALID; ++entry)
		{
			uint32_t fullHash = entry->key;
			uint32_t data = entry->data;

			int cc = collisionsCount;

			DualHash dualHash = db_fullhash_to_dualhash(fullHash, indexId, NULL);

			// Don't care about ex5 collisions
			if (collisionsCount != cc && (indexId & 0x00FF00) == 0)
			{
				printf(" !! collided: FullHash %08x\n", fullHash, dualHash.FileHash, dualHash.FolderHash);
				db_print_colliding_fullhash(fullHash, indexId);
			}

			//printf("lookup: { FileHash=%08x, FolderHash=%08x } => FullHash %08x\n", fullHash, dualHash.FileHash, dualHash.FolderHash);

			if (dualHash.FileHash == HASH_RECORD_INVALID)
			{
				++skippedCount;
			}
			else
			{
				HashTable* fileTable = get_folder_file_table(dualHash.FolderHash);
				hash_put_data(fileTable, dualHash.FileHash, data);
				++generatedCount;
			}
		}
	}
}

// XXX: This isn't very good because it will not generate synonym records in case of a collision
void generate_index2_data()
{
	for (unsigned folderIdx = 0; folderIdx < folderCount; ++folderIdx)
	{
		uint32_t folderHash = folderHashes[folderIdx];
		HashTable* table = folderFileLists[folderIdx];

		for (unsigned tableBucketIdx = 0; tableBucketIdx < table->len; ++tableBucketIdx)
		{
			HashBucket* bucket = table->buckets[tableBucketIdx];
			if (bucket == NULL)
				continue;
			for (BucketEntry* entry = &bucket->entries[0]; entry->data != HASH_RECORD_INVALID; ++entry)
			{
				uint32_t fileHash = entry->key;
				uint32_t data = entry->data;
				uint32_t fullHash = db_dualhash_to_fullhash((DualHash){ .FileHash = fileHash, .FolderHash = folderHash });

				//printf("lookup: { FileHash=%08x, FolderHash=%08x } => FullHash %08x\n", fileHash, folderHash, fullHash);

				if (fullHash == HASH_RECORD_INVALID)
				{
					++skippedCount;
				}
				else
				{
					hash_put_data(pathTable, fullHash, data);
					++generatedCount;
				}
			}
		}
	}
}
