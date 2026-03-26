#!/usr/bin/env python3
"""
Unified Enum and Flag Generator
Generates C and C++ module interfaces from unified configuration
"""

import sys
import json
import argparse
from datetime import datetime
from pathlib import Path
import re

MODULE_EXTENSIONS = [".cppm", ".ixx", ".mxx"]
HEADER_EXTENSIONS = [".h", ".hpp"]

class CodeGenerator:
	"""Base generator with shared functionality"""
	
	def __init__(self, config):
		self.config = config
	
	def write_file(self, filename, content, output_dir):
		"""Write content to a file"""
		path = Path(output_dir) / filename
		path.parent.mkdir(parents=True, exist_ok=True)
		path.write_text(content, encoding="utf-8")
		return path
	
	def ensure_extension(self, filename, extensions):
		"""Ensure filename has proper extension"""
		if any(filename.endswith(ext) for ext in extensions):
			return filename
		base = filename.rsplit('.', 1)[0] if '.' in filename else filename
		return base + extensions[0]
	
	def file_header(self, description):
		"""Generate file header comment"""
		return f"""/*
 * Auto-generated {description}
 * Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * Version: {self.config.get('version', '1.0')}
 */\n\n"""
	
	def full_name(self, name, namespace):
		if not name or name == "":
			return None
		if not namespace:
			return name
		prefix = ''.join(word.capitalize() for word in re.split(r'[:_]+', namespace))
		return f"{prefix}_{name}"
	

class CHeaderGenerator(CodeGenerator):
	"""Generate C header with inline implementations"""
	
	def generate(self, output_dir):
		"""Generate C header file"""
		config = self.config
		header_name = config['c_interface']['file']
		filename = self.ensure_extension(header_name, HEADER_EXTENSIONS)
		namepace = config['namespace']
		
		content = self.file_header("C enum/flag header")
		content += "#pragma once\n\n"
		content += self._generate_includes()
		content += self._generate_enums(namepace)
		content += self._generate_flags(namepace)
		
		self.write_file(filename, content, output_dir)
		return filename
	
	def _generate_includes(self):
		"""Generate C includes"""
		includes = self.config['c_interface'].get('includes', [])
		content = """
#if defined(__cplusplus)
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

"""
		for inc in includes:
			if inc.startswith('<') and inc.endswith('>'):
				content += f"#include {inc}\n"
			else:
				content += f'#include "{inc}"\n'
		return content + "\n"
	
	def _generate_enums(self, namespace):
		"""Generate C enum declarations"""
		content = ""
		for enum in self.config.get('enums', []):
			name = self.full_name(enum['name'], namespace)
			if enum.get('description'):
				content += f"/* {enum['description']} */\n"
			
			content += f"typedef enum {name} {{\n"
			content += f"\t{name}_Unknown = 0,\n"
			
			for i, elem in enumerate(enum['elements'], 1):
				content += f"\t{name}_{elem} = {i},\n"
			
			content += f"}} {name};\n\n"
		return content
	
	def _generate_flags(self, namespace):
		"""Generate C flag declarations with inline functions"""
		content = ""
		for flag in self.config.get('flags', []):
			name = self.full_name(flag['name'], namespace)
			
			if flag.get('description'):
				content += f"/* {flag['description']} */\n"
			
			content += f"typedef enum {name} {{\n"
			content += f"\t{name}_None = 0,\n"
			
			for i, bit in enumerate(flag['bits']):
				content += f"\t{name}_{bit} = {1 << i},\n"
			
			all_val = (1 << len(flag['bits'])) - 1
			content += f"\t{name}_All = {all_val},\n"
			content += f"}} {name};\n\n"
			
			content += self._generate_flag_functions(name)
		
		return content
	
	def _generate_flag_functions(self, name):
		"""Generate inline C functions for flag operations"""
		functions = f"/* Inline functions for {name} */\n"
		
		# Basic operations
		functions += f"static inline {name} {name}_Combine({name} a, {name} b) {{\n"
		functions += f"\treturn ({name})((uint32_t)a | (uint32_t)b);\n}}\n\n"
		
		functions += f"static inline {name} {name}_Add({name} flags, {name} flag) {{\n"
		functions += f"\treturn ({name})((uint32_t)flags | (uint32_t)flag);\n}}\n\n"
		
		functions += f"static inline {name} {name}_Remove({name} flags, {name} flag) {{\n"
		functions += f"\treturn ({name})((uint32_t)flags & ~(uint32_t)flag);\n}}\n\n"
		
		functions += f"static inline bool {name}_Has({name} flags, {name} flag) {{\n"
		functions += f"\treturn ((uint32_t)flags & (uint32_t)flag) != 0;\n}}\n\n"
		
		functions += f"static inline {name} {name}_Set({name} flags, {name} flag, bool enable) {{\n"
		functions += f"\treturn enable ? {name}_Add(flags, flag) : {name}_Remove(flags, flag);\n}}\n\n"
		
		# Convenience functions
		functions += f"static inline {name} {name}_Toggle({name} flags, {name} flag) {{\n"
		functions += f"\treturn ({name})((uint32_t)flags ^ (uint32_t)flag);\n}}\n\n"
		
		functions += f"static inline {name} {name}_Clear({name} flags) {{\n"
		functions += f"\treturn ({name})0;\n}}\n\n"
		
		return functions


