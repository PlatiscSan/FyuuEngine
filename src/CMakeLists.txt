# src/CMakeLists.txt
cmake_minimum_required(VERSION 4.2.3)
set(CMAKE_CXX_MODULES ON) 

# Set experimental flag to enable `import std` support from CMake.
# This must be enabled before C++ language support.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
  # This specific value changes as experimental support evolves. See
  # `Help/dev/experimental.rst` in the CMake source corresponding to
  # your CMake build for the exact value to use.
  "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

project(FyuuEngine)

include(${PROJECT_ROOT_DIR}/cmake/disable_rtti.cmake)
include(${PROJECT_ROOT_DIR}/cmake/collect_cxx_modules.cmake)
include(${PROJECT_ROOT_DIR}/cmake/collect_sources.cmake)
include(${PROJECT_ROOT_DIR}/cmake/collect_headers.cmake)


if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED)
else()
    add_library(${PROJECT_NAME} STATIC)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            BUILD_STATIC_LIBS
    )
endif()

collect_sources(${ENGINE_SOURCE_DIR} ""
    sources
)

collect_cxx_modules(${ENGINE_SOURCE_DIR} ""
    modules 
    module_impls
)

target_sources(${PROJECT_NAME}
    PUBLIC 
        ${PUBLIC_HEADERS}
)

target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS
            ${PROJECT_ROOT_DIR}/module
            ${CMAKE_CURRENT_SOURCE_DIR}
        FILES 
            ${MODULE_INTERFACES}
            ${modules}
    PRIVATE
        ${module_impls}
        ${sources}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_ROOT_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

disable_rtti(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME}
    PRIVATE   cxx_std_23
    INTERFACE cxx_std_20)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        plastic
        FyuuRHI
        # Boost
        Boost::uuid
        Boost::thread
        
        # command line parser
        CLI11::CLI11

        # graphics
        SDL3::SDL3
        imgui::imgui

        Eigen3::Eigen
        Jolt::Jolt

        # log
        spdlog::spdlog

        # serialization
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json

        # parallel programming
        TBB::tbb TBB::tbbmalloc TBB::tbbbind_2_5 TBB::tbbmalloc_proxy
)