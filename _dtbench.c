#include "hashlist_db.h"

#include <stdio.h>

#include "crc32.h"

#define LITERAL_CRC32(s) CRC32(s, sizeof((s)) - 1)

#define CASE_RESOLVE(fullHash, path, filename) case fullHash: \
	targetFolderHash = ~LITERAL_CRC32(path); \
	targetFileHash = ~LITERAL_CRC32(filename); \
	break;

int dtBenchCollisionResolver(uint32_t indexId, HashlistRecord* first, HashlistRecord* second)
{
	uint32_t fullHash = first->FullHash;

	uint32_t targetFileHash = 0xFFFFFFFFU;
	uint32_t targetFolderHash = 0xFFFFFFFFU;

	switch (indexId)
	{
		case 0x040000:
			switch (fullHash)
			{
				CASE_RESOLVE(0x34d84da7, "chara/human/c1701/animation/f0002/nonresident/emot", "salute_gcb.pap")
				CASE_RESOLVE(0x4dc02876, "chara/human/c1101/skeleton/met/m5046", "phy_c1101m5046.phyb")
				CASE_RESOLVE(0xe467d7e7, "chara/human/c0701/animation/f0002/nonresident", "mogu_paku.pap")
				CASE_RESOLVE(0xf161f6dd, "chara/human/c0201/animation/f0004/nonresident/emot", "clap.pap")
			}
			break;
	}

	if (targetFileHash == first->FileHash && targetFolderHash == first->FolderHash)
		return 1;
	else if (targetFileHash == second->FileHash && targetFolderHash == second->FolderHash)
		return 2;
	else
		return 0;
}
