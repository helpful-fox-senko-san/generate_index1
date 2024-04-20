#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "thirdparty/sha1.h"
#include "hash.h"
#include "hashlist_db.h"
#include "index.h"
#include "index_conv.h"
#include "sqpack.h"
#include "util.h"

static SqPackHeader sqpackHeader;
static SqPackIndexHeader sqpackIndexHeader;

static uint32_t indexId;

static int in_indexType;
static int out_indexType;

static int totalGenerated = 0;

_Bool do_input(const char* in_filename)
{
	FILE* fh = fopen(in_filename, "rb");
	size_t result;

	// Completely missing a file is the only non-critical error
	if (fh == NULL)
		return 0;

	read_SqPackHeader(&sqpackHeader, fh);
	read_SqPackIndexHeader(&sqpackIndexHeader, fh);

	in_indexType = (sqpackIndexHeader.IndexType == 2) ? 2 : 1;
	out_indexType = (in_indexType == 2) ? 1 : 2;

	// TexTools only cares about index data
	// The rest will be thrown away
	printf("Input: %s (format: Index%d)\n", in_filename, in_indexType);
	printf("  IndexDataSize = %d bytes (@0x%x) (SHA1: %s)\n", sqpackIndexHeader.IndexDataSize, sqpackIndexHeader.IndexDataOffset, sha1str(sqpackIndexHeader.IndexDataSHA1));
	//printf("  SynonymDataSize = %d bytes (@0x%x) (SHA1: %s)\n", sqpackIndexHeader.SynonymDataSize, sqpackIndexHeader.SynonymDataOffset, sha1str(sqpackIndexHeader.SynonymDataSHA1));
	//printf("  Seg3Size = %d bytes (@0x%x) (SHA1: %s)\n", sqpackIndexHeader.Seg3Size, sqpackIndexHeader.Seg3Offset, sha1str(sqpackIndexHeader.Seg3SHA1));
	//printf("  Seg4Size = %d bytes (@0x%x) (SHA1: %s)\n", sqpackIndexHeader.Seg4Size, sqpackIndexHeader.Seg4Offset, sha1str(sqpackIndexHeader.Seg4SHA1));

	fseek(fh, sqpackIndexHeader.IndexDataOffset, SEEK_SET);

	if (in_indexType == 2)
	{
		int numRecords = sqpackIndexHeader.IndexDataSize / sizeof(Index2PathIndexRecord);
		int numRecordsRead = 0;

		for (int i = 0; i < numRecords; i += numRecordsRead)
		{
			Index2PathIndexRecord records[1024];
			size_t readSize = sizeof(Index2PathIndexRecord) * umin(numRecords - i, sizeof(records) / sizeof(Index2PathIndexRecord));
			result = fread(&records[0], 1, readSize, fh);
			my_assert_f(result == readSize, "Failed to read from input at 0x%x\n", (unsigned)ftell(fh));

			numRecordsRead = result / sizeof(Index2PathIndexRecord);

			for (int recordIdx = 0; recordIdx < numRecordsRead; ++recordIdx)
			{
				Index2PathIndexRecord* record = &records[recordIdx];

				if (record->Data & 1)
				{
					printf(" !! synonym record: { PathHash=%08x, Data=%08x }\n", record->PathHash, record->Data);
					++skippedCount;
				}
				else
				{
					hash_put_data(pathTable, record->PathHash, record->Data);
					//printf("record: { PathHash=%08x, Data=%08x }\n", record->PathHash, record->Data);
				}
			}
		}
	}
	else
	{
		int numRecords = sqpackIndexHeader.IndexDataSize / sizeof(Index1PathIndexRecord);
		int numRecordsRead = 0;

		for (int i = 0; i < numRecords; i += numRecordsRead)
		{
			Index1PathIndexRecord records[512];
			size_t readSize = sizeof(Index1PathIndexRecord) * umin(numRecords - i, sizeof(records) / sizeof(Index1PathIndexRecord));
			result = fread(&records[0], 1, readSize, fh);
			my_assert_f(result == readSize, "Failed to read from input at 0x%x\n", (unsigned)ftell(fh));

			numRecordsRead = result / sizeof(Index1PathIndexRecord);

			for (int recordIdx = 0; recordIdx < numRecordsRead; ++recordIdx)
			{
				Index1PathIndexRecord* record = &records[recordIdx];
				HashTable* fileTable = get_folder_file_table(record->FolderHash);
				hash_put_data(fileTable, record->FileHash, record->Data);
				//printf("record: { FileHash=%08x, FolderHash=%08x, Data=%08x }\n", record->FileHash, record->FolderHash, record->Data);
			}
		}
	}

	fclose(fh);
	return 1;
}

