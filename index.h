#ifndef INDEX_H
#define INDEX_H

#include <stdint.h>

#include "hash.h"

extern HashTable* folderTable;
extern HashTable* pathTable;

#define MAX_FOLDERS (128 * 1024)
extern HashTable* folderFileLists[MAX_FOLDERS];
extern uint32_t folderHashes[MAX_FOLDERS];
extern uint32_t folderCount;

void index_startup(void);

HashTable* get_folder_file_table(uint32_t folderHash);

void clear_folder_file_table();

#endif // INDEX_H
