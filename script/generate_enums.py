#!/usr/bin/env python3
"""
枚举和标志位头文件生成器
"""

import sys

def generate_simple_header(max_elements, output_file):
    """生成简化的头文件，使用简单的方法处理可变参数"""
    
    # 生成MAP宏定义
    map_macros = []
    for i in range(1, max_elements + 1):
        params = [f"x{j}" for j in range(1, i + 1)]
        calls = " ".join([f"macro(enum_name, {param})" for param in params])
        map_macros.append(f"#define MAP{i}(macro, enum_name, {', '.join(params)}) {calls}")
    
    map_macros_text = "\n".join(map_macros)
    
    # 生成FLAG_MAP宏定义
    flag_map_macros = []
    for i in range(1, max_elements + 1):
        params = [f"x{j}" for j in range(1, i + 1)]
        calls = " ".join([f"macro(enum_name, {param})" for param in params])
        flag_map_macros.append(f"#define FLAG_MAP{i}(macro, enum_name, {', '.join(params)}) {calls}")
    
    flag_map_macros_text = "\n".join(flag_map_macros)
    
    # 生成GET_MACRO参数列表
    map_params = [f"_{i}" for i in range(1, max_elements + 1)]
    map_names = [f"MAP{i}" for i in range(max_elements, 0, -1)]
    
    flag_params = [f"_{i}" for i in range(1, max_elements + 1)]
    flag_names = [f"FLAG_MAP{i}" for i in range(max_elements, 0, -1)]
    
    # 构建头文件
    header_content = f"""#pragma once

/*
*	enumeration
*/

#define FYUU_ENUM_ELEMENT(enum_name, element) element,
#define FYUU_ENUM_ELEMENT_PREFIX(enum_name, element) Fyuu##enum_name##_##element,

#define DECLARE_FYUU_ENUM_C_STYLE(enum_name, ...) \\
typedef enum Fyuu##enum_name {{ \\
	Fyuu##enum_name##_Unknown, \\
	MAP(FYUU_ENUM_ELEMENT_PREFIX, enum_name, __VA_ARGS__) \\
}} Fyuu##enum_name;

#define DECLARE_FYUU_ENUM_CPP_STYLE(enum_name, ...) \\
namespace fyuu_engine {{ \\
	enum class enum_name {{ \\
		Unknown, \\
		MAP(FYUU_ENUM_ELEMENT, enum_name, __VA_ARGS__) \\
	}}; \\
}}

#ifdef __cplusplus
#define DECLARE_FYUU_ENUM(enum_name, ...) \\
		DECLARE_FYUU_ENUM_CPP_STYLE(enum_name, __VA_ARGS__) \\
		DECLARE_FYUU_ENUM_C_STYLE(enum_name, __VA_ARGS__)
#else
#define DECLARE_FYUU_ENUM(enum_name, ...) \\
		DECLARE_FYUU_ENUM_C_STYLE(enum_name, __VA_ARGS__)
#endif

/*
*	Flag
*/
#define FYUU_FLAG_BIT_ELEMENT(enum_name, element) element,
#define FYUU_FLAG_BIT_ELEMENT_C(enum_name, element) enum_name##_##element,

#define FLAG_MAP(macro, enum_name, ...) \\
	FLAG_GET_MACRO(__VA_ARGS__, {', '.join(flag_names)})(macro, enum_name, __VA_ARGS__)

#define FLAG_GET_MACRO({', '.join(flag_params + ['NAME', '...'])}) NAME

{flag_map_macros_text}

#define FLAG_AUTO_VALUE(index) (1u << (index))

#define DECLARE_FYUU_FLAG_C_STYLE(enum_name, ...) \\
typedef enum enum_name {{ \\
	enum_name##_None = 0, \\
	FLAG_MAP(FYUU_FLAG_BIT_ELEMENT_C, enum_name, __VA_ARGS__) \\
}} enum_name;

#define DECLARE_FYUU_FLAG_CPP_STYLE(enum_name, ...) \\
namespace fyuu_engine {{ \\
	enum class enum_name {{ \\
		None = 0, \\
		FLAG_MAP(FYUU_FLAG_BIT_ELEMENT, enum_name, __VA_ARGS__) \\
	}}; \\
	\\
	inline enum_name operator|(enum_name a, enum_name b) {{ \\
		return static_cast<enum_name>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b)); \\
	}} \\
	inline enum_name operator&(enum_name a, enum_name b) {{ \\
		return static_cast<enum_name>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b)); \\
	}} \\
	inline enum_name operator^(enum_name a, enum_name b) {{ \\
		return static_cast<enum_name>(static_cast<unsigned int>(a) ^ static_cast<unsigned int>(b)); \\
	}} \\
	inline enum_name operator~(enum_name a) {{ \\
		return static_cast<enum_name>(~static_cast<unsigned int>(a)); \\
	}} \\
	inline enum_name& operator|=(enum_name& a, enum_name b) {{ \\
		a = a | b; \\
		return a; \\
	}} \\
	inline enum_name& operator&=(enum_name& a, enum_name b) {{ \\
		a = a & b; \\
		return a; \\
	}} \\
	inline enum_name& operator^=(enum_name& a, enum_name b) {{ \\
		a = a ^ b; \\
		return a; \\
	}} \\
	\\
	inline bool HasFlag(enum_name flags, enum_name flag) {{ \\
		return (static_cast<unsigned int>(flags) & static_cast<unsigned int>(flag)) != 0; \\
	}} \\
	inline enum_name SetFlag(enum_name flags, enum_name flag, bool value) {{ \\
		if (value) {{ \\
			return flags | flag; \\
		}} else {{ \\
			return flags & ~flag; \\
		}} \\
	}} \\
}}

#ifdef __cplusplus
#define DECLARE_FYUU_FLAG(enum_name, ...) \\
		DECLARE_FYUU_FLAG_CPP_STYLE(enum_name, __VA_ARGS__) \\
		DECLARE_FYUU_FLAG_C_STYLE(enum_name, __VA_ARGS__)
#else
#define DECLARE_FYUU_FLAG(enum_name, ...) \\
		DECLARE_FYUU_FLAG_C_STYLE(enum_name, __VA_ARGS__)
#endif

#define FYUU_FLAG_AUTO_ELEMENT(enum_name, element) element = FLAG_AUTO_VALUE(__COUNTER__ - 1),
#define FYUU_FLAG_AUTO_ELEMENT_C(enum_name, element) Fyuu##enum_name##_##element = FLAG_AUTO_VALUE(__COUNTER__ - 1),

#define DECLARE_FYUU_FLAG_AUTO_C_STYLE(enum_name, ...) \\
typedef enum Fyuu##enum_name {{ \\
	Fyuu##enum_name##_None = 0, \\
	FLAG_MAP(FYUU_FLAG_AUTO_ELEMENT_C, enum_name, __VA_ARGS__) \\
}} Fyuu##enum_name;

#define DECLARE_FYUU_FLAG_AUTO_CPP_STYLE(enum_name, ...) \\
namespace fyuu_engine {{ \\
	enum class enum_name {{ \\
		None = 0, \\
		FLAG_MAP(FYUU_FLAG_AUTO_ELEMENT, enum_name, __VA_ARGS__) \\
	}}; \\
	\\
	inline enum_name operator|(enum_name a, enum_name b) {{ \\
		return static_cast<enum_name>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b)); \\
	}} \\
	inline enum_name operator&(enum_name a, enum_name b) {{ \\
		return static_cast<enum_name>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b)); \\
	}} \\
	inline enum_name operator^(enum_name a, enum_name b) {{ \\
		return static_cast<enum_name>(static_cast<unsigned int>(a) ^ static_cast<unsigned int>(b)); \\
	}} \\
	inline enum_name operator~(enum_name a) {{ \\
		return static_cast<enum_name>(~static_cast<unsigned int>(a)); \\
	}} \\
	inline enum_name& operator|=(enum_name& a, enum_name b) {{ \\
		a = a | b; \\
		return a; \\
	}} \\
	inline enum_name& operator&=(enum_name& a, enum_name b) {{ \\
		a = a & b; \\
		return a; \\
	}} \\
	inline enum_name& operator^=(enum_name& a, enum_name b) {{ \\
		a = a ^ b; \\
		return a; \\
	}} \\
	\\
	inline bool HasFlag(enum_name flags, enum_name flag) {{ \\
		return (static_cast<unsigned int>(flags) & static_cast<unsigned int>(flag)) != 0; \\
	}} \\
	inline enum_name SetFlag(enum_name flags, enum_name flag, bool value) {{ \\
		if (value) {{ \\
			return flags | flag; \\
		}} else {{ \\
			return flags & ~flag; \\
		}} \\
	}} \\
}}

#ifdef __cplusplus
#define DECLARE_FYUU_FLAG_AUTO(enum_name, ...) \\
		DECLARE_FYUU_FLAG_AUTO_CPP_STYLE(enum_name, __VA_ARGS__) \\
		DECLARE_FYUU_FLAG_AUTO_C_STYLE(enum_name, __VA_ARGS__)
#else
#define DECLARE_FYUU_FLAG_AUTO(enum_name, ...) \\
		DECLARE_FYUU_FLAG_AUTO_C_STYLE(enum_name, __VA_ARGS__)
#endif

/*
*	MAP macros
*/

#define MAP(macro, enum_name, ...) \\
	GET_MACRO(__VA_ARGS__, {', '.join(map_names)})(macro, enum_name, __VA_ARGS__)

#define GET_MACRO({', '.join(map_params + ['NAME', '...'])}) NAME

{map_macros_text}
"""
    
    # 写入文件
    with open(output_file, 'w') as f:
        f.write(header_content)
    
    print(f"成功生成头文件: {output_file}")
    print(f"支持最大元素数量: {max_elements}")
    print("注意: 这个头文件使用了C预处理器的参数计数技巧，请确保在编译时使用支持该功能的编译器。")

def main():
    if len(sys.argv) != 3:
        print("用法: python generate_enum_header.py [最大元素数量] [输出文件名]")
        print("示例: python generate_enum_header.py 64 fyuu_enums.h")
        sys.exit(1)
    
    try:
        max_elements = int(sys.argv[1])
        if max_elements <= 0:
            print("错误: 最大元素数量必须是正整数")
            sys.exit(1)
    except ValueError:
        print("错误: 最大元素数量必须是整数")
        sys.exit(1)
    
    output_file = sys.argv[2]
    
    # 确保输出文件扩展名为.h
    if not output_file.endswith('.h'):
        output_file += '.h'
    
    generate_simple_header(max_elements, output_file)

if __name__ == "__main__":
    main()