_Bool do_output(const char* out_filename)
{
	FILE* fh = fopen(out_filename, "wb");
	size_t result;

	my_assert_f(fh != NULL, "Failed to open output file: %s\n", out_filename);

	// All index files have an identical header
	write_SqPackHeader(&sqpackHeader, fh);
	// We write the index header later once we know the final index size and hash

	SHA1_CTX sha1ctx;
	SHA1Init(&sha1ctx);

	fseek(fh, sqpackIndexHeader.IndexDataOffset, SEEK_SET);

	if (out_indexType == 2)
	{
		printf("Writing index2 data...\n");
		write_index2_data(fh, &sha1ctx);
	}
	else
	{
		printf("Writing index1 data...\n");
		write_index1_data(fh, &sha1ctx);
	}

	sqpackIndexHeader.IndexType = (out_indexType == 2) ? 2 : 0;
	sqpackIndexHeader.IndexDataOffset = 0x800;
	uint32_t seg1size = (uint32_t)ftell(fh) - sqpackIndexHeader.IndexDataOffset;
	sqpackIndexHeader.IndexDataSize = seg1size;
	sqpackIndexHeader.SynonymDataOffset = 0x0;
	sqpackIndexHeader.SynonymDataSize = 0;
	sqpackIndexHeader.Seg3Offset = 0x0;
	sqpackIndexHeader.Seg3Size = 0;
	sqpackIndexHeader.Seg4Offset = 0x0;
	sqpackIndexHeader.Seg4Size = 0;

	SHA1Final((unsigned char*)sqpackIndexHeader.IndexDataSHA1, &sha1ctx);

	char nullsha1[20];
	SHA1(nullsha1, "", 0);
	memcpy(sqpackIndexHeader.SynonymDataSHA1, nullsha1, 20);
	memcpy(sqpackIndexHeader.Seg3SHA1, nullsha1, 20);
	memcpy(sqpackIndexHeader.Seg4SHA1, nullsha1, 20);

	SHA1(sqpackIndexHeader.IndexHeaderSHA1, (char*)&sqpackIndexHeader, sizeof sqpackIndexHeader);

	result = fseek(fh, 0x400, SEEK_SET);
	my_assert(result == 0);
	write_SqPackIndexHeader(&sqpackIndexHeader, fh);

	printf("Output: %s (format: Index%d)\n", out_filename, out_indexType);
	printf("  IndexDataSize = %d bytes (@0x%x) (SHA1: %s)\n", sqpackIndexHeader.IndexDataSize, sqpackIndexHeader.IndexDataOffset, sha1str(sqpackIndexHeader.IndexDataSHA1));
	//printf("  SynonymDataSize = %d bytes (@0x%x) (SHA1: %s)\n", sqpackIndexHeader.SynonymDataSize, sqpackIndexHeader.SynonymDataOffset, sha1str(sqpackIndexHeader.SynonymDataSHA1));

	fclose(fh);
	return 1;
}

extern int dtBenchCollisionResolver(uint32_t indexId, HashlistRecord* first, HashlistRecord* second);

void try_convert(uint32_t indexId)
{
	char in_filename[64];
	char out_filename[64];

	const char* ex = "ffxiv";

	if ((indexId & 0x00FF00) == 0x0500)
		ex = "ex5";

	sprintf(in_filename, "sqpack/%s/%06x.win32.index2", ex, indexId);
	sprintf(out_filename, "sqpack/%s/%06x.win32.index", ex, indexId);

	printf("\n=== Converting %s => %s ===\n", in_filename, out_filename);

	// --- I/O ---

	if (!do_input(in_filename))
	{
		printf("Reading failed\n");
		return;
	}

	if (out_indexType == 1)
		generate_index1_data(indexId);
	else
		generate_index2_data();

	if (!do_output(out_filename))
	{
		printf("Writing failed\n");
		return;
	}

	// --- Results ---

	printf("OK\n");
	printf("- Generated %d index entries.\n", generatedCount);
	printf("- Skipped %d index entries.\n", skippedCount);
	if (collisionsCount > 0)
		printf("- Hash collisions caused %d entries to be picked at random.\n", collisionsCount);
	puts("");
}

void cleanup()
{
	clear_folder_file_table();
	hash_table_clear(folderTable);
	hash_table_clear(pathTable);
	totalGenerated += generatedCount;
	generatedCount = 0;
	skippedCount = 0;
	collisionsCount = 0;
}

#define DUMMY_VER "9999.05.00.0000.0002"
#define DUMMY_VER_LEN (sizeof DUMMY_VER - 1)

int main(int argc, char** argv)
{
	// Also write an ffxivgame.ver file :)
	FILE* test_fh = fopen("ffxiv_dx11.exe", "rb");
	FILE* ver_fh = fopen("ffxivgame.ver", "rb");

	if (test_fh == NULL)
	{
		fprintf(stderr, "ffxiv_dx11.exe missing\n");
		fprintf(stderr, "Run from Dawntrail benchmark 'game' directory!\n");
		return EXIT_FAILURE;
	}

	fclose(test_fh);

	// Make sure we're not overwriting a real FFXIV installation
	if (ver_fh != NULL)
	{
		char verbuf[DUMMY_VER_LEN + 1];
		size_t result = fread(&verbuf, 1, sizeof verbuf - 1, ver_fh);
		if (result != (sizeof verbuf - 1) || memcmp(verbuf, DUMMY_VER, 5) != 0)
		{
			fprintf(stderr, "ffxivgame.ver incorrect\n");
			fprintf(stderr, "Run from Dawntrail benchmark 'game' directory!\n");
			return EXIT_FAILURE;
		}

		fclose(ver_fh);
	}

	db_startup();
	index_startup();

	if (!db_load_all_paths())
		return EXIT_FAILURE;

	// Hard-coded dawntrail benchmark resolutions for path hashes
	collisionResolver = dtBenchCollisionResolver;

	// Hard-coded dawntrail benchmark file list
	for (int i = 0x00; i <= 0x13; ++i)
	{
		// These files don't exist
		if (i == 0x09 || i == 0xD || i == 0xE || i == 0xF || i == 0x10 || i == 0x11)
			continue;

		try_convert(i << 16);
		cleanup();
	}

	try_convert(0x020502); // ex5/020502.win32.index2
	cleanup();

	printf("Total: Generated %d index entries.\n\n", totalGenerated);

	// Also write an ffxivgame.ver file :)
	ver_fh = fopen("ffxivgame.ver", "wb");
	fwrite(DUMMY_VER, 1, DUMMY_VER_LEN, ver_fh);
	fclose(ver_fh);

	return EXIT_SUCCESS;
}
