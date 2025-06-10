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
	foreach(flag "-std=c18" "-std=c17" "-std=c11" "-std=c1x" "-std=c99")
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

# Only look for readline if not building for WASM
if(NOT EMSCRIPTEN)
    if(NOT READLINE_INCLUDE_DIRS)
        find_path(READLINE_INCLUDE_DIRS readline.h PATH_SUFFIXES readline REQUIRED)
        find_library(READLINE_LIBRARIES readline REQUIRED)
    endif()
    include_directories(${READLINE_INCLUDE_DIRS})
endif()

# zlib handling
if(EMSCRIPTEN)
    # Emscripten-specific zlib handling
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --use-port=zlib")
    set(ZLIB_LIBS "")
else()
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
endif()

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

# WASM-specific target
if(EMSCRIPTEN)
    # Create WASM library target
    add_executable(libfoma ${SOURCES})
    target_link_libraries(libfoma PRIVATE ${ZLIB_LIBS})

    # Emscripten-specific link flags - export all fomalib functions
    set(EXPORTED_FUNCTIONS
        "[\'_malloc\',\'_free\',\'_defined_networks_init\',\'_defined_functions_init\',\'_add_defined\',\'_add_defined_function\',\'_fsm_construct_init\',\'_fsm_construct_set_final\',\'_fsm_construct_set_initial\',\'_fsm_construct_add_arc\',\'_fsm_construct_add_arc_nums\',\'_fsm_construct_add_symbol\',\'_fsm_construct_check_symbol\',\'_fsm_construct_copy_sigma\',\'_fsm_construct_done\',\'_fsm_parse_regex\',\'_fsm_create\',\'_fsm_empty\',\'_fsm_empty_set\',\'_fsm_universal\',\'_fsm_identity\',\'_fsm_contains\',\'_fsm_contains_one\',\'_fsm_contains_opt_one\',\'_fsm_minimize\',\'_fsm_determinize\',\'_fsm_epsilon_remove\',\'_fsm_reverse\',\'_fsm_invert\',\'_fsm_lower\',\'_fsm_upper\',\'_fsm_kleene_star\',\'_fsm_kleene_plus\',\'_fsm_optionality\',\'_fsm_concat\',\'_fsm_union\',\'_fsm_intersect\',\'_fsm_compose\',\'_fsm_complement\',\'_fsm_minus\',\'_fsm_simple_replace\',\'_fsm_context_restrict\',\'_fsm_isempty\',\'_fsm_isfunctional\',\'_fsm_isidentity\',\'_fsm_destroy\',\'_fsm_copy\',\'_fsm_complete\',\'_apply_init\',\'_apply_clear\',\'_apply_up\',\'_apply_down\',\'_apply_words\',\'_apply_upper_words\',\'_apply_lower_words\',\'_apply_random_words\',\'_apply_random_upper\',\'_apply_random_lower\',\'_fsm_get_library_version_string\',\'_fsm_set_option\',\'_fsm_get_option\',\'_fsm_read_binary_file\',\'_fsm_write_binary_file\',\'_fsm_trie_init\',\'_fsm_trie_done\',\'_fsm_trie_add_word\',\'_fsm_trie_end_word\',\'_sigma_copy\',\'_fsm_sigma_destroy\',\'_fsm_merge_sigma\']")

    set_target_properties(libfoma PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "-s WASM=1 \
                   -s EXPORTED_RUNTIME_METHODS=[\'ccall\',\'cwrap\',\'stringToUTF8\',\'UTF8ToString\'] \
                   -s EXPORTED_FUNCTIONS=${EXPORTED_FUNCTIONS} \
                   -s ENVIRONMENT=web"
    )

    # Add custom target to clean WASM-generated files
    set(WASM_GENERATED_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libfoma.js
        ${CMAKE_CURRENT_BINARY_DIR}/libfoma.wasm
    )

    set_directory_properties(PROPERTIES
        ADDITIONAL_CLEAN_FILES "${WASM_GENERATED_FILES}"
    )

    install(TARGETS libfoma RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    # Regular foma-bin target
    add_executable(foma-bin foma.c stack.c iface.c ${FLEX_Finterface_OUTPUTS})
    target_link_libraries(foma-bin PRIVATE foma-static ${READLINE_LIBRARIES} ${GETOPT_LIB})
    target_link_directories(foma-bin PRIVATE ${READLINE_LIBRARY_DIRS})
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

endif()
