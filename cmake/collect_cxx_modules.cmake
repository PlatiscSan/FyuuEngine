function(collect_cxx_modules base_dir group_name)

    file(GLOB_RECURSE modules 
        "${base_dir}/*.ixx"
        "${base_dir}/*.cppm"
        "${base_dir}/*.mpp"
        "${base_dir}/*.cxxm"
        "${base_dir}/*.mxx"
    )
    
    file(GLOB_RECURSE impls 
        "${base_dir}/*.impl.cpp"
        "${base_dir}/*.impl.cxx"
        "${base_dir}/*.impl.cc"
    )
    
    if(modules)
        source_group("module\\${group_name}" FILES ${modules})
    endif()
    
    if(impls)
        source_group("implementation\\${group_name}" FILES ${impls})
    endif()
    
    set(all_files ${modules} ${impls})
    
    set(${ARGV2} ${modules} PARENT_SCOPE)
    set(${ARGV3} ${impls} PARENT_SCOPE)

    set(${ARGV4} ${all_files} PARENT_SCOPE)
endfunction()