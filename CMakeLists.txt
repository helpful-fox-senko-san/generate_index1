cmake_minimum_required(VERSION 2.8.12)

project(generate_index1 LANGUAGES C)

add_library(sqlite3
	thirdparty/sqlite3.c
	thirdparty/sqlite3.h
)

add_executable(generate_index1
	thirdparty/crc32.c
	thirdparty/crc32.h
	thirdparty/sha1.c
	thirdparty/sha1.h
	_dtbench.c
	hash.c
	hash.h
	hashlist_db.c
	hashlist_db.h
	index.c
	index.h
	index_conv.c
	index_conv.h
	main.c
	sqpack.c
	sqpack.h
	util.c
	util.h
)

target_link_libraries(generate_index1 sqlite3)
