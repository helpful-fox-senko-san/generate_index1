#ifndef SQPACK_H
#define SQPACK_H

#include <stdint.h>
#include <stdio.h>

#include "thirdparty/sha1.h"

typedef struct SqPackHeader
{
	char Magic[8];
	uint32_t PlatformId;
	uint32_t HeaderSize; // Expect 0x400
	uint32_t Version;
	uint32_t Type; // 2 for data files, 1 for index files

	char _padding1[0x400 - 24 - 64];

	char HeaderSHA1[64];
} SqPackHeader;

typedef struct SqPackIndexHeader
{
	uint32_t HeaderSize; // Expect 0x400
	uint32_t IndexVersion; // Always 1?

	uint32_t IndexDataOffset; // I guess this would almost always be 0x800
	uint32_t IndexDataSize;
	char IndexDataSHA1[64];

	uint32_t NumDatFiles;

	uint32_t SynonymDataOffset;
	uint32_t SynonymDataSize;
	char SynonymDataSHA1[64];

	uint32_t Seg3Offset;
	uint32_t Seg3Size;
	char Seg3SHA1[64];

	uint32_t Seg4Offset;
	uint32_t Seg4Size;
	char Seg4SHA1[64];

	uint32_t IndexType; // 0 (Index1) or 2 (Index2)

	char _padding1[0x400 - 16 - 4*72 - 64];

	char IndexHeaderSHA1[64];
} SqPackIndexHeader;

typedef struct Index1PathIndexRecord
{
	uint32_t FileHash;
	uint32_t FolderHash;
	uint32_t Data;
	uint32_t _padding1;
} Index1PathIndexRecord;

typedef struct Index2PathIndexRecord
{
	uint32_t PathHash;
	uint32_t Data;
} Index2PathIndexRecord;

#if __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(SqPackHeader) == 0x400, "SqPackHeader wrong size");
_Static_assert(sizeof(SqPackIndexHeader) == 0x400, "SqPackIndexHeader wrong size");
_Static_assert(sizeof(Index1PathIndexRecord) == 16, "Index1PathIndexRecord wrong size");
_Static_assert(sizeof(Index2PathIndexRecord) == 8, "Index2PathIndexRecord wrong size");
#endif

void read_SqPackHeader(SqPackHeader* out, FILE* fh);
void write_SqPackHeader(const SqPackHeader* data, FILE* fh);
void read_SqPackIndexHeader(SqPackIndexHeader* out, FILE* fh);
void write_SqPackIndexHeader(const SqPackIndexHeader* data, FILE* fh);

// Writes Index1 style (file/folder) index out to the file
void write_index1_data(FILE* fh, SHA1_CTX* sha1ctx);

// Writes Index2 style (fullpath) index out to the file
void write_index2_data(FILE* fh, SHA1_CTX* sha1ctx);

#endif // SQPACK_H
