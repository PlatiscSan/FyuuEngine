cmake_minimum_required (VERSION 3.8)

add_library(utility STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/utility" utility utility_src)
target_sources(utility
    PRIVATE
        ${utility_src}
)

add_library(concurrency STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/concurrency" concurrency concurrency_src)
target_sources(concurrency
    PRIVATE
        ${concurrency_src}
)
target_link_libraries(concurrency
    PRIVATE
        utility
)

add_library(logger STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/logger" logger logger_src)
target_sources(logger
    PRIVATE
        ${logger_src}
)
target_link_libraries(logger
    PRIVATE
        concurrency
)

add_library(config STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/config" config config_src)
target_sources(config
    PRIVATE
        ${config_src}
)
target_link_libraries(config
    PRIVATE
        utility
        yaml-cpp::yaml-cpp
)

add_library(math STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/math" math math_src)
target_sources(math
    PRIVATE
        ${math_src}
)
target_link_libraries(math
    PRIVATE
        utility
        Eigen3::Eigen
)

add_library(physics STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/physics" physics physics_src)
target_sources(physics
    PRIVATE
        ${physics_src}
)
target_link_libraries(physics
    PRIVATE
        utility
        math
        config
        Jolt::Jolt
)

add_library(spirv STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/rendering/spirv" spirv spirv_src)
target_sources(spirv
    PRIVATE
        ${spirv_src}
)
target_link_libraries(spirv
    PRIVATE
        utility
        spirv-cross-c
        spirv-cross-core
        spirv-cross-cpp
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-util
        unofficial::spirv-reflect
)

add_library(core STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/core" core core_src)
    target_sources(core
        PRIVATE
            ${core_src}
)

target_link_libraries(core
    PRIVATE
        utility
        concurrency
        logger
        config
)

add_library(vulkan_backend STATIC)
recursive_group_sources("${Vulkan_INCLUDE_DIR}/rendering/vulkan" vulkan_backend vulkan_src)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/rendering/vulkan_backend" vulkan_backend vulkan_backend_src)
target_sources(vulkan_backend
    PRIVATE
        ${vulkan_backend_src}
)
target_sources(vulkan_backend 
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIR}
        FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)

target_compile_features(vulkan_backend 
    PUBLIC 
        cxx_std_20 
)
target_link_libraries(vulkan_backend
    PUBLIC
        core
        Vulkan::Vulkan
        glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper
)
if(WIN32)
    target_compile_definitions(vulkan_backend
        PUBLIC
            VK_USE_PLATFORM_WIN32_KHR
    )
endif(WIN32)


add_library(opengl_backend STATIC)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/rendering/opengl_backend" opengl_backend opengl_backend_src)
target_sources(opengl_backend
    PRIVATE
        ${opengl_backend_src}
)
target_link_libraries(opengl_backend
    PUBLIC
        core
        spirv-cross-c
        spirv-cross-core
        spirv-cross-cpp
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-util        
)

if(WIN32)
    add_library(windows_core STATIC)
    recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/platform/windows" windows_core windows_imp)
    target_sources(windows_core
        PRIVATE
            ${windows_imp}
    )
    target_link_libraries(windows_core
        PRIVATE
            core
            Boost::uuid
            d3d12
            dxgi
            d3dcompiler
            dxguid
            Opengl32
            vulkan_backend
            opengl_backend
            spirv-cross-c
            spirv-cross-core
            spirv-cross-cpp
            spirv-cross-glsl
            spirv-cross-hlsl
            spirv-cross-msl
            spirv-cross-reflect
            spirv-cross-util

    )
endif(WIN32)

if(BUILD_SHARED_LIBS)
    set(application_define DLL_EXPORTS)
else()
    set(application_define STATIC_LIB)
endif()

add_library(application)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/application" application application_src)
target_sources(application
    PRIVATE
        ${application_src}
)
target_compile_definitions(application 
    PRIVATE 
        ${application_define}
)
if(WIN32)
    target_link_libraries(application
        PRIVATE
            windows_core
    )
endif(WIN32)