class CppModuleGenerator(CodeGenerator):
	"""Generate C++ module interface"""
	
	def generate(self, output_dir):
		"""Generate C++ module interface file"""
		module = self.config['interface_module']
		filename = self.ensure_extension(module['file'], MODULE_EXTENSIONS)
		
		content = self.file_header("C++ enum/flag module")
		content += f"export module {module['name']};\n\n"
		
		imports = module.get('imports', [])
		for imp in imports:
			content += f"import {imp};\n"
		if imports:
			content += "\n"
		
		namespace = self.config.get('namespace', '')
		if namespace:
			content += f"namespace {namespace} {{\n\n"
		
		content += self._generate_cpp_enums()
		content += self._generate_cpp_flags()
		
		if namespace:
			content += f"}} // namespace {namespace}\n"
		
		self.write_file(filename, content, output_dir)
		return filename
	
	def _generate_cpp_enums(self):
		"""Generate C++ enum classes"""
		content = ""
		for enum in self.config.get('enums', []):
			ns = enum.get('namespace', '')
			if ns and ns != self.config.get('namespace', ''):
				content += f"namespace {ns} {{\n\n"
			
			if enum.get('description'):
				content += f"\t/* {enum['description']} */\n"
			
			content += f"\texport enum class {enum['name']} : std::uint32_t {{\n"
			content += f"\t\tUnknown = 0,\n"
			for i, elem in enumerate(enum['elements'], 1):
				content += f"\t\t{elem} = {i},\n"
			content += f"\t}};\n\n"
			
			if ns and ns != self.config.get('namespace', ''):
				content += f"}} // namespace {ns}\n\n"
		
		return content
	
	def _generate_cpp_flags(self):
		"""Generate C++ flag enum classes with operators"""
		content = ""
		for flag in self.config.get('flags', []):
			ns = flag.get('namespace', '')
			name = flag['name']
			
			if ns and ns != self.config.get('namespace', ''):
				content += f"namespace {ns} {{\n\n"
			
			if flag.get('description'):
				content += f"\t/* {flag['description']} */\n"
			
			content += f"\texport enum class {name} : std::uint32_t {{\n"
			content += f"\t\tNone = 0,\n"
			for i, bit in enumerate(flag['bits']):
				content += f"\t\t{bit} = {1 << i},\n"
			
			all_val = (1 << len(flag['bits'])) - 1
			content += f"\t\tAll = {all_val},\n"
			content += f"\t}};\n\n"
			
			content += self._generate_cpp_flag_operators(name)
			
			if ns and ns != self.config.get('namespace', ''):
				content += f"}} // namespace {ns}\n\n"
		
		return content
	
	def _generate_cpp_flag_operators(self, name):
		"""Generate C++ operators and functions for flags"""
		ops = f"\t/* Operators for {name} */\n\n"
		
		# Operators
		ops += f"\texport constexpr {name} operator|({name} a, {name} b) noexcept {{\n"
		ops += f"\t\treturn static_cast<{name}>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));\n\t}}\n\n"
		
		ops += f"\texport constexpr {name} operator&({name} a, {name} b) noexcept {{\n"
		ops += f"\t\treturn static_cast<{name}>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));\n\t}}\n\n"
		
		ops += f"\texport constexpr {name} operator^({name} a, {name} b) noexcept {{\n"
		ops += f"\t\treturn static_cast<{name}>(static_cast<std::uint32_t>(a) ^ static_cast<std::uint32_t>(b));\n\t}}\n\n"
		
		ops += f"\texport constexpr {name} operator~({name} a) noexcept {{\n"
		ops += f"\t\treturn static_cast<{name}>(~static_cast<std::uint32_t>(a));\n\t}}\n\n"
		
		# Compound operators
		ops += f"\texport {name}& operator|=({name}& a, {name} b) noexcept {{\n"
		ops += f"\t\ta = a | b;\n\t\treturn a;\n\t}}\n\n"
		
		ops += f"\texport {name}& operator&=({name}& a, {name} b) noexcept {{\n"
		ops += f"\t\ta = a & b;\n\t\treturn a;\n\t}}\n\n"
		
		ops += f"\texport {name}& operator^=({name}& a, {name} b) noexcept {{\n"
		ops += f"\t\ta = a ^ b;\n\t\treturn a;\n\t}}\n\n"
		
		# Utility functions
		ops += f"\texport constexpr {name} Combine({name} a, {name} b) noexcept {{\n"
		ops += f"\t\treturn a | b;\n\t}}\n\n"
		
		ops += f"\texport constexpr {name} Add({name} flags, {name} flag) noexcept {{\n"
		ops += f"\t\treturn flags | flag;\n\t}}\n\n"
		
		ops += f"\texport constexpr {name} Remove({name} flags, {name} flag) noexcept {{\n"
		ops += f"\t\treturn flags & ~flag;\n\t}}\n\n"
		
		ops += f"\texport constexpr bool Has({name} flags, {name} flag) noexcept {{\n"
		ops += f"\t\treturn (flags & flag) != {name}::None;\n\t}}\n\n"
		
		ops += f"\texport constexpr {name} Set({name} flags, {name} flag, bool enable) noexcept {{\n"
		ops += f"\t\treturn enable ? Add(flags, flag) : Remove(flags, flag);\n\t}}\n\n"
		
		return ops


