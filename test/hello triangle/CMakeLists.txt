
project(hello_triangle)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE 
        hello_triangle.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Engine
)

target_link_directories(${PROJECT_NAME}
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            BUILD_STATIC_LIBS
    )
endif()