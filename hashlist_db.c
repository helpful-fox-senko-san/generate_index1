#include "hashlist_db.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "sha1.h"
#include "sqlite3.h"

static HashlistRecord* dbRecords;
static int dbRecordCount = 0;

// Folder index points to the first matching record in the db record array (which is sorted by FolderHash)
DEFINE_STATIC_HASH_TABLE_16K(dbFolderIndex)
// Path index points to an index for a full path hash, however due to hash collisions, two entries may be present
//   (more than two is possible in theory but hasn't happened yet)
DEFINE_STATIC_HASH_TABLE_64K(dbPathIndex)

static void db_allocate(int count)
{
	dbRecords = malloc(sizeof(HashlistRecord) * count);
	assert(dbRecords != NULL);
	dbRecordCount = count;
}

static void db_store(int idx, uint32_t fullHash, uint32_t folderHash, uint32_t fileHash, uint32_t indexId, const char* fullPath)
{
	assert(idx < dbRecordCount);
	HashlistRecord* record = &dbRecords[idx];
	record->FullHash = fullHash;
	record->FileHash = fileHash;
	record->FolderHash = folderHash;
	record->IndexId = indexId;
	//record->FullPathString = strdup(fullPath);
	//SHA1(record->FullPathSHA1, fullPath, strlen(fullPath));

	HashBucket** bucket = hash_locate_bucket(dbFolderIndex, folderHash);
	BucketEntry* entry = hash_bucket_find_entry(bucket, folderHash);

	if (entry->key == HASH_RECORD_INVALID)
		hash_bucket_entry_put_data(bucket, entry, folderHash, idx);

	hash_put_data(dbPathIndex, fullHash, idx);
}

int collisionsCount = 0;
int (*collisionResolver)(uint32_t indexId, HashlistRecord* first, HashlistRecord* second) = NULL;

// XXX: This may return the wrong thing because of collisions
DualHash db_fullhash_to_dualhash(uint32_t fullHash, uint32_t indexId, char* fullPathOptional)
{
	DualHash result = { HASH_RECORD_INVALID, HASH_RECORD_INVALID };

	// Retrieve up to two values inserted in to the hash table with the same hash
	pair_uint32_t idxpair = hash_get_data2(dbPathIndex, fullHash);

	if (idxpair.first == HASH_RECORD_INVALID)
		return result;

	HashlistRecord* record = &dbRecords[idxpair.first];

	// There was a second record, try to disambiguate
	if (idxpair.second != HASH_RECORD_INVALID)
	{
		HashlistRecord* first = &dbRecords[idxpair.first];
		HashlistRecord* second = &dbRecords[idxpair.second];

		if (first->IndexId != second->IndexId && first->IndexId == indexId)
		{
			record = first;
		}
		else if (first->IndexId != second->IndexId && second->IndexId == indexId)
		{
			record = second;
		}
#if 0 // Not implemented
		else if (fullPathOptional != NULL)
		{
			char pathsha1[20];
			SHA1(pathsha1, fullPathOptional, strlen(fullPathOptional));

			if (memcmp(first->FullPathSHA1, pathsha1, 20) == 0)
			{
				record = first;
			}
			else if (memcmp(second->FullPathSHA1, pathsha1, 20) == 0)
			{
				record = second;
			}
			else
			{
				// Provided file path was no help in disambiguation...
				++collisionsCount;
			}
		}
#endif
		else
		{
			int resolution = 0;

			// Pluggable resolver for hard-coded resolutions
			if (collisionResolver != NULL)
				resolution = collisionResolver(indexId, first, second);

			switch (resolution)
			{
				case 1:
					record = first;
					break;

				case 2:
					record = second;
					break;

				case 0:
				default:
					++collisionsCount;
			}

		}
	}

	result.FileHash = record->FileHash;
	result.FolderHash = record->FolderHash;

	return result;
}

