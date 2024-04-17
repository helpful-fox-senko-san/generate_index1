#include "sqpack.h"

#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "index.h"
#include "util.h"

void read_SqPackHeader(SqPackHeader* out, FILE* fh)
{
	int result = fread(out, 1, sizeof(SqPackHeader), fh);
	my_assert_f(result == sizeof(SqPackHeader), "Failed to read from input at 0x%x\n", (unsigned)ftell(fh));
	my_assert(memcmp(out->Magic, "SqPack\0", 8) == 0);
	my_assert(out->PlatformId == 0);
	my_assert(out->HeaderSize == sizeof(SqPackHeader));
	my_assert(out->Version == 1);
	my_assert(out->Type == 2);
}

void write_SqPackHeader(const SqPackHeader* data, FILE* fh)
{
	int result = fwrite(data, 1, sizeof(SqPackHeader), fh);
	my_assert_f(result == sizeof(SqPackHeader), "Failed to write to output at 0x%x\n", (unsigned)ftell(fh));
}

void read_SqPackIndexHeader(SqPackIndexHeader* out, FILE* fh)
{
	int result = fread(out, 1, sizeof(SqPackIndexHeader), fh);
	my_assert_f(result == sizeof(SqPackIndexHeader), "Failed to read from input at 0x%x\n", (unsigned)ftell(fh));
	my_assert(out->HeaderSize == sizeof(SqPackIndexHeader));
	my_assert(out->IndexVersion == 1);
	my_assert(out->NumDatFiles < 999);
}

void write_SqPackIndexHeader(const SqPackIndexHeader* data, FILE* fh)
{
	int result = fwrite(data, 1, sizeof(SqPackIndexHeader), fh);
	my_assert_f(result == sizeof(SqPackIndexHeader), "Failed to write to output at 0x%x\n", (unsigned)ftell(fh));
}

void write_index1_data(FILE* fh, SHA1_CTX* sha1ctx)
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
				Index1PathIndexRecord record = {};
				record.FolderHash = folderHash;
				record.FileHash = entry->key;
				record.Data = entry->data;
				int result = fwrite(&record, 1, sizeof record, fh);
				my_assert(result == sizeof record);
				SHA1Update(sha1ctx, (unsigned char*)&record, sizeof record);
				//printf("Generated: { FileHash=%08x, FolderHash=%08x, Data=%08x }\n", record.FileHash, record.FolderHash, record.Data);
			}
		}
	}
}

void write_index2_data(FILE* fh, SHA1_CTX* sha1ctx)
{
	HashTable* table = pathTable;

	for (unsigned tableBucketIdx = 0; tableBucketIdx < table->len; ++tableBucketIdx)
	{
		HashBucket* bucket = table->buckets[tableBucketIdx];
		if (bucket == NULL)
			continue;
		for (BucketEntry* entry = &bucket->entries[0]; entry->data != HASH_RECORD_INVALID; ++entry)
		{
			Index2PathIndexRecord record = {};
			record.PathHash = entry->key;
			record.Data = entry->data;
			int result = fwrite(&record, 1, sizeof record, fh);
			my_assert(result == sizeof record);
			SHA1Update(sha1ctx, (unsigned char*)&record, sizeof record);
			//printf("Generated: { PathHash=%08x, Data=%08x }\n", record.PathHash, record.Data);
		}
	}
}
