function(collect_cxx_modules base_dir group_name)

    file(GLOB_RECURSE all_module_files 
        "${base_dir}/*.ixx"
        "${base_dir}/*.cppm"
        "${base_dir}/*.mpp"
        "${base_dir}/*.cxxm"
        "${base_dir}/*.mxx"
    )
    
    file(GLOB_RECURSE all_impl_files 
        "${base_dir}/*.impl.cpp"
        "${base_dir}/*.impl.cxx"
        "${base_dir}/*.impl.cc"
    )
    
    set(modules_by_dir "")
    set(impls_by_dir "")
    
    foreach(module_file ${all_module_files})
        file(RELATIVE_PATH rel_path ${base_dir} ${module_file})
        get_filename_component(dir_path ${rel_path} DIRECTORY)
        
        if(dir_path STREQUAL "")
            set(dir_group "${group_name}")
        else()
            set(dir_group "${group_name}\\${dir_path}")
        endif()
        
        source_group("module\\${dir_group}" FILES ${module_file})
        list(APPEND modules_by_dir ${module_file})
    endforeach()
    
    foreach(impl_file ${all_impl_files})
        file(RELATIVE_PATH rel_path ${base_dir} ${impl_file})
        get_filename_component(dir_path ${rel_path} DIRECTORY)
        
        if(dir_path STREQUAL "")
            set(dir_group "${group_name}")
        else()
            set(dir_group "${group_name}\\${dir_path}")
        endif()
        
        source_group("implementation\\${dir_group}" FILES ${impl_file})
        list(APPEND impls_by_dir ${impl_file})
    endforeach()
    
    set(all_files ${modules_by_dir} ${impls_by_dir})
    
    set(${ARGV2} ${modules_by_dir} PARENT_SCOPE)
    set(${ARGV3} ${impls_by_dir} PARENT_SCOPE)
    set(${ARGV4} ${all_files} PARENT_SCOPE)

endfunction()