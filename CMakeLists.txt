cmake_minimum_required(VERSION 3.10)

# Version.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/glbind.h" GLBIND_VERSION_LINE REGEX "glbind - v[0-9]+\\.[0-9]+\\.[0-9]+")

string(REGEX MATCH "v([0-9]+\\.[0-9]+\\.[0-9]+)" _ ${GLBIND_VERSION_LINE})
set(GLBIND_VERSION ${CMAKE_MATCH_1})

message(STATUS "glbind Version: ${GLBIND_VERSION}")

project(glbind VERSION ${GLBIND_VERSION})


# Options
option(GLBIND_BUILD_EXAMPLES "Build glbind examples"            OFF)
option(GLBIND_BUILD_TESTS    "Build glbind tests"               OFF)
option(GLBIND_BUILD_TOOLS    "Build glbind tools"               OFF)
option(GLBIND_FORCE_CXX      "Force compilation as C++"         OFF)
option(GLBIND_FORCE_C89      "Force compilation as C89"         OFF)


# Construct compiler options.
set(COMPILE_OPTIONS)

if(GLBIND_FORCE_CXX AND GLBIND_FORCE_C89)
    message(FATAL_ERROR "GLBIND_FORCE_CXX and GLBIND_FORCE_C89 cannot be enabled at the same time.")
endif()

if(GLBIND_FORCE_CXX)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Compiling as C++ (GNU/Clang)")
        list(APPEND COMPILE_OPTIONS -x c++)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(STATUS "Compiling as C++ (MSVC)")
        list(APPEND COMPILE_OPTIONS /TP)
    else()
        message(WARNING "GLBIND_FORCE_CXX is enabled but the compiler does not support it. Ignoring.")
    endif()
endif()

if(GLBIND_FORCE_C89)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Compiling as C89")
        list(APPEND COMPILE_OPTIONS -std=c89)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(WARNING "MSVC does not support forcing C89. GLBIND_FORCE_C89 ignored.")
    else()
        message(WARNING "GLBIND_FORCE_C89 is enabled but the compiler does not support it. Ingoring.")
    endif()
endif()

# Warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    #list(APPEND COMPILE_OPTIONS /W4)
endif()


# Link libraries
set(COMMON_LINK_LIBRARIES)

if (UNIX)
    list(APPEND COMMON_LINK_LIBRARIES dl)      # For dlopen(), etc. Most compilers will link to this by default, but some may not.
    list(APPEND COMMON_LINK_LIBRARIES pthread) # Some compilers will not link to pthread by default so list it here just in case.
    list(APPEND COMMON_LINK_LIBRARIES m)

    # If we're compiling for 32-bit ARM we need to link to -latomic.
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        list(APPEND COMMON_LINK_LIBRARIES atomic)
    endif()
endif()


# Common interface
add_library(glbind_common INTERFACE)
target_compile_options(glbind_common INTERFACE ${COMPILE_OPTIONS})
target_link_libraries (glbind_common INTERFACE ${COMMON_LINK_LIBRARIES})

if (UNIX)
    target_link_libraries(glbind_common INTERFACE X11)
endif()


# Examples
if (GLBIND_BUILD_EXAMPLES)
    add_executable(01_Triangle examples/01_Triangle/01_Triangle.c)
    target_link_libraries(01_Triangle glbind_common)

    add_executable(99_ARB_shaders examples/99_ARB_shaders/99_ARB_shaders.c)
    target_link_libraries(99_ARB_shaders glbind_common)
endif()

# Tools
if (GLBIND_BUILD_TOOLS)
    add_executable(glbind_build source/glbind_build.cpp)
    target_link_libraries(glbind_build glbind_common)
endif()
