#!/usr/bin/env python3
"""
Enum and Flag Generator - Generates C and C++ module interfaces from configuration files
Supports automatic value assignment for flags
"""

import sys
import json
import os
from datetime import datetime

def parse_config(config_file):
    """Parse configuration file"""
    with open(config_file, 'r', encoding='utf-8') as f:
        config = json.load(f)
    
    return config

def generate_c_enum_header(config, output_dir):
    """Generate C language interface header file """
    
    # Get global prefix from config
    global_prefix = config.get("prefix", "")
    
    header_content = f"""/* 
 * Auto-generated enum and flag header file
 * Generation time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * Version: {config.get('version', '1.0.0')}
 * Global prefix: '{global_prefix if global_prefix else "(none)"}'
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {{
#endif

"""
    
    # Helper function to get full enum/flag type name
    def get_full_type_name(name, prefix):
        if prefix:
            return f"{prefix}{name}"
        else:
            return name
    
    # Helper function to get value name with prefix
    def get_value_name(base_name, type_name, is_flag=False):
        # Special handling for None/Unknown
        if is_flag and base_name == "None":
            return f"{type_name}_None"
        elif not is_flag and base_name == "Unknown":
            return f"{type_name}_Unknown"
        elif base_name == "All":
            return f"{type_name}_All"
        else:
            return f"{type_name}_{base_name}"
    
    # Generate enum declarations
    for enum_config in config.get("enums", []):
        enum_name = enum_config["name"]
        
        # Get prefix - use local if specified, otherwise use global
        prefix = enum_config.get("prefix", global_prefix)
        
        # Full type name includes prefix
        full_type_name = get_full_type_name(enum_name, prefix)
        
        elements = enum_config["elements"]
        
        # Add description comment if it exists
        if "description" in enum_config and enum_config["description"]:
            header_content += f"/* {enum_config['description']} */\n"
        
        header_content += f"typedef enum {full_type_name} {{\n"
        
        # Generate Unknown value (only if there are elements or explicitly requested)
        if elements or enum_config.get("include_unknown", True):
            unknown_name = get_value_name("Unknown", full_type_name, is_flag=False)
            header_content += f"    {unknown_name} = 0,\n"
        
        for i, element in enumerate(elements, 1):
            if isinstance(element, dict):
                element_name = element["name"]
                element_value = element.get("value", i)
                full_element_name = get_value_name(element_name, full_type_name, is_flag=False)
                
                if "description" in element and element["description"]:
                    header_content += f"    {full_element_name} = {element_value},  /* {element['description']} */\n"
                else:
                    header_content += f"    {full_element_name} = {element_value},\n"
            else:
                full_element_name = get_value_name(element, full_type_name, is_flag=False)
                header_content += f"    {full_element_name} = {i},\n"
        
        header_content += f"}} {full_type_name};\n\n"
    
    # Generate flag declarations - auto-assign values
    for flag_config in config.get("flags", []):
        flag_name = flag_config["name"]
        
        # Get prefix - use local if specified, otherwise use global
        prefix = flag_config.get("prefix", global_prefix)
        
        # Full type name includes prefix
        full_type_name = get_full_type_name(flag_name, prefix)
        
        bits = flag_config["bits"]
        
        # Add description comment if it exists
        if "description" in flag_config and flag_config["description"]:
            header_content += f"/* {flag_config['description']} */\n"
        
        header_content += f"typedef enum {full_type_name} {{\n"
        
        # Generate None value
        none_name = get_value_name("None", full_type_name, is_flag=True)
        header_content += f"    {none_name} = 0,\n"
        
        for i, bit in enumerate(bits):
            bit_value = 1 << i
            if isinstance(bit, dict):
                bit_name = bit["name"]
                full_bit_name = get_value_name(bit_name, full_type_name, is_flag=True)
                
                if "description" in bit and bit["description"]:
                    header_content += f"    {full_bit_name} = {bit_value},  /* {bit['description']} */\n"
                else:
                    header_content += f"    {full_bit_name} = {bit_value},\n"
            else:
                full_bit_name = get_value_name(bit, full_type_name, is_flag=True)
                header_content += f"    {full_bit_name} = {bit_value},\n"
        
        # Add common combinations (optional)
        if flag_config.get("generate_common_combinations", True):
            if len(bits) >= 2:
                # Generate All combination
                all_value = (1 << len(bits)) - 1
                all_name = get_value_name("All", full_type_name, is_flag=True)
                header_content += f"    {all_name} = {all_value},\n"
        
        header_content += f"}} {full_type_name};\n\n"
    
    header_content += """#ifdef __cplusplus
}
#endif
"""
    
    # Write to file
    output_file = os.path.join(output_dir, "fyuu_enums.h")
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    print(f"C header file generated: {output_file}")
    return output_file

