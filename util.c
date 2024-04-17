#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void my_assert_f(int condition, const char* fmt, ...)
{
	if (condition)
		return;
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	exit(1);
}

void my_assert_(int condition, const char* errstr)
{
	if (condition)
		return;
	fprintf(stderr, "Assertion failed: %s\n", errstr);
	exit(1);
}

char* sha1str(char* bytes)
{
	static /* _Thread */ char strbuf[64];
	for (int i = 0; i < 20; ++i)
	{
		unsigned char b = (unsigned char)bytes[i];
		strbuf[i * 2] = "0123456789abcdef"[(b >> 4) & 0xF];
		strbuf[i * 2 + 1] = "0123456789abcdef"[b & 0xF];
	}
	strbuf[42] = 0;
	return strbuf;
}

