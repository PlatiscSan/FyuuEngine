cmake_minimum_required(VERSION 3.28)
project(plastic VERSION 1.0.0 LANGUAGES CXX)


# serialization libraries
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_cxx_modules.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/disable_rtti.cmake)

add_library(${PROJECT_NAME} STATIC)

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src/utility "utility" 
    utility_modules 
    utility_module_impls 
    all_utility_files
)

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src/concurrency "concurrency" 
    concurrency_modules 
    concurrency_module_impls 
    all_concurrency_files
)

#collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src/config "config" 
#    config_modules 
#    config_module_impls 
#    all_config_files
#)

if(MSVC)
    set_source_files_properties(${utility_modules} ${concurrency_modules}
        PROPERTIES
            COMPILE_OPTIONS "/interface"
    )
endif(MSVC)

target_sources(${PROJECT_NAME}
    PUBLIC
    FILE_SET CXX_MODULES FILES 
        ${utility_modules}
        ${concurrency_modules}
        #${config_modules}
    PRIVATE
        ${utility_module_impls}
        ${concurrency_module_impls}
        #${config_module_impls}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        nlohmann_json::nlohmann_json
        yaml-cpp::yaml-cpp
)

disable_rtti(${PROJECT_NAME})
