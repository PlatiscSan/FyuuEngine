cmake_minimum_required(VERSION 3.28)
project(fyuu_rhi VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Boost locale
find_package(Boost REQUIRED COMPONENTS locale)

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

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_cxx_modules.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/disable_rtti.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_headers.cmake)

# set up Vulkan C++ module as a library
add_library(VulkanHppModule STATIC)
disable_rtti(VulkanHppModule)
target_sources(VulkanHppModule 
    PRIVATE
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIR}
        FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)
target_compile_features(VulkanHppModule 
    PUBLIC 
        cxx_std_20
)
target_link_libraries(VulkanHppModule 
    PUBLIC 
        Vulkan::Vulkan
)
target_compile_definitions(VulkanHppModule 
    PRIVATE
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
        #VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL
)
if(WIN32)
target_compile_definitions(VulkanHppModule 
    PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
)
elseif(UNIX)
target_compile_definitions(VulkanHppModule 
    PRIVATE
        VK_USE_PLATFORM_WAYLAND_KHR
        VK_USE_PLATFORM_XLIB_KHR
)
elseif(ANDROID)
target_compile_definitions(VulkanHppModule 
    PRIVATE
        VkAndroidSurfaceCreateInfoKHR
)
endif(WIN32)

add_library(${PROJECT_NAME} STATIC)

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src ""
    rhi_modules 
    rhi_module_impls 
    all_rhi_files
)

collect_headers(${CMAKE_CURRENT_SOURCE_DIR}/include/ ""
    public_headers
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(${PROJECT_NAME}
    PRIVATE FILE_SET CXX_MODULES FILES 
        ${rhi_modules}
    PRIVATE 
        ${public_headers}
    PRIVATE
        ${rhi_module_impls}
        ${interface_impl}
)

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE
        VMA_IMPLEMENTATION
)

disable_rtti(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        plastic
        fyuu_glad

        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator

        VulkanHppModule

        glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-util
        spirv-cross-cpp
        Boost::locale
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
    )
endif()