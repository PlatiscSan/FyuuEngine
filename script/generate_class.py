import sys
import argparse
from datetime import datetime
from typing import Dict, List, Callable, Any
from dataclasses import dataclass, field
from pathlib import Path
from dataclasses_json import dataclass_json
from enum import IntEnum, StrEnum, Enum
from collections import Counter
import json
import re
from copy import deepcopy

@dataclass_json
@dataclass
class Type:
	cpp: str = "void"
	c: str = "void"

@dataclass_json
@dataclass
class Parameter:
	tp: Type = None
	name: str = None

class FunctionType(StrEnum):
	NONE = ""
	Interface = "interface"
	Abstract = "abstract"

class Permission(StrEnum):
	Public = "public"
	Protected = "protected"
	Private = "private"

@dataclass_json
@dataclass
class FunctionInfo:
	name: str = None
	return_type: Type = field(default_factory=lambda: Type("void", "void"))
	template_params: List[Type] = field(default_factory=list)
	params: List[Parameter] = field(default_factory=list)
	function_type: FunctionType = FunctionType.NONE
	permission: Permission = Permission.Public
	is_const: bool = False
	is_no_except: bool = False
	description: str = None
	is_static: bool = False

@dataclass_json
@dataclass
class Field:
	name: str = None
	tp: Type = field(default_factory=lambda: Type("int", "int"))
	is_static: bool = False
	permission: Permission = Permission.Public
	default: str = None
	description: str = None

@dataclass_json
@dataclass
class Aliases:
	name: str = None
	tp: Type = field(default_factory=lambda: Type("int", "int"))
	permission: Permission = Permission.Public
	description: str = None

@dataclass_json
@dataclass
class BaseClass:
	name: str = None
	permission: Permission = Permission.Public

@dataclass_json
@dataclass
class ClassInfo:
	name: str = None
	base_classes: List[BaseClass] = field(default_factory=list)
	aliases: List[Aliases] = field(default_factory=list)
	fields: List[Field] = field(default_factory=list)
	functions: List[FunctionInfo] = field(default_factory=list)
	constructor: List[FunctionInfo] = field(default_factory=list)
	is_final: bool = False
	default_destructor: bool = True

@dataclass_json
@dataclass
class CInterface:
	file: str = None
	includes: List[str] = field(default_factory=list)
	imports: List[str] = field(default_factory=list)
	dll_export_macro: str = "DLL_API"
	dll_call_convention: str = "DLL_CALL"

@dataclass_json
@dataclass
class Module:
	name: str = None
	file: str = None
	description: str = None
	imports: List[str] = field(default_factory=lambda: ["std"])
	includes: List[str] = field(default_factory=list)

@dataclass_json
@dataclass
class Configuration:
	version: str = "1.0.0"
	namespace: str = None
	c_interface: CInterface = None
	module: Module = None
	classes: List[ClassInfo] = None


class ParameterTarget(IntEnum):
	Declare = 0
	Call = 1

