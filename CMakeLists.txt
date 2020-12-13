# In order to make release or debug builds, set CMAKE_BUILD_TYPE to Release or Debug.
# In order to change the install prefix, set CMAKE_INSTALL_PREFIX.

# User customizations can be placed in this directory in a file named "local.cmake".

cmake_minimum_required(VERSION 3.5)

# <Package>_ROOT variables are now added to the path by default.  We
# were explicitly using our _ROOT variables for this purpose, so the
# "NEW" policy should be fine, although we might want to revisit
# whether we should be doing things differently under the new policy.
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# Handle option without warnings in newer cmake
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

project(liboodle CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
  
include(AddCXXCompilerFlags)
add_cxx_compiler_flags(-pedantic -fno-permissive -pedantic-errors -Wall -Wextra -Wshadow -Wstrict-aliasing -Wno-misleading-indentation "-ftemplate-depth=1024")
option(OODLE_FAIL_ON_WARNING "Cause compilation to fail if a warning is generated" OFF)
if(OODLE_FAIL_ON_WARNING)
  add_cxx_compiler_flags(-Werror)
endif()

option(OODLE_ENABLE_AVX2 "Enable AVX Vector Extensions" ON)
if(OODLE_ENABLE_AVX2)
  add_cxx_compiler_flags(-mavx -mavx2)
  add_compile_definitions(OODLE_ENABLE_AVX2)
endif()

# The following goes against the principle of the GPLv3
# and as such is disabled by default.
option(OODLE_ENABLE_DLL "Enable loading of proprietary Oodle DLL" OFF)
if(OODLE_ENABLE_DLL)
  add_compile_definitions(ENABLE_USE_DLL)
endif()

option(OODLE_COMPILE_SANITIZE "Compile with undefined behavior sanitization enabled")
if(OODLE_COMPILE_SANITIZE)
  add_cxx_compiler_flags(-fsanitize=undefined)
endif()

#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH true)
#set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# Load the user's customizations, then the site customization, then
# the local customization.  This lets the local customization take
# precedence over site when setting cache variables, and when setting
# non-cache variables.
include("${CMAKE_CURRENT_SOURCE_DIR}/local.cmake" OPTIONAL)
include("${CMAKE_CURRENT_SOURCE_DIR}/site.cmake" OPTIONAL)
include("${CMAKE_CURRENT_SOURCE_DIR}/local.cmake" OPTIONAL)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Using Build Type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(THREADS_PREFER_PTHREAD_FLAG on)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(OODLE_RELEASE true)
endif()

set(OODLE_SOURCES
  bitknit.cpp
  compr_entropy.cpp
  compr_kraken.cpp
  compr_leviathan.cpp
  compr_match_finder.cpp
  compr_mermaid.cpp
  compr_multiarray.cpp
  compr_tans.cpp
  compress.cpp
  lzna.cpp
  kraken_impl.cpp
  mermaid_impl.cpp
  leviathan_impl.cpp
  lzna_impl.cpp
  bitreader.cpp
  util.cpp
)

# Note: the executable and the library are built from the same sources; the only difference is linking data vs executable entry point.
# Build both shared and static copies of the shared library.
add_library(liboodle_o OBJECT ${OODLE_SOURCES})
# Yes, this actually needs to be specified manually. CMake doesn't do a reverse-dependency heuristic for some reason.
set_property(TARGET liboodle_o PROPERTY POSITION_INDEPENDENT_CODE 1)
add_library(oodle SHARED $<TARGET_OBJECTS:liboodle_o>)
add_library(oodle_static STATIC $<TARGET_OBJECTS:liboodle_o>)
# Executable needs the main kraken.cpp, as that's what has main()
add_executable(ooz $<TARGET_OBJECTS:liboodle_o> kraken.cpp)

install(TARGETS oodle oodle_static DESTINATION lib)
install(TARGETS ooz DESTINATION bin)
