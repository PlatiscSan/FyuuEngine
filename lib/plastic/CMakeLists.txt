cmake_minimum_required(VERSION 3.28)
project(plastic VERSION 1.0.0 LANGUAGES CXX)

find_package(nameof CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/collect_cxx_modules.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/disable_rtti.cmake)

add_library(${PROJECT_NAME} STATIC)

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src/utility "utility" 
    utility_modules 
)

collect_cxx_modules(${CMAKE_CURRENT_SOURCE_DIR}/src/concurrency "concurrency" 
    concurrency_modules 
)

target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET CXX_MODULES FILES 
        ${utility_modules}
        ${concurrency_modules}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        nameof::nameof)

disable_rtti(${PROJECT_NAME})
