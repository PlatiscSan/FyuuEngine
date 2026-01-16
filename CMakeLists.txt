# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project(FyuuEngine)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable generation of compile_commands.json for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories for binaries and libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Define source directories
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    # Enable Hot Reload for MSVC compilers if supported.
    if (POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()

    # Set common MSVC compiler options
    add_compile_options(
        /Zc:__cplusplus # Enable standard __cplusplus macro
        /Zc:preprocessor # Enable standard preprocessor behavior
        /permissive- # Enable standards-compliant mode
        /W4 # Enable high warning level
        /wd4100 # Unreferenced formal parameter
        /wd4324 # Structure was padded due to alignment specifier
        /wd4201 # Nonstandard extension used: nameless struct/union
        /wd4514 # Unreferenced inline function has been removed
        /wd4710 # Function not inlined
        /wd4711 # Function selected for automatic inline expansion
        /bigobj # Enable large object files
        /utf-8 # Set source and execution character set to UTF-8
    )


else()
    add_compile_options(
        -Wall # Enable all warnings
        -Wextra # Enable extra warnings
        -Wpedantic # Enable pedantic warnings
        -Wno-unused-parameter # Suppress unused parameter warnings
        -Wno-unused-variable # Suppress unused variable warnings
        -Wno-unused-function # Suppress unused function warnings
        -Wno-sign-compare # Suppress signed/unsigned comparison warnings
    )


endif()

# Enable position independent code for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Boost
find_package(Boost REQUIRED COMPONENTS uuid)
if(Boost_FOUND)
    add_definitions(-DBOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX)
else()
    message(FATAL_ERROR "Boost not found!")
endif()

# log
find_package(spdlog CONFIG REQUIRED)

# Eigen3
find_package(Eigen3 REQUIRED)

# graphics libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# physics
find_package(Jolt CONFIG REQUIRED)

# Include subdirectories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include(${PROJECT_ROOT_DIR}/cmake/disable_rtti.cmake)
include(${PROJECT_ROOT_DIR}/cmake/collect_cxx_modules.cmake)
include(${PROJECT_ROOT_DIR}/cmake/collect_sources.cmake)
include(${PROJECT_ROOT_DIR}/cmake/collect_headers.cmake)

collect_headers(${CMAKE_CURRENT_SOURCE_DIR}/include "Headers" public_headers)

add_subdirectory(lib/plastic)
add_subdirectory(lib/fyuu_glad)
add_subdirectory(lib/PixEvents-cmake)
add_subdirectory(lib/fyuu_rhi)
add_subdirectory(src)

set(BUILD_TESTING ON)

if(BUILD_TESTING)
    add_subdirectory("test/hello triangle")
endif()