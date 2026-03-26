# test/hello triangle/CMakeLists.txt
project(HelloTriangle)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE 
        main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        FyuuEngine
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

disable_rtti(${PROJECT_NAME})