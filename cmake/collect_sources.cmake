function(collect_sources base_dir group_name)

    file(GLOB_RECURSE sources
        "${base_dir}/*.cpp"
        "${base_dir}/*.cxx"
        "${base_dir}/*.cc"
        "${base_dir}/*.c"
        "${base_dir}/*.cppm"
        "${base_dir}/*.c++"
        "${base_dir}/*.C"
    )
    
    set(all_sources ${sources})
    
    if(all_sources)
        source_group("Source\\${group_name}" FILES ${all_sources})
    endif()
    
    if(ARGV2)
        set(${ARGV2} ${all_sources} PARENT_SCOPE)
    endif()

endfunction()