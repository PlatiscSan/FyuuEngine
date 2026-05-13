#!/usr/bin/env python3
"""
Cross-platform CMake interactive build script with last-config quick build.
"""

import curses
import subprocess
import sys
import os
import json
import platform

# ---------- Detect current system ----------
SYSTEM = platform.system()  # 'Windows', 'Linux', 'Darwin'

# ---------- Configuration mappings ----------
GENERATOR_MAP = {
	"Ninja": "Ninja",
	"Unix Makefiles": "Unix Makefiles",
	"Xcode": "Xcode",
	"Visual Studio 2026": "Visual Studio 18 2026",
	"Visual Studio 2022": "Visual Studio 17 2022",
	"Visual Studio 2019": "Visual Studio 16 2019",
	"Visual Studio 2017": "Visual Studio 15 2017",
}

COMPILER_MAP = {
	"Clang": {"CC": "clang", "CXX": "clang++"},
	"GCC": {"CC": "gcc", "CXX": "g++"},
	"MSVC": {"CC": "cl", "CXX": "cl"},
}

PLATFORM_MAP = {
	"x86": "Win32",
	"x64": "x64",
	"ARM64": "ARM64",
	"ARM32": "ARM",
}

BUILD_TYPES = ["Release", "Debug", "RelWithDebInfo", "MinSizeRel"]

TOOLCHAIN_OPTIONS = ["None", "vcpkg"]

LAST_CONFIG_FILE = os.path.join(os.path.dirname(__file__), '.last_build_config.json')

# ---------- Helper functions ----------
def safe_dirname(s):
	"""Replace characters that are problematic in directory names."""
	return s.replace(' ', '_').replace('/', '_').replace('\\', '_')

def save_last_config(config):
	"""Save configuration to file."""
	try:
		with open(LAST_CONFIG_FILE, 'w') as f:
			json.dump(config, f, indent=2)
	except Exception as e:
		print(f"Warning: Could not save last config: {e}")

def load_last_config():
	"""Load configuration from file, return None if not exists or invalid."""
	if os.path.exists(LAST_CONFIG_FILE):
		try:
			with open(LAST_CONFIG_FILE, 'r') as f:
				config = json.load(f)
			required_keys = ['generator', 'compiler', 'platform', 'build_type', 'toolchain']
			if all(k in config for k in required_keys):
				return config
			else:
				print("Last config file is incomplete, ignoring.")
		except Exception as e:
			print(f"Error reading last config: {e}")
	return None

def get_vcpkg_path():
	"""Interactively get vcpkg installation path. Returns path string or None if cancelled."""
	vcpkg_root = os.environ.get("VCPKG_ROOT")
	if vcpkg_root and os.path.isdir(vcpkg_root):
		print(f"Found VCPKG_ROOT environment variable: {vcpkg_root}")
		use_env = input("Use this path? (y/n): ").strip().lower()
		if use_env == 'y':
			return vcpkg_root
	print("Enter vcpkg installation directory, or leave empty / type 'q' to cancel.")
	while True:
		path = input("vcpkg path (e.g., C:/dev/vcpkg or /home/user/vcpkg): ").strip()
		if path == '' or path.lower() == 'q':
			return None
		if os.path.isdir(path):
			return path
		print("Invalid path, please try again.")

def get_vcpkg_triplet(system, platform):
	"""Return vcpkg triplet based on system and platform."""
	arch_map = {
		'x86': 'x86',
		'x64': 'x64',
		'ARM32': 'arm',
		'ARM64': 'arm64'
	}
	arch = arch_map.get(platform, 'x64')
	if system == 'Windows':
		return f"{arch}-windows"
	elif system == 'Linux':
		return f"{arch}-linux"
	elif system == 'Darwin':
		return f"{arch}-osx"
	else:
		return f"{arch}-{system.lower()}"

def get_build_dir(config):
	"""Generate a unique build directory based on configuration."""
	base_dir = os.path.join(os.getcwd(), "cmake-build")
	if config.get('compiler') == 'MSVC':
		# MSVC: flat directory with MSVC prefix (no platform/build_type)
		dir_name = f"MSVC_{safe_dirname(config['generator'])}"
		if config.get('toolchain') == 'vcpkg':
			dir_name += "_vcpkg"
		return os.path.join(base_dir, dir_name)
	else:
		# Other compilers: hierarchical structure
		components = [
			safe_dirname(config['generator']),
			safe_dirname(config['compiler']),
			safe_dirname(config['platform']),
			safe_dirname(config['build_type'])
		]
		if config.get('toolchain') == 'vcpkg':
			components.append('vcpkg')
		return os.path.join(base_dir, *components)