def generate_cpp_module_interface(config, output_dir):
    """Generate C++ module interface file - simplified version"""
    
    module_name = config.get("cpp_module_name", "fyuu_engine")
    module_partition = config.get("cpp_module_partition", "enum")
    
    # Generate module interface file content
    module_content = f"""/* 
 * Auto-generated C++ module interface file
 * Generation time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * Version: {config.get('version', '1.0.0')}
 */

export module {module_name}:{module_partition};

import std;

namespace {module_name} {{
"""
    
    # Generate enum class definitions
    for enum_config in config.get("enums", []):
        enum_name = enum_config["name"]
        elements = enum_config["elements"]
        
        # Add description comment
        if "description" in enum_config:
            module_content += f"\n    /* {enum_config['description']} */\n"
        
        module_content += f"    export enum class {enum_name} : std::uint32_t {{\n"
        module_content += f"        Unknown = 0,\n"
        
        for i, element in enumerate(elements, 1):
            if isinstance(element, dict):
                element_name = element["name"]
                element_value = element.get("value", i)
                if "description" in element:
                    module_content += f"        {element_name} = {element_value},  // {element['description']}\n"
                else:
                    module_content += f"        {element_name} = {element_value},\n"
            else:
                module_content += f"        {element} = {i},\n"
        
        module_content += f"    }};\n"
    
    # Generate flag enum class definitions - auto-assign values
    for flag_config in config.get("flags", []):
        flag_name = flag_config["name"]
        bits = flag_config["bits"]
        
        # Add description comment
        if "description" in flag_config:
            module_content += f"\n    /* {flag_config['description']} */\n"
        
        module_content += f"    export enum class {flag_name} : std::uint32_t {{\n"
        module_content += f"        None = 0,\n"
        
        for i, bit in enumerate(bits):
            bit_value = 1 << i
            if isinstance(bit, dict):
                bit_name = bit["name"]
                if "description" in bit:
                    module_content += f"        {bit_name} = {bit_value},  // {bit['description']}\n"
                else:
                    module_content += f"        {bit_name} = {bit_value},\n"
            else:
                module_content += f"        {bit} = {bit_value},\n"
        
        # Add common combinations (optional)
        if flag_config.get("generate_common_combinations", True):
            if len(bits) >= 2:
                # Generate All combination
                all_value = (1 << len(bits)) - 1
                module_content += f"        All = {all_value},\n"
        
        module_content += f"    }};\n\n"
        
        # Generate combination operators for flags
        module_content += f"    /* {flag_name} flag operators */\n"
        
        # Bitwise OR operator
        module_content += f"    export constexpr {flag_name} operator|({flag_name} a, {flag_name} b) noexcept {{\n"
        module_content += f"        return static_cast<{flag_name}>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));\n"
        module_content += f"    }}\n\n"
        
        # Bitwise AND operator
        module_content += f"    export constexpr {flag_name} operator&({flag_name} a, {flag_name} b) noexcept {{\n"
        module_content += f"        return static_cast<{flag_name}>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));\n"
        module_content += f"    }}\n\n"
        
        # Bitwise XOR operator
        module_content += f"    export constexpr {flag_name} operator^({flag_name} a, {flag_name} b) noexcept {{\n"
        module_content += f"        return static_cast<{flag_name}>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));\n"
        module_content += f"    }}\n\n"
        
        # Bitwise NOT operator
        module_content += f"    export constexpr {flag_name} operator~({flag_name} a) noexcept {{\n"
        module_content += f"        return static_cast<{flag_name}>(~static_cast<uint32_t>(a));\n"
        module_content += f"    }}\n\n"
        
        # Compound assignment operators
        module_content += f"    export inline {flag_name}& operator|=({flag_name}& a, {flag_name} b) noexcept {{\n"
        module_content += f"        a = static_cast<{flag_name}>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));\n"
        module_content += f"        return a;\n"
        module_content += f"    }}\n\n"
        
        module_content += f"    export inline {flag_name}& operator&=({flag_name}& a, {flag_name} b) noexcept {{\n"
        module_content += f"        a = static_cast<{flag_name}>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));\n"
        module_content += f"        return a;\n"
        module_content += f"    }}\n\n"
        
        module_content += f"    export inline {flag_name}& operator^=({flag_name}& a, {flag_name} b) noexcept {{\n"
        module_content += f"        a = static_cast<{flag_name}>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));\n"
        module_content += f"        return a;\n"
        module_content += f"    }}\n\n"
        
        # Flag operation functions
        module_content += f"    /* {flag_name} flag operation functions */\n"
        
        # Check flag
        module_content += f"    export constexpr bool HasFlag({flag_name} flags, {flag_name} flag) noexcept {{\n"
        module_content += f"        return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;\n"
        module_content += f"    }}\n\n"
        
        # Set flag
        module_content += f"    export constexpr {flag_name} SetFlag({flag_name} flags, {flag_name} flag, bool enable) noexcept {{\n"
        module_content += f"        if (enable) {{\n"
        module_content += f"            return flags | flag;\n"
        module_content += f"        }} else {{\n"
        module_content += f"            return flags & ~flag;\n"
        module_content += f"        }}\n"
        module_content += f"    }}\n\n"
        
        # Add flag
        module_content += f"    export constexpr {flag_name} AddFlag({flag_name} flags, {flag_name} flag) noexcept {{\n"
        module_content += f"        return flags | flag;\n"
        module_content += f"    }}\n\n"
        
        # Remove flag
        module_content += f"    export constexpr {flag_name} RemoveFlag({flag_name} flags, {flag_name} flag) noexcept {{\n"
        module_content += f"        return flags & ~flag;\n"
        module_content += f"    }}\n\n"
        
        # Toggle flag
        module_content += f"    export constexpr {flag_name} ToggleFlag({flag_name} flags, {flag_name} flag) noexcept {{\n"
        module_content += f"        return flags ^ flag;\n"
        module_content += f"    }}\n\n"
        
        # Clear all flags
        module_content += f"    export constexpr {flag_name} ClearFlags({flag_name} flags) noexcept {{\n"
        module_content += f"        return {flag_name}::None;\n"
        module_content += f"    }}\n"
    
    module_content += f"\n}} // namespace {module_name}\n"
    
    # Write module interface file
    output_file = os.path.join(output_dir, f"{module_partition}.cppm")
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(module_content)
    
    print(f"C++ module interface file generated: {output_file}")
    return output_file

def main():
    if len(sys.argv) < 2:
        print("Usage: python generate_enum_module.py [config_file] <output_directory>")
        print("Example: python generate_enum_module.py config.json ./output")
        sys.exit(1)
    
    config_file = sys.argv[1]
    output_dir = "./output" if len(sys.argv) < 3 else sys.argv[2]
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Parse configuration
    config = parse_config(config_file)
    
    # Determine which interfaces to generate
    interfaces = config.get("interfaces", ["c", "cpp_module"])
    
    # Generate specified interfaces
    if "c" in interfaces:
        print("Generating C language interface...")
        generate_c_enum_header(config, output_dir)
    
    if "cpp_module" in interfaces:
        print("Generating C++ module interface...")
        generate_cpp_module_interface(config, output_dir)
    
    print(f"\nAll files generated to: {os.path.abspath(output_dir)}")
    print(f"Generation time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

if __name__ == "__main__":
    main()