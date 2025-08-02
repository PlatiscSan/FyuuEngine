# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

function(recursive_group_sources root_dir target_name)
    # 定义源文件和模块文件匹配模式
    set(file_patterns
        "${root_dir}/*.[ch]"
        "${root_dir}/*.[ch]pp"
        "${root_dir}/*.cc"
        "${root_dir}/*.ixx"        # MSVC 模块接口文件
        "${root_dir}/*.cppm"       # Clang/GCC 模块接口文件
        "${root_dir}/*.mpp"        # 其他模块接口文件
        "${root_dir}/*.cxxm"       # 备用模块接口扩展
        "${root_dir}/*.mxx"        # 备用模块接口扩展
        "${root_dir}/*.impl.cpp"   # 模块实现文件
    )

    # 递归获取所有源文件
    file(GLOB_RECURSE all_files
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${file_patterns}
    )

    # 分离普通源文件和模块文件
    set(source_files)
    set(module_interface_files)
    set(module_implementation_files)
    
    foreach(file ${all_files})
        get_filename_component(ext ${file} EXT)
        
        if(ext MATCHES "\\.(ixx|cppm|mpp|cxxm|mxx)$")
            list(APPEND module_interface_files ${file})
        elseif(ext MATCHES "\\.(impl\\.cpp)$")
            list(APPEND module_implementation_files ${file})
        else()
            list(APPEND source_files ${file})
        endif()
    endforeach()

    # 处理所有文件的分组
    foreach(file ${all_files})
        file(RELATIVE_PATH rel_path ${root_dir} ${file})
        get_filename_component(group_dir ${rel_path} DIRECTORY)
        
        # 为模块文件添加特殊分组前缀
        get_filename_component(ext ${file} EXT)
        if(ext MATCHES "\\.(ixx|cppm|mpp|cxxm|mxx|impl\\.cpp)$")
            set(group_prefix "Modules\\\\")
        else()
            set(group_prefix "")
        endif()
        
        if(group_dir)
            string(REPLACE "/" "\\" group_name "${group_prefix}${group_dir}")
            source_group("${group_name}" FILES ${file})
        else()
            source_group("${group_prefix}\\" FILES ${file})
        endif()
    endforeach()

    # 设置模块编译属性（如果找到模块文件）
    if(module_interface_files OR module_implementation_files)
        # 设置 C++20 标准
        set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 20)
        set_property(TARGET ${target_name} PROPERTY CXX_STANDARD_REQUIRED ON)
        
        # 编译器特定设置
        if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            # MSVC 需要启用模块支持
            target_compile_options(${target_name} PRIVATE "/experimental:module")
            target_compile_options(${target_name} PRIVATE "/std:c++latest")
            
            
            # 获取默认输出目录（MSVC 放置 .ifc 文件的位置）
            set(module_search_dir "$<TARGET_FILE_DIR:${target_name}>/../")
            
            # 添加引用搜索路径
            target_compile_options(${target_name} PRIVATE
                "/reference $<TARGET_FILE_DIR:${target_name}>/../"
            )
            
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            # Clang/GCC 需要启用模块支持
            target_compile_options(${target_name} PRIVATE "-fmodules-ts")
            target_compile_options(${target_name} PRIVATE "-std=c++20")
            
            # 设置模块缓存目录
            set(module_cache_dir "${CMAKE_BINARY_DIR}/modules/${target_name}")
            file(MAKE_DIRECTORY ${module_cache_dir})
            target_compile_options(${target_name} PRIVATE
                "-fmodules-cache-path=${module_cache_dir}"
            )
        endif()
        
        # 将模块目录添加到包含路径
        target_include_directories(${target_name} PRIVATE ${root_dir})
    endif()

    # 返回所有文件列表
    set(all_files ${all_files} PARENT_SCOPE)
endfunction()

project ("FyuuEngine")

find_package(Boost REQUIRED COMPONENTS uuid)
find_package(Eigen3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(MyApp)
recursive_group_sources("${CMAKE_CURRENT_SOURCE_DIR}/src" MyApp)

target_sources(MyApp 
    PRIVATE 
        ${all_files}
)

target_link_libraries(MyApp 
    PRIVATE 
        Boost::uuid
        Eigen3::Eigen
        imgui::imgui
        glfw
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
        ${Vulkan_LIBRARIES}
)

target_include_directories(MyApp 
    PRIVATE 
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(MyApp 
        PRIVATE 
            d3d12
            dxgi
            d3dcompiler
            dxguid
            Opengl32
    )
endif()
       