class BaseCodeGenerator:
	conf: Configuration = None
	parameter_generator = Callable[[Parameter, List[str], List[str]], str]
	generator_strategy: Dict[tuple[bool, ParameterTarget], parameter_generator]

	def _decay(self, type_str: str) -> str:
		return type_str.replace("&", "")

	def _cpp_declare_generator(
		self,
		param: Parameter,
		template_params: List[str],
		decay_types: List[str]
	) -> str:
		typename = param.tp.cpp
		if "..." in typename:
			decay_t = self._decay(typename)
			if decay_t not in template_params:
				raise RuntimeError(
					f"Template parameter pack {decay_t} is not declared"
				)
		return f"{typename} {param.name}"

	def _cpp_call_generator(
		self,
		param: Parameter,
		template_params: List[str],
		decay_types: List[str]
	) -> str:
		typename = param.tp.cpp
		decay_t = self._decay(typename)
		
		if "..." in typename:
			if decay_t not in template_params:
				raise RuntimeError(
					f"Template parameter pack {decay_t} is not declared"
				)
			base_type = decay_t.replace("...", "")
			return f"std::forward<{base_type}>({param.name})..."
		
		if decay_t in decay_types:
			return f"std::forward<{decay_t}>({param.name})"
		return param.name

	def _c_declare_generator(
		self,
		param: Parameter,
		template_params: List[str],
		decay_types: List[str]
	) -> str:
		typename = param.tp.c
		if "..." in typename:
			return "..."
		return f"{typename} {param.name}"

	def _c_call_generator(
		self,
		param: Parameter,
		template_params: List[str],
		decay_types: List[str]
	) -> str:
		typename = param.tp.c
		if "..." in typename:
			return "..."
		return param.name

	def __init__(self, conf: Configuration):
		self.conf = conf
		self.generator_strategy = {
			(False, ParameterTarget.Declare): self._cpp_declare_generator,
			(False, ParameterTarget.Call): self._cpp_call_generator,
			(True, ParameterTarget.Declare): self._c_declare_generator,
			(True, ParameterTarget.Call): self._c_call_generator,
		}

	def _ensure_extension(self, filename: str, extensions: List[str], suffix: str = "", index: int = 0) -> str:
		"""Ensure filename has one of the specified extensions"""

		if any(filename.endswith(ext) for ext in extensions):
	
			for ext in extensions:
				if filename.endswith(ext):
					base_name = filename[:-len(ext)]
					return base_name + suffix + ext
		else:

			base_name = Path(filename).stem if "." in filename else filename

			path_obj = Path(filename)
			if path_obj.suffix and path_obj.suffix not in extensions:

				return base_name + suffix + extensions[index]
		

		return base_name + suffix + extensions[index]
	
	def _file_header(self, description: str) -> str:
		"""Generate file header comment"""
		return f"""/* 
 * Auto-generated: {description}
 * Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * Version: {self.conf.version}
 */
 
"""

	def _parameter(self, function: FunctionInfo, target: ParameterTarget, is_c: bool = False) -> str:
		if not function.params:
			return ""
		
		strategy_key = (is_c, target)
		if strategy_key not in self.generator_strategy:
			return ""
		
		generator = self.generator_strategy[strategy_key]
		
		template_params = []
		decay_types = []
		if not is_c and function.template_params:
			template_params = [tp.cpp for tp in function.template_params]
			decay_types = [self._decay(tp) for tp in template_params]
		
		param_strings = [
			generator(param, template_params, decay_types)
			for param in function.params
		]
		
		return ", ".join(param_strings)
	
	def _template_parameter(self, function: FunctionInfo) -> str:
		"""Build template parameter string"""
		if not function.template_params:
			return ""
		
		packet: str = None
		params = []
		for param in function.template_params:
			if "..." in param.cpp:
				packet = param.cpp
			else:
				params.append(f"class {param.cpp}")

		if packet:
			params.append(f"class... {packet.replace('...', '')}")

		return "template <" + ", ".join(params) + ">"
	
	def _import_module(self, imports: List[str]) -> str:
		"""Generate import statements"""
		if not imports:
			return ""
		
		import_list = []
		for import_stmt in imports:
			import_list.append(f"import {import_stmt};")
		return "\n".join(import_list)
	
	def _include_header(self, headers: List[str]) -> str:
		if not headers:
			return ""
		
		include_list = []
		for header in headers:
			if header.startswith("<") and header.endswith(">") or header.startswith('"') and header.endswith('"'):
				include_list.append(f"#include {header}")
			else:
				include_list.append(f'#include "{header}"')

		return "\n".join(include_list)
	
	def _write_file(self, filename: str, new_content: str, output_dir: Path) -> Path:
		"""Write content to file, preserving existing MARK sections"""
		path = output_dir / filename
		path.parent.mkdir(parents=True, exist_ok=True)
		
		if not path.exists():
			path.write_text(new_content, encoding="utf-8")
			print(f"Created: {path}")
			return path
		
		try:
			existing_content = path.read_text(encoding="utf-8")
		except Exception:
			existing_content = ""
		
		if not existing_content:
			path.write_text(new_content, encoding="utf-8")
			print(f"Updated (empty file): {path}")
			return path
		
		existing_marks = {}

		lines = existing_content.split('\n')
		i = 0
		
		while i < len(lines):
			line = lines[i]
			if line.strip().startswith('// MARK:'):
				mark_name = line.strip()[8:].strip()
				
				inner_lines = []
				i += 1
				
				while i < len(lines):
					current_line = lines[i]
					if (current_line.strip().startswith('// ENDMARK:') and 
						current_line.strip()[11:].strip() == mark_name):
						existing_marks[mark_name] = '\n'.join(inner_lines).rstrip()
						i += 1 
						break
					else:
						inner_lines.append(current_line)
						i += 1
			else:
				i += 1

		new_lines = new_content.split('\n')
		result_lines = []
		i = 0
		
		while i < len(new_lines):
			line = new_lines[i]
			stripped = line.strip()
			
			if stripped.startswith('// MARK:'):

				mark_name = stripped[8:].strip()
				
				result_lines.append(line)
				
				if mark_name in existing_marks:
					if existing_marks[mark_name]:
						for inner_line in existing_marks[mark_name].split('\n'):
							result_lines.append(inner_line)
					
					i += 1
					while i < len(new_lines):
						current_line = new_lines[i]
						current_stripped = current_line.strip()
						if (current_stripped.startswith('// ENDMARK:') and 
							current_stripped[11:].strip() == mark_name):
							result_lines.append(current_line)
							i += 1
							break
						i += 1
				else:
					i += 1
					while i < len(new_lines):
						current_line = new_lines[i]
						result_lines.append(current_line)
						current_stripped = current_line.strip()
						if (current_stripped.startswith('// ENDMARK:') and 
							current_stripped[11:].strip() == mark_name):
							i += 1
							break
						i += 1
			else:
				result_lines.append(line)
				i += 1
		
		merged_content = '\n'.join(result_lines)
		
		path.write_text(merged_content, encoding="utf-8")
		
		return path

	def _get_class_names(self) -> str :
		class_name_list = []
		for cls_ in self.conf.classes:
			class_name_list.append(cls_.name)
		return ", ".join(class_name_list)
		
	def _foreach_generate(self, func: Callable[[ClassInfo], str]) -> str:
		content_list = []
		for cls_ in self.conf.classes:
			content_list.append(func(cls_))
		return "\n".join(content_list)

	def _function_signature(self, cls_: ClassInfo, func: FunctionInfo) -> str:
		if func.is_static:
			param = self._parameter(func, ParameterTarget.Declare)
		else:
			param_list = [f"{cls_.name} const* self" if func.is_const else f"{cls_.name}* self"]
			param_ = self._parameter(func, ParameterTarget.Declare)
			if param_ != "":
				param_list.append(param_)
				
			param = ", ".join(param_list)

		return f"{func.return_type.cpp} {f"{self.conf.namespace}::" if self.conf.namespace else ""}{cls_.name}::{func.name}({param}) {"const" if func.is_const else "" } {"noexcept" if func.is_no_except else ""}"
	
	def _constructor_signature(self, cls_: ClassInfo, func: FunctionInfo) -> str:
		return f"{func.return_type.cpp} {f"{self.conf.namespace}::" if self.conf.namespace else ""}{cls_.name}::{cls_.name}{self._parameter(func, ParameterTarget.Declare)} {"noexcept" if func.is_no_except else ""}"