def parse_config(config_file):
	"""Parse JSON configuration file"""
	with open(config_file, 'r', encoding='utf-8') as f:
		return json.load(f)


def generate_template(output_file):
	"""Generate template configuration file"""
	template = {
		"version": "1.0",
		"description": "Enum and Flag Configuration",
		"namespace": "MyNamespace",
		"c_interface": {
			"file": "enums",
			"includes": []
		},
		"interface_module": {
			"name": "Enums",
			"file": "enums",
			"description": "C++ module interface",
			"imports": ["std"]
		},
		"enums": [
			{
				"name": "Result",
				"description": "Operation results",
				"elements": ["Success", "Failed", "Invalid"]
			}
		],
		"flags": [
			{
				"name": "Options",
				"description": "Configuration options",
				"bits": ["Enabled", "Visible", "Editable"]
			}
		]
	}
	
	Path(output_file).parent.mkdir(parents=True, exist_ok=True)
	with open(output_file, 'w', encoding='utf-8') as f:
		json.dump(template, f, indent=2)
	
	print(f"Template created: {output_file}")


def main():
	"""Main entry point"""
	parser = argparse.ArgumentParser(
		description="Unified Enum and Flag Generator",
		formatter_class=argparse.RawDescriptionHelpFormatter,
		epilog="""
Examples:
  %(prog)s --generate-template config.json
  %(prog)s --config config.json --output-dir ./generated
  %(prog)s --config config.json --header --module
  %(prog)s --config config.json --header ./include --module ./module
		"""
	)
	
	# Primary operation modes
	group = parser.add_mutually_exclusive_group(required=True)
	group.add_argument("-c", "--config", help="Configuration file")
	group.add_argument("-g", "--generate-template", metavar="FILE", 
					  help="Generate template configuration file")
	
	# Output options
	parser.add_argument("-o", "--output-dir", default=".", 
					   help="Default output directory for generated files")
	
	# Generation options (like original script)
	parser.add_argument("--header", nargs="?", const=".",
					   help="Generate C header file (optional: specify output directory)")
	parser.add_argument("--module", nargs="?", const=".",
					   help="Generate C++ module interface file (optional: specify output directory)")
	
	parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
	parser.add_argument("--dry-run", action="store_true", 
					   help="Show what files would be generated without actually creating them")
	
	args = parser.parse_args()
	
	# Generate template mode
	if args.generate_template:
		generate_template(args.generate_template)
		return 0
	
	# Configuration mode
	if not args.config:
		print("Error: --config required", file=sys.stderr)
		return 1
	
	# Load configuration
	try:
		config = parse_config(args.config)
		if args.verbose:
			print(f"Loaded config: {args.config}")
			print(f"  Namespace: {config.get('namespace', 'none')}")
			print(f"  Enums: {len(config.get('enums', []))}")
			print(f"  Flags: {len(config.get('flags', []))}")
	except Exception as e:
		print(f"Error loading config: {e}", file=sys.stderr)
		return 1
	
	# Determine what to generate and output directories
	if args.header is None and args.module is None:
		# Default: generate everything to default output directory
		generate_header = True
		generate_module = True
		header_output = args.output_dir
		module_output = args.output_dir
		
		if args.verbose:
			print(f"Generating both header and module to: {args.output_dir}")
	else:
		generate_header = args.header is not None
		generate_module = args.module is not None
		
		header_output = args.header if args.header is not None else args.output_dir
		module_output = args.module if args.module is not None else args.output_dir
		
		if args.verbose:
			if generate_header:
				print(f"Generating header to: {header_output}")
			if generate_module:
				print(f"Generating module to: {module_output}")
	
	# Create output directories if needed (not in dry-run)
	if not args.dry_run:
		for output_dir in [header_output, module_output]:
			if output_dir:
				Path(output_dir).mkdir(parents=True, exist_ok=True)
	
	# Generate files
	generated = []
	
	try:
		if generate_header:
			cgen = CHeaderGenerator(config)
			header_file = cgen.ensure_extension(config['c_interface']['file'], HEADER_EXTENSIONS)
			
			if args.dry_run:
				print(f"Would generate C header: {Path(header_output) / header_file}")
			else:
				file_path = cgen.generate(header_output)
				if args.verbose:
					print(f"Generated C header: {file_path}")
				generated.append(str(file_path))
		
		if generate_module:
			mgen = CppModuleGenerator(config)
			module_file = mgen.ensure_extension(config['interface_module']['file'], MODULE_EXTENSIONS)
			
			if args.dry_run:
				print(f"Would generate C++ module: {Path(module_output) / module_file}")
			else:
				file_path = mgen.generate(module_output)
				if args.verbose:
					print(f"Generated C++ module: {file_path}")
				generated.append(str(file_path))
		
		# Summary
		if args.dry_run:
			print(f"\nDry run complete. Would generate {generate_header + generate_module} files.")
		elif generated:
			# Always show summary even without verbose
			if len(generated) == 1:
				print(f"Generated: {generated[0]}")
			else:
				print(f"Generated {len(generated)} files:")
				for file in generated:
					print(f"  {file}")
		
		return 0
		
	except Exception as e:
		print(f"\n✗ Error during generation: {e}", file=sys.stderr)
		if args.verbose:
			import traceback
			traceback.print_exc()
		return 1

if __name__ == "__main__":
	sys.exit(main())