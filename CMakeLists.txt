cmake_minimum_required(VERSION 2.8.12)

project(generate_index1 LANGUAGES C)

add_library(sqlite3
	sqlite3.c
	sqlite3.h
)

add_executable(generate_index1
	_dtbench.c
	crc32.c
	crc32.h
	hash.c
	hash.h
	hashlist_db.c
	hashlist_db.h
	index.c
	index.h
	index_conv.c
	index_conv.h
	main.c
	sha1.c
	sha1.h
	sqpack.c
	sqpack.h
	util.c
	util.h
)

target_link_libraries(generate_index1 sqlite3)