class CClassGenerator(BaseCodeGenerator):

	@dataclass
	class Expected:
		type_name: str
		type_def: str

	@dataclass
	class CFunction:
		signature: str
		define: str


	def __c_api_full_name(self, name: str) -> str:
		if not name or name == "":
			return None
		prefix = ''.join(word.capitalize() for word in re.split(r'[:_]+', self.conf.namespace))
		return f"{prefix}_{name}"
	
	def __init_expecteds(self):

		self.expecteds = {}
		for cls_ in self.conf.classes:
			for func in cls_.functions:
				words = re.findall(r'[a-zA-Z_]+', func.return_type.c.replace("*", "").removesuffix("_t").strip())
				expected = ''.join(word.capitalize() for word in words)
				expected = f"{self.__c_api_full_name(expected)}Expected"
				expected_typedef = f"""typedef {expected} {{ 
{f"\t{func.return_type.c} result;\n" if func.return_type.c and func.return_type.c != "void" else ""}\tuint32_t error_code; 
\tchar const* error_msg; 
}} {expected};
"""
				self.expecteds[func.return_type.c] = self.Expected(expected, expected_typedef)

			class_full_name = self.__c_api_full_name(cls_.name)
			expected = self.__c_api_full_name(f"{cls_.name}Expected")
			expected_typedef = f"""typedef {expected} {{ 
{f"\t{class_full_name}* result;\n" if func.return_type.c and func.return_type.c != "void" else ""}\tuint32_t error_code; 
\tchar const* error_msg; 
}} {expected};
"""
			self.expecteds[f"{class_full_name}*"] = self.Expected(expected, expected_typedef)

	def __init_functions(self):

		self.functions = {}
		for cls_ in self.conf.classes:
			
			overloads = Counter()

			for func in cls_.functions:
				overloads[func.name] += 1
			
			for func in cls_.functions:

				dll_export_macro = self.conf.c_interface.dll_export_macro if func.permission != Permission.Private else "static"
				dll_call_convention = self.conf.c_interface.dll_call_convention if func.permission != Permission.Private else ""

				signature = self._function_signature(cls_, func)
				return_type_c = self.expecteds[func.return_type.c].type_name
				overload = overloads[func.name]
				function_name = deepcopy(func.name)
				if overload >= 1:
					function_name = self.__c_api_full_name(f"{function_name}{overload}") if func.function_type == FunctionType.NONE else f"{function_name}{overload}"
					overloads[func.name] -= 1
				
				param: str = None
				if func.is_static:
					param = self._parameter(func, ParameterTarget.Declare, True)
				else:
					full_name = self.__c_api_full_name(cls_.name)
					param_list = []
					if func.function_type == FunctionType.NONE:
						param_list.append(f"{full_name} const* self" if func.is_const else f"{full_name}* self")
					else:
						param_list.append("void const* self" if func.is_const else "void* self")
					param_ = self._parameter(func, ParameterTarget.Declare, True)
					if param_ != "":
						param_list.append(param_)
					param = ", ".join(param_list)

				if func.function_type == FunctionType.NONE:
					self.functions[signature] = self.CFunction(f"{return_type_c} {function_name}({param})", f"{dll_export_macro} {return_type_c} {dll_call_convention} {function_name}({param}) NO_EXCEPT")
				else:
					self.functions[signature] = self.CFunction(f"{return_type_c} (*{function_name})({param})", f"{return_type_c} (*{function_name})({param})")
	
	def __init_constructors(self):

		self.constructor = {}
		for cls_ in self.conf.classes:

			overloads = Counter()

			for func in cls_.constructor:
				overloads[func.name] += 1
			
			for func in cls_.constructor:

				if func.function_type != FunctionType.NONE or func.is_const or func.is_static:
					continue

				dll_export_macro = self.conf.c_interface.dll_export_macro if func.permission != Permission.Private else "static"
				dll_call_convention = self.conf.c_interface.dll_call_convention if func.permission != Permission.Private else ""

				signature = self._constructor_signature(cls_, func)
				return_type_c = self.expecteds[f"{self.__c_api_full_name(cls_.name)}*"].type_name
				overload = overloads[func.name]
				function_name: str = ""
				if overload >= 1:
					function_name = self.__c_api_full_name(f"Create{cls_.name}{overload}")
					overloads[func.name] -= 1
				else:
					function_name = self.__c_api_full_name(f"Create{cls_.name}")
				
				param = self._parameter(func, ParameterTarget.Declare, True)

				if func.function_type == FunctionType.NONE:
					self.functions[signature] = self.CFunction(f"{return_type_c} {function_name}({param})", f"{dll_export_macro} {return_type_c} {dll_call_convention} {function_name}({param}) NO_EXCEPT")

	def __init__(self, conf):
		super().__init__(conf)
		self.__init_expecteds()
		self.__init_functions()
		self.__init_constructors()
	
	def __include(self) -> str:
		includes = """
#if defined(__cplusplus)
#include <cstddef>
#include <cstdint>
#include <cstdbool>
#include <cstdarg>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#endif
"""
		includes += self._include_header(self.conf.c_interface.includes)
		return includes

	def __macro(self) -> str:
		dll_export_macro = self.conf.c_interface.dll_export_macro
		dll_call_convention = self.conf.c_interface.dll_call_convention
		
		return f"""
#if !defined(DECLARE_DLL_API)

	#define DECLARE_DLL_API

	#if defined(_MSC_VER)
		#define DLL_COMPILER_MSVC
	#elif defined(__GNUC__)
		#define DLL_COMPILER_GCC
	#elif defined(__clang__)
		#define DLL_COMPILER_CLANG
	#endif

	#if defined(_WIN32) || defined(_WIN64)
		#define DLL_PLATFORM_WINDOWS
	#elif defined(__linux__)
		#define DLL_PLATFORM_LINUX
	#elif defined(__APPLE__) && defined(__MACH__)
		#define DLL_PLATFORM_OSX
	#endif

	#if defined(DLL_PLATFORM_WINDOWS)
		#ifdef DLL_EXPORTS
			#define {dll_export_macro} __declspec(dllexport)
		#else
			#define {dll_export_macro} __declspec(dllimport)
		#endif
		#define {dll_call_convention} __stdcall
	#else
		#if defined(DLL_COMPILER_GCC) || defined(DLL_COMPILER_CLANG)
			#ifdef DLL_EXPORTS
				#define {dll_export_macro} __attribute__((visibility("default")))
			#else
				#define {dll_export_macro}
			#endif
		#else
			#define {dll_export_macro}
		#endif
		#define {dll_call_convention}
	#endif

	#ifndef BUILD_SHADER
		#undef {dll_export_macro}
		#define {dll_export_macro}
		#undef {dll_call_convention}
		#define {dll_call_convention}
	#endif

	#ifdef __cplusplus
		#define NO_EXCEPT noexcept
	#else
		#define NO_EXCEPT
	#endif // __cplusplus

#endif
"""		

	def __class(self, cls_: ClassInfo) -> str:
		full_name = self.__c_api_full_name(cls_.name)
		protected_macro = f"{full_name}_Protected".upper()
		content = f"""typedef struct {full_name}Private {full_name}Private;
#if !defined({protected_macro})
typedef struct {full_name}Protected {full_name}Protected;
#endif // !defined({protected_macro})
"""
		
		static_fields = []
		fields = []
		
		base_class_counter: int = 1
		for base_class in cls_.base_classes:
			base_class_full_name = self.__c_api_full_name(base_class.name)
			fields.append(f"\t// base class {base_class_full_name}\n\t{base_class_full_name} base{base_class_counter};")
			base_class_counter += 1
		
		for field in cls_.fields:
			if field.permission == Permission.Public:
				if field.is_static:
					static_fields.append(f"\t// {field.description}\n\t{field.tp.c} {field.name};")
				else:
					fields.append(f"\t// {field.description}\n\t{field.tp.c} {field.name};")

		fields.append(f"\t// private fields\n\t{full_name}Private* private;")
		fields.append(f"\t// protected fields\n\t{full_name}Protected* protected;")
		fields.append(f"\t// static public fields\n\t{full_name}PublicStatic* const public_static;")
		fields.append(f"\t// user data\n\tvoid* user_data1;")
		fields.append(f"\t// user data\n\tvoid* user_data2;")
		fields.append(f"\n")

		for func in cls_.functions:
			if func.function_type == FunctionType.NONE or func.is_static or func.permission != Permission.Public:
				continue
			c_func = self.functions[self._function_signature(cls_, func)]
			fields.append(f"\t// {func.description}\n\t\t{c_func.signature}")

		content += f"""
typedef struct {full_name}PublicStatic {{
{"\n".join(static_fields)}
}} {full_name}PublicStatic;

typedef struct {full_name} {{
{"\n".join(fields)}
}} {full_name};

"""
		return content

	def __expected(self) -> str:

		expected_list = []

		for _, expected in self.expecteds.items():
			type_name: str = expected.type_name.upper()
			type_def: str = expected.type_def
			expected_list.append(
				f"""#if !defined({type_name})
#define {type_name}
{type_def}
#endif
""")
		return "\n".join(expected_list)

	def __generate_functions(
			self, 
			cls_: ClassInfo, 
			permission: Permission, 
			is_declare: bool,
			get_function: Callable[[ClassInfo], List],
			get_signature: Callable[[ClassInfo, Any], str]
			) -> str:
		dll_export_macro = self.conf.c_interface.dll_export_macro if permission != Permission.Private else "static"
		dll_call_convention = self.conf.c_interface.dll_call_convention if permission != Permission.Private else ""
		full_name = self.__c_api_full_name(cls_.name)
		
		func_list = []
		
		for func in get_function(cls_):
			if func.permission != permission or func.function_type:
				continue
				
			c_func = self.functions[get_signature(cls_, func)]
			
			if is_declare:
				func_list.append(f"\t{c_func.define};\n")
			else:
				signature = c_func.signature
				func_list.append(f"""\t{c_func.define} {{
\t\t// MARK: {signature}
\t\t// implement here
\t\t// ENDMARK: {signature}
\t}}
"""
)
		return "\n".join(func_list)

	def __functions(self, cls_: ClassInfo, permission: Permission, is_declare: bool = True) -> str:
		return self.__generate_functions(cls_, permission, is_declare, lambda cls_: cls_.functions, self._function_signature)

	def __constructors(self, cls_: ClassInfo, permission: Permission, is_declare: bool = True) -> str:
		return self.__generate_functions(cls_, permission, is_declare, lambda cls_: cls_.constructor, self._constructor_signature)

	def __destructor(self, cls_: ClassInfo, is_declare: bool = True) -> str:
		dll_export_macro = self.conf.c_interface.dll_export_macro
		dll_call_convention = self.conf.c_interface.dll_call_convention
		full_name = self.__c_api_full_name(cls_.name)
		
		if is_declare:
			return f"\t{dll_export_macro} {self.expecteds["void"].type_name} {dll_call_convention} {self.__c_api_full_name(f"Destroy{cls_.name}")}({full_name}* self) NO_EXCEPT;"

		signature = f"{self.expecteds["void"].type_name} {self.__c_api_full_name(f"Destroy{cls_.name}")}"
		return f"""\t{dll_export_macro} {self.expecteds["void"].type_name} {dll_call_convention} {self.__c_api_full_name(f"Destroy{cls_.name}")}({full_name}* self) NO_EXCEPT {{
\t\t// MARK: {signature}
\t\t// implement here
\t\t// ENDMARK: {signature}		
\t}}
"""

	def __generate_public_header(self, output_dir: Path) -> str: 
		content = f"""{self._file_header(f"This header defines the public functions and public fields for classes: {self._get_class_names()}")}
{self.__include()}
{self.__macro()}
{self._foreach_generate(self.__class)}
{self.__expected()}
#if defined(__cplusplus)
extern "C" {{
#endif // defined(__cplusplus)
{self._foreach_generate(lambda cls_: self.__constructors(cls_, Permission.Public))}
{self._foreach_generate(lambda cls_: self.__destructor(cls_))}
{self._foreach_generate(lambda cls_: self.__functions(cls_, Permission.Public))}
#if defined(__cplusplus)
}}
#endif // defined(__cplusplus)
"""
		file_name = self._ensure_extension(self.conf.c_interface.file, [".h"])
		self._write_file(file_name, content, output_dir)

		return file_name

	def __protected_field(self, cls_: ClassInfo) -> str:
		full_name = self.__c_api_full_name(cls_.name)
		protected_macro = f"{full_name}_Protected".upper()				
		static_fields = []
		fields = []

		for field in cls_.fields:
			if field.permission == Permission.Protected:
				if field.is_static:
					static_fields.append(f"\t// {field.description}\n\t{field.tp.c} {field.name};")
				else:
					fields.append(f"\t// {field.description}\n\t{field.tp.c} {field.name};")

		fields.append(f"\t// static protected fields\n\t{full_name}ProtectedStatic* const protected_static;")
		fields.append(f"\n")

		for func in cls_.functions:
			if func.function_type == FunctionType.NONE or func.is_static or func.permission != Permission.Protected:
				continue
			c_func = self.functions[self._function_signature(cls_, func)]
			fields.append(f"\t// {func.description}\n\t{c_func.signature};")	

		return f"""#if !defined({protected_macro})
typedef struct {full_name}ProtectedStatic {{
{"\n".join(static_fields)}
}} {full_name}ProtectedStatic;

typedef struct {full_name}Protected {{
{"\n".join(fields)}
}} {full_name}Protected;
#endif // #if !defined({protected_macro})
#define {protected_macro}

"""	
	
	def __private_field(self, cls_: ClassInfo) -> str:
		full_name = self.__c_api_full_name(cls_.name)			
		static_fields = []
		fields = []

		for field in cls_.fields:
			if field.permission == Permission.Private:
				if field.is_static:
					static_fields.append(f"\t// {field.description}\n\t{field.tp.c} {field.name};")
				else:
					fields.append(f"\t// {field.description}\n\t{field.tp.c} {field.name};")

		fields.append(f"\t// static private fields\n\t{full_name}PrivateStatic* const private_static;")
		fields.append(f"\n")

		for func in cls_.functions:
			if func.function_type == FunctionType.NONE or func.is_static or func.permission != Permission.Private:
				continue
			c_func = self.functions[self._function_signature(cls_, func)]
			fields.append(f"\t// {func.description}\n\t{c_func.signature};")	

		return f"""typedef struct {full_name}PrivateStatic {{
{"\n".join(static_fields)}
}} {full_name}PrivateStatic;

typedef struct {full_name}Private {{
{"\n".join(fields)}
}} {full_name}Private;
"""	

	def __generate_protected_header(self, output_dir: Path) -> str:
		content = f"""{self._file_header(f"This header defines the protected functions and protected fields for classes: {self._get_class_names()}")}
{self.__include()}
{self.__macro()}
{self.__expected()}
{self._foreach_generate(self.__protected_field)}
#if defined(__cplusplus)
extern "C" {{
#endif // defined(__cplusplus)
{self._foreach_generate(lambda cls_: self.__functions(cls_, Permission.Protected))}
#if defined(__cplusplus)
}}
#endif // defined(__cplusplus)
"""
		file_name = self._ensure_extension(self.conf.c_interface.file, [".h"], "_protected")
		self._write_file(file_name, content, output_dir)		

		return file_name

	def __generate_implementation(self, public_header: str, protected_header: str, output_dir: Path):
		content = f"""{self._file_header(f"This header defines implementation field and functions for classes: {self._get_class_names()}")}
#include "{public_header}"
#include "{protected_header}"
#if defined(__cplusplus)
{self._import_module(self.conf.c_interface.imports)}
#endif // defined(__cplusplus)
{self._foreach_generate(self.__private_field)}
#if defined(__cplusplus)
extern "C" {{
#endif // defined(__cplusplus)
{self._foreach_generate(lambda cls_: self.__constructors(cls_, Permission.Private, False))}
{self._foreach_generate(lambda cls_: self.__constructors(cls_, Permission.Protected, False))}
{self._foreach_generate(lambda cls_: self.__constructors(cls_, Permission.Public, False))}
{self._foreach_generate(lambda cls_: self.__destructor(cls_, False))}
{self._foreach_generate(lambda cls_: self.__functions(cls_, Permission.Private, False))}
{self._foreach_generate(lambda cls_: self.__functions(cls_, Permission.Protected, False))}
{self._foreach_generate(lambda cls_: self.__functions(cls_, Permission.Public, False))}
#if defined(__cplusplus)
}}
#endif // defined(__cplusplus)
"""
		self._write_file(self._ensure_extension(f"{self.conf.c_interface.file}", [".cpp", ".cxx", ".c"]), content, output_dir)

	def generate(self, header_out: Path, impl_out: Path):
		public_header = self.__generate_public_header(header_out)
		protected_header = self.__generate_protected_header(header_out)
		self.__generate_implementation(public_header, protected_header, impl_out)
				

