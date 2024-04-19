#include "hashlist_db.h"

#include <stdio.h>

#include "thirdparty/crc32.h"

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
				CASE_RESOLVE(0x16529f56, "chara/human/c0201/obj/hair/h0176/texture", "c0201h0176_acc_m.tex")
				CASE_RESOLVE(0x26ce231b, "chara/equipment/e0038/texture", "v13_c0101e0038_top_id.tex")
				CASE_RESOLVE(0x34d84da7, "chara/human/c1701/animation/f0002/nonresident/emot", "salute_gcb.pap")
				CASE_RESOLVE(0x4dc02876, "chara/human/c1101/skeleton/met/m5046", "phy_c1101m5046.phyb")
				CASE_RESOLVE(0x987b6856, "chara/equipment/e0746/model", "c0301e0746_top.mdl")
				CASE_RESOLVE(0xa00b05da, "chara/human/c0501/animation/f0008/nonresident/emot", "j_goodbye_st.pap")
				CASE_RESOLVE(0xa3db22b6, "chara/weapon/w3101/obj/body/b0011/texture", "v01_w3101b0011_id.tex")
				CASE_RESOLVE(0xa8732c25, "chara/human/c0101/animation/a0001/bt_common/emote", "u_huh.pap")
				CASE_RESOLVE(0xabcebe5a, "chara/human/c1401/animation/f0002/nonresident/emot", "sp54.pap")
				CASE_RESOLVE(0xdd038c14, "chara/human/c0501/skeleton/met/m0589", "skl_c0501m0589.sklb")
				CASE_RESOLVE(0xdd5b77f8, "chara/human/c0201/obj/hair/h0006/texture", "c0201h0006_hir_n.tex")
				CASE_RESOLVE(0xde232758, "chara/human/c0901/animation/a0001/bt_common/event", "event_move_run_end_r.pap")
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
