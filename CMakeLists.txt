cmake_minimum_required(VERSION 3.10)
project(foma C)

set(CPACK_PACKAGE_NAME "foma")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 10)
set(CPACK_PACKAGE_VERSION_PATCH 0)

find_package(FLEX)
find_package(BISON)

set(CMAKE_C_STANDARD 99)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release MinSizeRel RelWithDebInfo" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# bison and flex
bison_target(REGEX "${CMAKE_CURRENT_SOURCE_DIR}/foma/regex.y" "${CMAKE_CURRENT_BINARY_DIR}/regex.c"
             DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/regex.h")
flex_target(YY "${CMAKE_CURRENT_SOURCE_DIR}/foma/regex.l" "${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c")
add_flex_bison_dependency(YY REGEX)
flex_target(LEXC "${CMAKE_CURRENT_SOURCE_DIR}/foma/lexc.l" "${CMAKE_CURRENT_BINARY_DIR}/lex.lexc.c"
            COMPILE_FLAGS "-8 --prefix=lexc")
flex_target(INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/foma/interface.l" "${CMAKE_CURRENT_BINARY_DIR}/lex.interface.c"
            COMPILE_FLAGS "-8 --prefix=interface")
flex_target(CMATRIX "${CMAKE_CURRENT_SOURCE_DIR}/foma/cmatrix.l" "${CMAKE_CURRENT_BINARY_DIR}/lex.cmatrix.c"
            COMPILE_FLAGS "-8 --prefix=cmatrix")

# source compile
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/foma/foma*.h")

add_library(foma OBJECT
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/apply.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/coaccessible.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/constructions.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/define.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/determinize.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/dynarray.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/extract.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/flags.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/int_stack.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/io.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/lexcread.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/mem.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/minimize.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/reverse.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/rewrite.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/sigma.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/spelling.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/stringhash.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/structures.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/topsort.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/trie.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/foma/utf8.c"
            ${BISON_REGEX_OUTPUT_SOURCE}
            ${FLEX_YY_OUTPUTS}
            ${FLEX_LEXC_OUTPUTS}
            ${FLEX_CMATRIX_OUTPUTS}
            )
target_compile_options(foma PRIVATE -O3 -Wall -D_GNU_SOURCE -fvisibility=hidden -fPIC)
target_include_directories(foma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/foma)
target_link_libraries(foma PUBLIC -lreadline -lz -ltermcap)

include(CPack)

add_library(foma_static STATIC $<TARGET_OBJECTS:foma>)
set_target_properties(foma_static PROPERTIES
                      OUTPUT_NAME foma
                      VERSION "${CPACK_PACKAGE_VERSION}"
                      SOVERSION "${CPACK_PACKAGE_VERSION_MAJOR}"
                      PUBLIC_HEADER "${HEADERS}")

add_library(foma_shared SHARED $<TARGET_OBJECTS:foma>)
target_link_libraries(foma_shared
                      PUBLIC -lreadline -lz -ltermcap
                      PRIVATE foma)
set_target_properties(foma_shared PROPERTIES
                      OUTPUT_NAME foma
                      VERSION "${CPACK_PACKAGE_VERSION}"
                      SOVERSION "${CPACK_PACKAGE_VERSION_MAJOR}"
                      PUBLIC_HEADER "${HEADERS}")

add_executable(foma_exec
               "${CMAKE_CURRENT_SOURCE_DIR}/foma/foma.c"
               "${CMAKE_CURRENT_SOURCE_DIR}/foma/stack.c"
               "${CMAKE_CURRENT_SOURCE_DIR}/foma/iface.c"
               ${FLEX_INTERFACE_OUTPUTS}
               )
target_include_directories(foma_exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/foma)
target_compile_options(foma_exec PUBLIC -O3 -Wall -D_GNU_SOURCE -fvisibility=hidden)
target_link_libraries(foma_exec PUBLIC foma -lreadline -lz -ltermcap)
set_target_properties(foma_exec PROPERTIES OUTPUT_NAME foma)

add_executable(flookup "${CMAKE_CURRENT_SOURCE_DIR}/foma/flookup.c")
target_compile_options(flookup PUBLIC -O3 -Wall -D_GNU_SOURCE -fvisibility=hidden)
target_link_libraries(flookup PUBLIC foma -lreadline -lz -ltermcap)

add_executable(cgflookup "${CMAKE_CURRENT_SOURCE_DIR}/foma/cgflookup.c")
target_compile_options(cgflookup PUBLIC -O3 -Wall -D_GNU_SOURCE -fvisibility=hidden)
target_link_libraries(cgflookup PUBLIC foma -lreadline -lz -ltermcap)

# install
install(TARGETS foma_shared foma_static
        EXPORT foma
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/foma)
install(TARGETS foma_exec flookup cgflookup
        RUNTIME DESTINATION bin)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/foma-config.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/foma-config.cmake
                              INSTALL_DESTINATION lib/cmake/foma)
write_basic_package_version_file(foma-config-version.cmake
                                 VERSION "${CPACK_PACKAGE_VERSION}"
                                 COMPATIBILITY SameMinorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/foma-config.cmake
        DESTINATION lib/cmake/foma)

install(EXPORT foma
        NAMESPACE foma::
        DESTINATION lib/cmake/foma)
