#ifndef DB_H
#define DB_H

#include <stddef.h>
#include <stdint.h>

typedef struct DualHash
{
	uint32_t FileHash;
	uint32_t FolderHash;
} DualHash;

typedef struct HashlistRecord
{
	uint32_t FullHash;
	uint32_t FileHash;
	uint32_t FolderHash;
	uint32_t IndexId;
	//char FullPathSHA1[20]; // If we only need to compare 2 paths keeping a hash is simpler
} HashlistRecord;

extern int collisionsCount;
extern int (*collisionResolver)(uint32_t indexId, HashlistRecord* first, HashlistRecord* second);

DualHash db_fullhash_to_dualhash(uint32_t fullHash, uint32_t indexId, char* fullPathOptional);
uint32_t db_dualhash_to_fullhash(DualHash dualHash);

// This loads the hash conversion table from hashlist.db in memory for querying
_Bool db_load_all_paths();

#endif // DB_H