def run_cmake(config):
	"""Run CMake configuration and build."""
	build_dir = get_build_dir(config)
	os.makedirs(build_dir, exist_ok=True)

	cmake_cmd = ["cmake", "-B", build_dir]

	# Generator
	generator = config['generator']
	if generator not in GENERATOR_MAP:
		print(f"Error: Unsupported generator '{generator}'")
		return False
	cmake_cmd.extend(["-G", GENERATOR_MAP[generator]])

	# Platform architecture (only relevant for Visual Studio and Xcode)
	platform_choice = config['platform']
	if generator.startswith("Visual Studio"):
		vs_arch = PLATFORM_MAP.get(platform_choice, platform_choice)
		cmake_cmd.extend(["-A", vs_arch])
	elif generator == "Xcode" and SYSTEM == 'Darwin':
		# Set target architecture for Xcode
		xcode_arch_map = {'x64': 'x86_64', 'x86': 'i386', 'ARM64': 'arm64'}
		if platform_choice in xcode_arch_map:
			cmake_cmd.append(f"-DCMAKE_OSX_ARCHITECTURES={xcode_arch_map[platform_choice]}")
	# Other generators do not automatically add architecture flags

	# Compiler
	compiler = config['compiler']
	if compiler == "MSVC" and SYSTEM != 'Windows':
		print("Error: MSVC compiler can only be used on Windows.")
		return False
	if compiler != "MSVC":
		comp = COMPILER_MAP.get(compiler)
		if comp:
			cmake_cmd.append(f"-DCMAKE_C_COMPILER={comp['CC']}")
			cmake_cmd.append(f"-DCMAKE_CXX_COMPILER={comp['CXX']}")

	# Build type
	build_type = config['build_type']
	cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={build_type}")

	# vcpkg toolchain
	if config.get('toolchain') == 'vcpkg':
		vcpkg_root = config.get('vcpkg_root')
		if vcpkg_root:
			toolchain_file = os.path.join(vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake")
			if os.path.isfile(toolchain_file):
				cmake_cmd.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
				triplet = get_vcpkg_triplet(SYSTEM, platform_choice)
				cmake_cmd.append(f"-DVCPKG_TARGET_TRIPLET={triplet}")
				print(f"Using vcpkg toolchain: {toolchain_file}, triplet: {triplet}")
			else:
				print(f"Warning: vcpkg toolchain file not found at {toolchain_file}, skipping.")
		else:
			print("Warning: vcpkg selected but no path provided, skipping.")

	print("=" * 60)
	print(f"Build directory: {build_dir}")
	print("Running CMake configuration...")
	print(" ".join(cmake_cmd))
	result = subprocess.run(cmake_cmd, cwd=os.getcwd())
	if result.returncode != 0:
		print("CMake configuration failed.")
		return False

	# Build for all compilers (including MSVC)
	build_cmd = ["cmake", "--build", build_dir, "--config", build_type]
	print("\nRunning build...")
	print(" ".join(build_cmd))
	result = subprocess.run(build_cmd, cwd=os.getcwd())
	if result.returncode != 0:
		print("Build failed.")
		return False

	print("Build succeeded!")
	save_last_config(config)
	return True

# ---------- curses menu functions ----------
def draw_menu(stdscr, title, options, current_row):
	stdscr.clear()
	h, w = stdscr.getmaxyx()
	start_y = max(0, h // 2 - len(options) // 2 - 2)
	start_x = max(0, w // 2 - max(len(title), max(len(o) for o in options)) // 2)

	stdscr.addstr(start_y, start_x, title, curses.A_BOLD)

	for idx, opt in enumerate(options):
		y = start_y + 2 + idx
		x = start_x
		if y >= h:
			break
		if idx == current_row:
			stdscr.attron(curses.A_REVERSE)
			stdscr.addstr(y, x, opt[:w-x])
			stdscr.attroff(curses.A_REVERSE)
		else:
			stdscr.addstr(y, x, opt[:w-x])

	stdscr.refresh()

def menu(stdscr, title, options):
	curses.curs_set(0)
	current_row = 0

	while True:
		try:
			draw_menu(stdscr, title, options, current_row)
		except curses.error:
			stdscr.clear()
			stdscr.addstr(0, 0, "Terminal window too small, please enlarge and press any key...")
			stdscr.getch()
			continue

		key = stdscr.getch()
		if key == curses.KEY_UP and current_row > 0:
			current_row -= 1
		elif key == curses.KEY_DOWN and current_row < len(options) - 1:
			current_row += 1
		elif key == ord('\n'):
			return options[current_row]

def reset_following(config, steps, start_key):
	"""Reset config values from start_key onward (inclusive) to None."""
	keys_order = [k for k, _ in steps]
	try:
		idx = keys_order.index(start_key)
	except ValueError:
		return
	for k in keys_order[idx:]:
		config[k] = None
	# Also remove vcpkg_root if toolchain is cleared
	if 'toolchain' in keys_order[idx:] and config.get('toolchain') is None:
		config.pop('vcpkg_root', None)

def main(stdscr):
	# Generate compiler options dynamically based on system
	compiler_options = list(COMPILER_MAP.keys())
	if SYSTEM != 'Windows':
		compiler_options = [c for c in compiler_options if c != 'MSVC']

	# Steps definition
	steps = [
		('generator', list(GENERATOR_MAP.keys()) + ['Back']),
		('compiler', compiler_options + ['Back']),
		('platform', list(PLATFORM_MAP.keys()) + ['Back']),
		('build_type', BUILD_TYPES + ['Back']),
		('toolchain', TOOLCHAIN_OPTIONS + ['Back'])
	]

	config = {key: None for key, _ in steps}
	step_idx = 0

	while step_idx < len(steps):
		key, options = steps[step_idx]

		# --- Automatic skipping logic: only skip compiler for Visual Studio ---
		if key == 'compiler' and config.get('generator', '').startswith('Visual Studio'):
			config['compiler'] = 'MSVC'
			step_idx += 1
			continue
		# --- end of automatic skipping ---

		title = f"Select {key.capitalize()} (arrow keys, Enter to confirm)"
		choice = menu(stdscr, title, options)

		if choice == 'Back':
			if step_idx > 0:
				# step back
				step_idx -= 1
				# Skip steps that are automatically skipped under current configuration
				while step_idx >= 0:
					k, _ = steps[step_idx]
					should_skip = False
					if k == 'compiler' and config.get('generator', '').startswith('Visual Studio'):
						should_skip = True
					if should_skip:
						step_idx -= 1
					else:
						break
				# Reset all values starting from the new step_idx
				if step_idx >= 0:
					reset_following(config, steps, steps[step_idx][0])
				else:
					# Exited all steps and exited the program directly.
					return
			else:
				return
		else:
			config[key] = choice

			# Post-selection cleanup
			if key == 'generator':
				# Generator changed, reset all subsequent choices
				reset_following(config, steps, 'compiler')
			elif key == 'compiler':
				# Compiler changed, platform and build_type need to be reselected
				config['platform'] = None
				config['build_type'] = None
			elif key == 'toolchain' and choice == 'vcpkg':
				# Temporarily exit curses to get vcpkg path
				curses.endwin()
				vcpkg_path = get_vcpkg_path()
				# Re-enter curses
				stdscr = curses.initscr()
				curses.noecho()
				curses.cbreak()
				stdscr.keypad(True)

				if vcpkg_path is None:
					# User cancelled vcpkg selection, reset toolchain to None and stay on same step
					config['toolchain'] = None
					config.pop('vcpkg_root', None)
					# Do not increment step_idx, so loop repeats this step
					continue
				else:
					config['vcpkg_root'] = vcpkg_path
					# Proceed to next step
					step_idx += 1
					continue  # skip the default step_idx increment below

			step_idx += 1

	# Final confirmation
	while True:
		stdscr.clear()
		h, w = stdscr.getmaxyx()
		lines = [
			"Current configuration:",
			f"  Generator: {config['generator']}",
			f"  Compiler:  {config['compiler']}",
			f"  Platform:  {config['platform']}",
			f"  Type:	  {config['build_type']}",
			f"  Toolchain: {config['toolchain']}",
		]
		if config.get('vcpkg_root'):
			lines.append(f"  vcpkg path: {config['vcpkg_root']}")

		lines.extend([
			"",
			"Start build?",
			"1) Yes, start build",
			"2) No, reselect"
		])
		start_y = max(0, h // 2 - len(lines) // 2)
		start_x = max(0, w // 2 - max(len(l) for l in lines) // 2)
		for i, line in enumerate(lines):
			try:
				stdscr.addstr(start_y + i, start_x, line)
			except curses.error:
				pass
		stdscr.refresh()

		key = stdscr.getch()
		if key == ord('1'):
			curses.endwin()
			success = run_cmake(config)
			if success:
				print("Build completed.")
			else:
				print("Build/Generation failed, please check errors.")
			return
		elif key == ord('2'):
			step_idx = 0
			# Reset entire config
			config = {k: None for k, _ in steps}
			break
			
# ---------- Entry point ----------
if __name__ == "__main__":
	# Check for last config and offer quick build
	last_config = load_last_config()
	if last_config:
		print("Found previous build configuration:")
		print(f"  Generator: {last_config['generator']}")
		print(f"  Compiler:  {last_config['compiler']}")
		print(f"  Platform:  {last_config['platform']}")
		print(f"  Type:	  {last_config['build_type']}")
		print(f"  Toolchain: {last_config['toolchain']}")
		if last_config.get('vcpkg_root'):
			print(f"  vcpkg path: {last_config['vcpkg_root']}")
		use_last = input("Use this configuration to build now? (y/n): ").strip().lower()
		if use_last == 'y':
			success = run_cmake(last_config)
			if success:
				print("Build completed.")
			else:
				print("Build/Generation failed.")
			sys.exit(0 if success else 1)

	# Interactive menu
	try:
		import curses
	except ImportError:
		print("Error: curses module not found.")
		print("On Windows, install windows-curses: pip install windows-curses")
		sys.exit(1)

	try:
		curses.wrapper(main)
	except KeyboardInterrupt:
		print("\nInterrupted by user.")
		sys.exit(0)
	except Exception as e:
		print(f"An error occurred: {e}")
		sys.exit(1)