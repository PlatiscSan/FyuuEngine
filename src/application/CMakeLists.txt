add_executable(hello_world)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/hello_world" hello_world hello_world_src)
target_sources(hello_world
    PRIVATE
        ${hello_world_src}
)
target_link_libraries(hello_world
    PRIVATE
        application
)

add_executable(editor)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/editor" editor editor_src)
target_sources(editor
    PRIVATE
        ${editor_src}
)
target_link_libraries(editor
    PRIVATE
        application
)