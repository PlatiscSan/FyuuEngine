# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

function(recursive_group_sources root_dir target_name target_files)
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

    # 分离不同类型的文件
    set(source_files)
    set(header_files)
    set(module_interface_files)
    set(module_implementation_files)
    
    foreach(file ${all_files})
        get_filename_component(ext ${file} EXT)
        
        if(ext MATCHES "\\.(ixx|cppm|mpp|cxxm|mxx)$")
            list(APPEND module_interface_files ${file})
        elseif(ext MATCHES "\\.(impl\\.cpp)$")
            list(APPEND module_implementation_files ${file})
        elseif(ext MATCHES "\\.(h|hpp|hh|hxx)$")
            list(APPEND header_files ${file})
        else()
            list(APPEND source_files ${file})
        endif()
    endforeach()

    # 为不同类型文件设置不同的分组
    # 1. 处理模块接口文件
    foreach(file ${module_interface_files})
        file(RELATIVE_PATH rel_path ${root_dir} ${file})
        get_filename_component(group_dir ${rel_path} DIRECTORY)
        
        if(group_dir)
            string(REPLACE "/" "\\" group_name "Modules\\${group_dir}")
            source_group("${group_name}" FILES ${file})
        else()
            source_group("Modules" FILES ${file})
        endif()
    endforeach()

    # 2. 处理模块实现文件
    foreach(file ${module_implementation_files})
        file(RELATIVE_PATH rel_path ${root_dir} ${file})
        get_filename_component(group_dir ${rel_path} DIRECTORY)
        
        if(group_dir)
            string(REPLACE "/" "\\" group_name "Modules\\Implementations\\${group_dir}")
            source_group("${group_name}" FILES ${file})
        else()
            source_group("Modules\\Implementations" FILES ${file})
        endif()
    endforeach()

    # 3. 处理头文件
    foreach(file ${header_files})
        file(RELATIVE_PATH rel_path ${root_dir} ${file})
        get_filename_component(group_dir ${rel_path} DIRECTORY)
        
        if(group_dir)
            string(REPLACE "/" "\\" group_name "Headers\\${group_dir}")
            source_group("${group_name}" FILES ${file})
        else()
            source_group("Headers" FILES ${file})
        endif()
    endforeach()

    # 4. 处理普通源文件
    foreach(file ${source_files})
        file(RELATIVE_PATH rel_path ${root_dir} ${file})
        get_filename_component(group_dir ${rel_path} DIRECTORY)
        
        if(group_dir)
            string(REPLACE "/" "\\" group_name "Sources\\${group_dir}")
            source_group("${group_name}" FILES ${file})
        else()
            source_group("Sources" FILES ${file})
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
    set(${target_files} ${all_files} PARENT_SCOPE)
endfunction()

project ("FyuuEngine")

find_package(Boost REQUIRED COMPONENTS uuid)
find_package(Eigen3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_msl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_c CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)

if(WIN32)
    find_package(directx-headers CONFIG REQUIRED)
endif(WIN32)


if( ${Vulkan_VERSION} VERSION_LESS "1.3.256" )
  message( FATAL_ERROR "Minimum required Vulkan version for C++ modules is 1.3.256. "
           "Found ${Vulkan_VERSION}."
  )
endif()

if(MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_subdirectory(src/engine)
add_subdirectory(src/application)