uint32_t db_dualhash_to_fullhash(DualHash dualHash)
{
	uint32_t idx = hash_get_data(dbFolderIndex, dualHash.FolderHash);

	if (idx == HASH_RECORD_INVALID)
		return HASH_RECORD_INVALID;

	// Scan linearly for the file hash
	HashlistRecord* record = &dbRecords[idx];
	HashlistRecord* recordEnd = &dbRecords[dbRecordCount];

	for (; record < recordEnd && record->FolderHash == dualHash.FolderHash; ++record)
	{
		if (record->FileHash == dualHash.FileHash)
			return record->FullHash;
	}

	return HASH_RECORD_INVALID;
}

#define checked_fread(buf, sz, n, fh) \
	result = fread((buf), sz, (n), (fh)); \
	if (result != (int)(n)) \
		return 0;

#define checked_fwrite(buf, sz, n, fh) \
	result = fwrite((buf), sz, (n), (fh)); \
	if (result != (int)(n)) \
		return 0;

static void db_error(const char* errmsg)
{
	fprintf(stderr, "%s\n", errmsg);
	exit(1);
}

// --- Caching ---

static int db_save_hashtable(HashTable* table, FILE* fh)
{
	int result;
	checked_fwrite(&table->prefix_bits, 1, sizeof table->prefix_bits, fh);
	checked_fwrite(&table->len, 1, sizeof table->len, fh);

	unsigned zero = 0;

	for (unsigned i = 0; i < table->len; ++i)
	{
		HashBucket* bucket = table->buckets[i];
		if (bucket == NULL)
		{
			checked_fwrite(&zero, 1, sizeof zero, fh);
			continue;
		}
		checked_fwrite(&bucket->len, 1, sizeof bucket->len, fh);
		checked_fwrite(&bucket->entries[0], 1, sizeof(BucketEntry) * bucket->len, fh);
	}

	return 1;
}

static int db_load_hashtable(HashTable* table, FILE* fh)
{
	int result;
	unsigned prefix_bits, len;
	checked_fread(&prefix_bits, 1, sizeof prefix_bits, fh);
	checked_fread(&len, 1, sizeof len, fh);

	if (prefix_bits != table->prefix_bits)
		return 0;

	if (len != table->len)
		return 0;

	for (unsigned i = 0; i < table->len; ++i)
	{
		HashBucket** bucket_ptr = &table->buckets[i];
		unsigned len;
		checked_fread(&len, 1, sizeof len, fh);
		if (len == 0)
		{
			*bucket_ptr = 0;
			continue;
		}
		HashBucket* bucket = malloc(sizeof(HashBucket) + sizeof(BucketEntry) * len);
		assert(bucket != NULL);
		bucket->len = len;
		checked_fread(&bucket->entries[0], 1, sizeof(BucketEntry) * bucket->len, fh);
		*bucket_ptr = bucket;
	}

	// Assume success
	return 1;
}

// SQLite query takes like 10 seconds, so cache the data once its loaded
static int db_save_cache()
{
	FILE* fh = fopen("hashlist.cache", "wb");
	int result;
	if (!fh)
		return 0;
	unsigned Version = 1000;
	result = fwrite(&Version, 1, sizeof Version, fh);
	Version = sizeof(long);
	result = fwrite(&Version, 1, sizeof Version, fh);
	Version = sizeof(HashlistRecord);
	result = fwrite(&Version, 1, sizeof Version, fh);
	Version = sizeof(HashTable);
	result = fwrite(&Version, 1, sizeof Version, fh);
	Version = sizeof(HashBucket);
	result = fwrite(&Version, 1, sizeof Version, fh);
	Version = sizeof(BucketEntry);
	result = fwrite(&Version, 1, sizeof Version, fh);
	checked_fwrite(&dbRecordCount, 1, sizeof dbRecordCount, fh);
	checked_fwrite(dbRecords, 1, sizeof(HashlistRecord) * dbRecordCount, fh);

	if (!db_save_hashtable(dbFolderIndex, fh))
		return 0;

	if (!db_save_hashtable(dbPathIndex, fh))
		return 0;

	return 1;
}

