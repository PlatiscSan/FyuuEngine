cmake_minimum_required(VERSION 3.28)
project(FyuuRHI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Boost
find_package(Boost REQUIRED COMPONENTS locale)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Boost REQUIRED COMPONENTS uuid)
find_package(Boost REQUIRED COMPONENTS container)

# Find Vulkan package and check version
find_package(Vulkan REQUIRED)
if(Vulkan_VERSION VERSION_LESS "1.3.256")
    message(FATAL_ERROR 
        "Minimum required Vulkan version for C++ modules is 1.3.256. "
        "Found ${Vulkan_VERSION}."
    )
endif()

# Vulkan memory allocator
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# webgpu Dawn
find_package(Dawn CONFIG REQUIRED)

# shader compiler
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_msl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_c CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)

if(WIN32)
    # DirectX
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directxtk12 CONFIG REQUIRED)
    find_package(D3D12MemoryAllocator CONFIG REQUIRED)
endif(WIN32)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_sources.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_cxx_modules.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/disable_rtti.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_headers.cmake)

add_library(${PROJECT_NAME} STATIC)

collect_sources(${CMAKE_CURRENT_SOURCE_DIR}/src ""
    sources
)

if(WIN32)
    list(FILTER sources EXCLUDE REGEX ".*glad_glx\\.c$|.*glad_egl\\.c$|.*glad_x11\\.c$")
elseif(UNIX)
    list(FILTER sources EXCLUDE REGEX ".*glad_wgl\\.c$")
elseif(ANDROID)
    list(FILTER sources EXCLUDE REGEX ".*glad_glx\\.c$|.*glad_wgl\\.c$")
elseif(APPLE)
    list(FILTER sources EXCLUDE REGEX ".*glad_glx\\.c$|.*glad_egl\\.c$|.*glad_wgl\\.c$")
endif()

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/module "interface"
    interface_modules 
)

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src ""
    implementation_modules 
)

collect_headers(${CMAKE_CURRENT_SOURCE_DIR}/include/ ""
    public_headers
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET CXX_MODULES FILES 
        ${interface_modules}
        ${implementation_modules}
    PRIVATE 
        ${public_headers}
    PRIVATE
        ${sources}
)

target_compile_definitions(${PROJECT_NAME} 
    PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
        PIX_ENABLED
)

disable_rtti(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        plastic
        #-- Boost
        Boost::system
        Boost::locale
        Boost::uuid
        Boost::container
        #-- Vulkan
        VulkanHppModule
        GPUOpen::VulkanMemoryAllocator
        #-- webgpu Dawn
        dawn::webgpu_dawn
        #-- shader compiler
        glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-util
        spirv-cross-cpp
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            D3DCompiler
            Microsoft::DirectX-Headers
            Microsoft::DirectXTK12
            GPUOpen::D3D12MemoryAllocator
            Microsoft::DirectXShaderCompiler

            WinPixEventRuntime

            Opengl32.lib
    )
endif()

