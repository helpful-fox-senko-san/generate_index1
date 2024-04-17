#ifndef INDEX_CONV_H
#define INDEX_CONV_H

extern int generatedCount;
extern int skippedCount;

// Populate Index1 data (file/folder) from Index2 (fullpath)
void generate_index1_data();

// Generate Index2 (fullpath) data from Index1 (file/folder)
void generate_index2_data();

#endif // INDEX_CONV_H
