#ifndef UTIL_H
#define UTIL_H

#define my_assert(condition) my_assert_((condition), #condition)

void my_assert_(int condition, const char* errstr);
void my_assert_f(int condition, const char* fmt, ...);

// Converts SHA1 data (20 bytes) in to a printable hex string
char* sha1str(char* bytes);

#endif // UTIL_H
