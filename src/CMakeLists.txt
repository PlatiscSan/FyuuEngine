# src/CMakeLists.txt
project(Engine)

if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED)
else()
    add_library(${PROJECT_NAME} STATIC)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            BUILD_STATIC_LIBS
    )
endif()

collect_sources(${ENGINE_SOURCE_DIR}/input "input"
    input_sources
)

collect_sources(${ENGINE_SOURCE_DIR}/rhi "RHI"
    rhi_sources
)

collect_cxx_modules(${ENGINE_SOURCE_DIR}/rhi "RHI"
    rhi_modules 
    rhi_module_impls
)

target_sources(${PROJECT_NAME}
    PUBLIC 
        ${public_headers}
    PUBLIC FILE_SET CXX_MODULES FILES 
        ${rhi_modules}
    PRIVATE
	${input_sources}
        ${rhi_module_impls}
        ${rhi_sources}
        ${CMAKE_CURRENT_SOURCE_DIR}/core/main.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_ROOT_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

disable_rtti(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        plastic
        fyuu_rhi
        imgui::imgui
        Boost::uuid
        Eigen3::Eigen
        Jolt::Jolt
        spdlog::spdlog
)