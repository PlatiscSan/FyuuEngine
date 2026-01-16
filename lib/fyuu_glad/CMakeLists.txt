cmake_minimum_required(VERSION 3.28)
project(fyuu_glad VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(fyuu_glad STATIC src/glad.c)

target_include_directories(fyuu_glad 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(fyuu_glad 
    PRIVATE 
        GLAD_GL_IMPLEMENTATION
)

install(
    TARGETS fyuu_glad
    EXPORT fyuu_glad-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(
    DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(
    EXPORT fyuu_glad-targets
    FILE fyuu_glad-config.cmake
    NAMESPACE fyuu_glad::
    DESTINATION lib/cmake/fyuu_glad
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/fyuu_glad-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/fyuu_glad-config-version.cmake
    DESTINATION lib/cmake/fyuu_glad
)