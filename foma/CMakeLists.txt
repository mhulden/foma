cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

file(READ fomalib.h _VERSION_FILE)
string(REGEX REPLACE ".*MAJOR_VERSION ([0-9]+).*MINOR_VERSION ([0-9]+).*BUILD_VERSION ([0-9]+).*" "\\1.\\2.\\3" _VERSION ${_VERSION_FILE})

project(foma
	VERSION ${_VERSION}
	LANGUAGES C
	)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH ON)

include(GNUInstallDirs)

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8 /std:c17 /permissive- /W4 /MP")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

	add_definitions(-DYY_NO_UNISTD_H) # Prevent Flex generated code including unistd.h
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -Wno-deprecated -Wno-unused-parameter -fvisibility=hidden -fPIC")

	# Require latest possible C standard
	include(CheckCCompilerFlag)
	foreach(flag "-std=c2x" "-std=c18" "-std=c17" "-std=c11" "-std=c1x" "-std=c99")
		string(REGEX REPLACE "[^a-z0-9]" "" _flag ${flag})
		CHECK_C_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_${_flag})
		if(COMPILER_SUPPORTS_${_flag})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
			set(_ENABLED_C ${flag})
			break()
		endif()
	endforeach()
	if(NOT _ENABLED_C)
		message(FATAL_ERROR "Could not enable at least C99 - upgrade your compiler")
	endif()

	add_definitions(-D_GNU_SOURCE)
endif()

if(WIN32)
	add_definitions(-D_SECURE_SCL=0 -D_ITERATOR_DEBUG_LEVEL=0 -D_CRT_SECURE_NO_DEPRECATE -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DNOMINMAX)
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# getopt & readline
find_path(GETOPT_INCLUDE getopt.h)
include_directories(${GETOPT_INCLUDE})
if(VCPKG_TOOLCHAIN)
	find_library(GETOPT_LIB NAMES getopt)
	add_definitions(-DHAVE_GETOPT_LONG)
else()
	set(GETOPT_LIB)

	find_package(PkgConfig REQUIRED)
	pkg_search_module(READLINE readline)
endif()
if(NOT READLINE_INCLUDE_DIRS)
	find_path(READLINE_INCLUDE_DIRS readline.h PATH_SUFFIXES readline REQUIRED)
	find_library(READLINE_LIBRARIES readline REQUIRED)
endif()
include_directories(${READLINE_INCLUDE_DIRS})

# zlib
find_path(ZLIB_INCLUDE zlib.h REQUIRED)
if(VCPKG_TOOLCHAIN)
	find_package(ZLIB REQUIRED)
	find_library(ZLIB_LIBRARIES z REQUIRED)
	set(ZLIB_LIBS ZLIB::ZLIB)
else()
	find_library(ZLIB_LIBRARIES z REQUIRED)
	set(ZLIB_LIBS ${ZLIB_LIBRARIES})
endif()
include_directories(${ZLIB_INCLUDE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

BISON_TARGET(Bregex regex.y "${CMAKE_CURRENT_BINARY_DIR}/regex.c" COMPILE_FLAGS "-v")
FLEX_TARGET(Fregex regex.l "${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c" COMPILE_FLAGS "-8")
FLEX_TARGET(Flexc lexc.l "${CMAKE_CURRENT_BINARY_DIR}/lex.lexc.c" COMPILE_FLAGS "-8 --prefix=lexc")
FLEX_TARGET(Finterface interface.l "${CMAKE_CURRENT_BINARY_DIR}/lex.interface.c" COMPILE_FLAGS "-8 --prefix=interface")
FLEX_TARGET(Fcmatrix cmatrix.l "${CMAKE_CURRENT_BINARY_DIR}/lex.cmatrix.c" COMPILE_FLAGS "-8 --prefix=cmatrix")

set(SOURCES
	foma.h
	fomalib.h
	fomalibconf.h
	lexc.h
	apply.c
	coaccessible.c
	constructions.c
	define.c
	determinize.c
	dynarray.c
	extract.c
	flags.c
	int_stack.c
	io.c
	lexcread.c
	mem.c
	minimize.c
	reverse.c
	rewrite.c
	sigma.c
	spelling.c
	stringhash.c
	structures.c
	topsort.c
	trie.c
	utf8.c
	${FLEX_Fregex_OUTPUTS}
	${FLEX_Flexc_OUTPUTS}
	${FLEX_Fcmatrix_OUTPUTS}
	${BISON_Bregex_OUTPUTS}
	)

add_library(foma-static STATIC ${SOURCES})
target_link_libraries(foma-static PUBLIC ${ZLIB_LIBS})
set_target_properties(foma-static PROPERTIES ARCHIVE_OUTPUT_NAME foma)

add_library(foma-shared SHARED ${SOURCES})
target_link_libraries(foma-shared PRIVATE ${ZLIB_LIBS})
set_target_properties(foma-shared PROPERTIES
	LIBRARY_OUTPUT_NAME foma RUNTIME_OUTPUT_NAME foma
	VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
if(NOT MSVC)
	set_target_properties(foma-shared PROPERTIES ARCHIVE_OUTPUT_NAME foma)
endif()

add_executable(foma-bin foma.c stack.c iface.c ${FLEX_Finterface_OUTPUTS})
target_link_libraries(foma-bin PRIVATE foma-static ${READLINE_LIBRARIES} ${GETOPT_LIB})
set_target_properties(foma-bin PROPERTIES RUNTIME_OUTPUT_NAME foma)

if(MSYS OR NOT WIN32)
	add_executable(flookup flookup.c)
	target_link_libraries(flookup PRIVATE foma-static ${GETOPT_LIB})
	install(TARGETS flookup RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

add_executable(cgflookup cgflookup.c)
target_link_libraries(cgflookup PRIVATE foma-static ${GETOPT_LIB})

configure_file(libfoma.pc.in libfoma.pc @ONLY)

# Install
install(TARGETS foma-static foma-shared ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES fomalib.h fomalibconf.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS foma-bin cgflookup RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libfoma.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