class ClassGenerator(BaseCodeGenerator):

	def __init__(self, conf):
		super().__init__(conf)

	def __base_classes(self, cls_: ClassInfo) -> str:

		if len(cls_.base_classes) == 0:
			return ""
		
		base_class_list = []
		for base_class in cls_.base_classes:
			base_class_list.append(f"{base_class.permission} {base_class.name}")

		return "\n\t\t: " + ",\n\t\t".join(base_class_list)
	
	def __sort_by_permission(self, element_list: List, cls_: ClassInfo, func: Callable[[ClassInfo, Any], str]) -> str:
		
		if len(element_list) == 0:
			return ""
		
		list_by_permission = {
			Permission.Public: [],
			Permission.Protected: [],
			Permission.Private: []
		}
		
		for element in element_list:
			list_by_permission[element.permission].append(element)
		
		output_parts = []
		
		for permission in [Permission.Public, Permission.Protected, Permission.Private]:
			current_list = list_by_permission[permission]
			
			if current_list:
				permission_label = permission.name.lower()  # public, protected, private
				output_parts.append(f"\t{permission_label}:")
				
				for element in current_list:
					output_parts.append(func(cls_, element))
				
				if any(list_by_permission[p] for p in [Permission.Public, Permission.Protected, Permission.Private] 
					if p != permission and list_by_permission[p]):
					output_parts.append("")
		
		return "\n".join(output_parts)
		
	def __declare_function(self, cls_: ClassInfo, func: FunctionInfo) -> str :
		
		def handle_none(func: FunctionInfo) -> str :

			if func.is_static and func.is_const:
				return ""

			is_template = len(func.template_params) != 0
			signature = self._function_signature(cls_, func)

			func_declaration_list = []
			if func.description:
				func_declaration_list.append(f"\t\t// {func.description}")

			template_param = self._template_parameter(func)
			if template_param != "":
				func_declaration_list.append(f"\t\t{template_param}")

			if is_template:
				func_declaration_list.append(
					f"""\t\t{func.return_type.cpp} {func.name}({self._parameter(func, ParameterTarget.Declare)}){" const" if func.is_const else "" }{" noexcept" if func.is_no_except else ""} {{
\t\t\t// MARK: {signature}
\t\t\t// implement here
\t\t\t
\t\t\t// ENDMARK: {signature}
\t\t}}
""")
			else:
				func_declaration_list.append(f"\t\t{func.return_type.cpp} {func.name}({self._parameter(func, ParameterTarget.Declare)}){" const" if func.is_const else ""}{" noexcept" if func.is_no_except else ""};\n")

			return "\n".join(func_declaration_list)			
		
		def handle_inteface(func: FunctionInfo) -> str :

			is_template = len(func.template_params) != 0

			if is_template or func.is_static:
				return ""

			return f"""\t\t{f"// {func.description}" if func.description else ""}\n
\t\tvirtual {func.return_type.cpp} {func.name}({self._parameter(func, ParameterTarget.Declare)}){" const" if func.is_const else "" }{" noexcept" if func.is_no_except else ""};
"""
		
		def handle_abstract(func: FunctionInfo) -> str :

			is_template = len(func.template_params) != 0

			if is_template or func.is_static:
				return ""

			return f"""\t\t{f"// {func.description}" if func.description else ""}\n
\t\tvirtual {func.return_type.cpp} {func.name}({self._parameter(func, ParameterTarget.Declare)}){" const" if func.is_const else "" }{" noexcept" if func.is_no_except else ""} = 0;
"""
		
		handler = {
			FunctionType.NONE: handle_none,
			FunctionType.Interface: handle_inteface,
			FunctionType.Abstract: handle_abstract
		}

		return handler[func.function_type](func)

	def __declare_constructor(self, cls_: ClassInfo, func: FunctionInfo) -> str :
				
		def handle_none(func: FunctionInfo) -> str :

			if func.is_static or func.is_const:
				return

			is_template = len(func.template_params) != 0
			signature = self._constructor_signature(cls_, func)

			func_declaration_list = []
			if func.description:
				func_declaration_list.append(f"\t\t// {func.description}")
			
			template_param = self._template_parameter(func)
			if template_param != "":
				func_declaration_list.append(f"\t\t{template_param}")

			if is_template:
				func_declaration_list.append(f"""\t\t{cls_.name}({self._parameter(func, ParameterTarget.Declare)}){" noexcept" if func.is_no_except else ""} {{
\t\t\t// MARK: {signature}
\t\t\t// implement here
\t\t\t
\t\t\t// ENDMARK: {signature}
\t\t}}
""")
			else:
				func_declaration_list.append(f"\t\t{cls_.name}({self._parameter(func, ParameterTarget.Declare)}){" noexcept" if func.is_no_except else ""};\n")

			return "\n".join(func_declaration_list)
		
		def invalid_handler(func: FunctionInfo) -> str :
			return ""
		
		handler = {
			FunctionType.NONE: handle_none,
			FunctionType.Interface: invalid_handler,
			FunctionType.Abstract: invalid_handler
		}

		return handler[func.function_type](func)

	def __class(self, cls_: ClassInfo) -> str :

		constructor = self.__sort_by_permission(cls_.constructor, cls_, self.__declare_constructor)
		if constructor == "":
			constructor = f"\tpublic:\n\t\t{cls_.name}() = default;"

		content = f"""\texport class {cls_.name} {"final" if cls_.is_final else ""} {self.__base_classes(cls_)} {{
{self.__sort_by_permission(cls_.aliases, cls_, lambda cls_, alias : f"\t\t{f"// {alias.description}" if alias.description else ""}\n\t\tusing {alias.name} = {alias.tp.cpp};")}
{self.__sort_by_permission(cls_.fields, cls_, lambda cls_, field : f"\t\t{f"// {field.description}" if field.description else ""}\n\t\t{"static inline " if field.is_static else ""}{field.tp.cpp} {field.name} = {field.default};")}
\tprivate:
\t\t// user data
\t\tvoid* m_user_data1 = nullptr;

\t\t// user data
\t\tvoid* m_user_data2 = nullptr;

{constructor}
{self.__sort_by_permission(cls_.functions, cls_, self.__declare_function)}
\tpublic:
\t\t~{cls_.name}() noexcept{" = default" if cls_.default_destructor else ""};
\t}};
"""
		return content

	def __generate_module_file(self, output_dir: Path) :
		
		content = f"""{self._file_header(f"The module interface defines the classes: {self._get_class_names()}")}
module;
{self._include_header(self.conf.module.includes)}
export module {self.conf.module.name};
{self._import_module(self.conf.module.imports)}
{f"namespace {self.conf.namespace}" if self.conf.namespace else ""} {{
{self._foreach_generate(lambda cls_: self.__class(cls_))}
}}

"""

		self._write_file(self._ensure_extension(self.conf.module.file, [".cppm", ".ixx"]), content, output_dir)

	def __implement_functions(self, cls_: ClassInfo) -> str:

		function_list = []

		for func in cls_.functions:

			is_template = len(func.template_params) != 0
			if is_template or func.function_type == FunctionType.Abstract:
				continue

			signature = self._function_signature(cls_, func)
			function_list.append(
				f"""\t{func.return_type.cpp} {cls_.name}::{func.name}({self._parameter(func, ParameterTarget.Declare)}){" const" if func.is_const else "" }{" noexcept" if func.is_no_except else ""} {{
\t\t// MARK: {signature}
\t\t// implement here
\t\t
\t\t// ENDMARK: {signature}
\t}}

"""
			)
			
		for func in cls_.constructor:

			is_template = len(func.template_params)
			if is_template or func.function_type != FunctionType.NONE:
				continue

			signature = self._constructor_signature(cls_, func)
			function_list.append(
				f"""\t{cls_.name}::{cls_.name}({self._parameter(func, ParameterTarget.Declare)}){" noexcept" if func.is_no_except else ""} {{
\t\t// MARK: {signature}
\t\t// implement here
\t\t
\t\t// ENDMARK: {signature}
\t}}

"""
			)
			
		if not cls_.default_destructor:
			
			signature = f"{f"{self.conf.namespace}::" if self.conf.namespace else ""}{cls_.name}::~{cls_.name}() noexcept"

			function_list.append(
				f"""\t{cls_.name}::~{cls_.name}() noexcept {{
\t\t // MARK: {signature}
\t\t // implement here
\t\t
\t\t // ENDMARK: {signature}
\t}}

"""
			)

		return "\n".join(function_list)

	def __generate_implementation(self, output_dir: Path) :
		content = f"""{self._file_header(f"The file defines the implementations of classes: {self._get_class_names()}")}
module;
{self._include_header(self.conf.module.includes)}
module {self.conf.module.name};
{f"namespace {self.conf.namespace}" if self.conf.namespace else ""} {{
{self._foreach_generate(lambda cls_: self.__implement_functions(cls_))}
}}

"""

		self._write_file(self._ensure_extension(self.conf.module.file, [".impl.cpp", ".impl.cxx"]), content, output_dir)

	def generate(self, module_out: Path, impl_out: Path) :
		self.__generate_module_file(module_out)
		self.__generate_implementation(impl_out)


