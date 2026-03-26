# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 4.2.3)

# # Set experimental flag to enable `import std` support from CMake.
# # This must be enabled before C++ language support.
# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
# 	# This specific value changes as experimental support evolves. See
# 	# `Help/dev/experimental.rst` in the CMake source corresponding to
# 	# your CMake build for the exact value to use.
# 	"451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

set(CMAKE_CXX_MODULES ON) 

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(FyuuEngine)

# set(CMAKE_CXX_MODULE_STD 1)

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
find_package(Boost REQUIRED COMPONENTS thread)

if(Boost_FOUND)
    add_definitions(-DBOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX)
else()
    message(FATAL_ERROR "Boost not found!")
endif()

# command line parser
find_package(CLI11 CONFIG REQUIRED)

# serialization
find_package(yaml-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# parallel programming
find_package(PkgConfig REQUIRED)
pkg_check_modules(HWLOC REQUIRED IMPORTED_TARGET hwloc)
find_package(TBB CONFIG REQUIRED)

# log
find_package(spdlog CONFIG REQUIRED)

# Math
find_package(Eigen3 REQUIRED)

# graphics libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# physics
find_package(Jolt CONFIG REQUIRED)

# Include subdirectories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib/plastic)
add_subdirectory(lib/vulkan_hpp)
add_subdirectory(lib/PixEvents-cmake)
add_subdirectory(lib/fyuu_rhi)

# Collect public headers and module interfaces

file(GLOB_RECURSE PUBLIC_HEADERS 
    "${CMAKE_SOURCE_DIR}/include/*.h"
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/*.hxx"
)

file(GLOB_RECURSE MODULE_INTERFACES
    "${CMAKE_SOURCE_DIR}/module/*.cppm"
    "${CMAKE_SOURCE_DIR}/module/*.ixx"
)
add_subdirectory(src)

set(BUILD_TESTING ON)

if(BUILD_TESTING)
    add_subdirectory("test/hello_triangle")
endif()