static int db_load_cache()
{
	FILE* fh = fopen("hashlist.cache", "rb");
	int result;
	if (!fh)
		return 0;
	unsigned Version;
	checked_fread(&Version, 1, sizeof Version, fh);
	if (Version != 1000)
		return 0;
	checked_fread(&Version, 1, sizeof Version, fh);
	if (Version != sizeof(long))
		return 0;
	checked_fread(&Version, 1, sizeof Version, fh);
	if (Version != sizeof(HashlistRecord))
		return 0;
	checked_fread(&Version, 1, sizeof Version, fh);
	if (Version != sizeof(HashTable))
		return 0;
	checked_fread(&Version, 1, sizeof Version, fh);
	if (Version != sizeof(HashBucket))
		return 0;
	checked_fread(&Version, 1, sizeof Version, fh);
	if (Version != sizeof(BucketEntry))
		return 0;
	checked_fread(&dbRecordCount, 1, sizeof dbRecordCount, fh);
	dbRecords = malloc(sizeof(HashlistRecord) * dbRecordCount);
	assert(dbRecords != NULL);
	result = fread(dbRecords, 1, sizeof(HashlistRecord) * dbRecordCount, fh);

	if (!db_load_hashtable(dbFolderIndex, fh))
		return 0;

	if (!db_load_hashtable(dbPathIndex, fh))
		return 0;

	return 1;
}

// --- API ---

_Bool db_load_all_paths()
{
	if (db_load_cache())
	{
		printf("Loaded %d records from hashlist.cache\n", dbRecords);
		return 1;
	}

	int result;
	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;

	result = sqlite3_open_v2("hashlist.db", &db, SQLITE_OPEN_READONLY, NULL);

	if (result != SQLITE_OK)
		db_error("Could not open hashlist.db");

	int count = 0;

	printf("Querying hashlist.db ... ");
	fflush(stdout);

	// Query 1
	{
		const char query[] = "SELECT COUNT(1) FROM fullpaths";

		if (sqlite3_prepare_v2(db, query, sizeof query - 1, &stmt, NULL) != SQLITE_OK)
			db_error(sqlite3_errmsg(db));

		while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			count = (unsigned)sqlite3_column_int(stmt, 0);
			db_allocate(count);
		}

		assert(result == SQLITE_DONE);

		result = sqlite3_finalize(stmt);
		assert(result == SQLITE_OK);
	}

	// Query 2
	{
		//const char query[] = "SELECT fullhash, folderhash, filehash FROM fullpaths ORDER BY folderhash ASC, filehash ASC";

		const char query[] = "\
SELECT fullhash, folderhash, filehash, indexid, folders.text || '/' || filenames.text \
FROM fullpaths \
LEFT JOIN folders ON folders.id = fullpaths.folder \
LEFT JOIN filenames ON filenames.id = fullpaths.file \
ORDER BY folderhash ASC, filehash ASC";

		if (sqlite3_prepare_v2(db, query, sizeof query - 1, &stmt, NULL) != SQLITE_OK)
			db_error(sqlite3_errmsg(db));

		int idx = 0;

		while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			if (idx >= count)
				break;

			unsigned fullHash = (unsigned)sqlite3_column_int(stmt, 0);
			unsigned folderHash = (unsigned)sqlite3_column_int(stmt, 1);
			unsigned fileHash = (unsigned)sqlite3_column_int(stmt, 2);
			unsigned indexId = (unsigned)sqlite3_column_int(stmt, 3);
			const char* fullPath = (const char*)sqlite3_column_text(stmt, 4);

			db_store(idx++, fullHash, folderHash, fileHash, indexId, fullPath);
		}

		assert(result == SQLITE_DONE);

		printf("Read %d entries from hashlist.db\n", idx);

		result = sqlite3_finalize(stmt);
		assert(result == SQLITE_OK);
	}

	result = sqlite3_close(db);
	assert(result == SQLITE_OK);

	db_save_cache();
	return 1;
}