def load_configuration(config_file: str) -> Configuration:
	"""Load configuration from JSON file"""
	with open(config_file, 'r', encoding='utf-8') as f:
		config_dict = json.load(f)
		return Configuration.from_dict(config_dict)

def main() -> int:
	parser = argparse.ArgumentParser(
		description="Unified Interface Generator - Generates C and C++ module interfaces",
		formatter_class=argparse.RawDescriptionHelpFormatter,
	)
	
	operation_group = parser.add_mutually_exclusive_group(required=True)
	operation_group.add_argument(
		"-c", "--config", type=str, help="Path to JSON configuration file"
	)
	
	parser.add_argument(
		"-o",
		"--output-dir",
		type=str,
		default=".",
		help="Default output directory for all generated files",
	)
	
	parser.add_argument(
		"--header",
		nargs="?",
		const=".",
		default=None,
		help="Generate C header file (optional: specify output directory)",
	)
	parser.add_argument(
		"--module",
		nargs="?",
		const=".",
		default=None,
		help="Generate all module files - C++ interface and implementation module interfaces (optional: specify output directory)",
	)
	parser.add_argument(
		"--implementation",
		nargs="?",
		const=".",
		default=None,
		help="Generate all implementation files - C implementation and C++ implementation module implementation (optional: specify output directory)",
	)
	
	args = parser.parse_args()
	
	if not args.config:
		print("Error: Configuration file is required when not generating template")
		return 1
	
	# Load configuration
	try:
		config = load_configuration(args.config)
	except Exception as e:
		print(f"Error loading configuration: {e}")
		return 1
	
	# Determine output directories
	base_output_dir = Path(args.output_dir)
	header_dir = Path(args.header) if args.header else base_output_dir
	module_dir = Path(args.module) if args.module else base_output_dir
	impl_dir = Path(args.implementation) if args.implementation else base_output_dir
	
	# Generate files based on what was requested
	c_generator = CClassGenerator(config)
	c_generator.generate(header_dir, impl_dir)
	
	cpp_generator = ClassGenerator(config)
	cpp_generator.generate(module_dir, impl_dir)
	
	print(f"Generation done!")
	return 0

if __name__ == "__main__":
	sys.exit(main())