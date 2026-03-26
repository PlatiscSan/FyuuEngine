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

def open_ide_project(build_dir, generator):
    """Open the generated IDE project file."""
    if generator.startswith("Visual Studio"):
        # Look for .sln file in build_dir
        sln_files = [f for f in os.listdir(build_dir) if f.endswith('.sln') or f.endswith('.slnx')]
        if sln_files:
            sln_path = os.path.join(build_dir, sln_files[0])
            print(f"Opening solution file: {sln_path}")
            if SYSTEM == 'Windows':
                os.startfile(sln_path)
            elif SYSTEM == 'Darwin':
                subprocess.run(['open', sln_path])
            else:  # Linux
                subprocess.run(['xdg-open', sln_path])
            return True
        else:
            print("No .sln file found in build directory.")
            return False
    elif generator == "Xcode":
        # Look for .xcodeproj
        proj_dirs = [d for d in os.listdir(build_dir) if d.endswith('.xcodeproj')]
        if proj_dirs:
            proj_path = os.path.join(build_dir, proj_dirs[0])
            print(f"Opening Xcode project: {proj_path}")
            if SYSTEM == 'Darwin':
                subprocess.run(['open', proj_path])
            else:
                print("Xcode projects can only be opened on macOS.")
            return True
        else:
            print("No .xcodeproj found.")
            return False
    else:
        print("Generator does not produce an IDE project file; cannot open.")
        return False

def run_cmake(config):
    """Run CMake configuration and optionally build."""
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

    # For MSVC compiler, open the generated project instead of building
    if compiler == 'MSVC':
        print("CMake configuration succeeded. Opening generated project...")
        open_ide_project(build_dir, generator)
        save_last_config(config)
        return True
    else:
        # Build for non-MSVC
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

        # --- Automatic skipping logic for MSVC ---
        # If generator is Visual Studio, compiler must be MSVC (skip compiler selection)
        if key == 'compiler' and config.get('generator', '').startswith('Visual Studio'):
            config['compiler'] = 'MSVC'
            step_idx += 1
            continue

        # If compiler is MSVC, skip platform and build type selection (use defaults)
        if key == 'platform' and config.get('compiler') == 'MSVC':
            config['platform'] = 'x64'  # Default platform
            step_idx += 1
            continue
        if key == 'build_type' and config.get('compiler') == 'MSVC':
            config['build_type'] = 'Release'  # Default build type
            step_idx += 1
            continue
        # --- end of automatic skipping ---

        title = f"Select {key.capitalize()} (arrow keys, Enter to confirm)"
        choice = menu(stdscr, title, options)

        if choice == 'Back':
            if step_idx > 0:
                # step back
                step_idx -= 1
                # Skip all steps that would be automatically skipped under the current configuration until a step requiring user interaction is encountered.
                while step_idx >= 0:
                    k, _ = steps[step_idx]
                    # Determine whether the current step will be automatically skipped under the current configuration.
                    should_skip = False
                    if k == 'compiler' and config.get('generator', '').startswith('Visual Studio'):
                        should_skip = True
                    elif k == 'platform' and config.get('compiler') == 'MSVC':
                        should_skip = True
                    elif k == 'build_type' and config.get('compiler') == 'MSVC':
                        should_skip = True
                    if should_skip:
                        step_idx -= 1
                    else:
                        break
                # Reset all values ​​starting from the new step_idx
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
                # If new generator is Visual Studio, compiler will be auto-set on next iteration
            elif key == 'compiler':
                # Compiler changed, platform and build_type need to be reselected
                config['platform'] = None
                config['build_type'] = None
                # No need to reset toolchain
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
            f"  Type:      {config['build_type']}",
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
                if config['compiler'] == 'MSVC':
                    print("Project generation succeeded and opened in IDE.")
                else:
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
        print(f"  Type:      {last_config['build_type']}")
        print(f"  Toolchain: {last_config['toolchain']}")
        if last_config.get('vcpkg_root'):
            print(f"  vcpkg path: {last_config['vcpkg_root']}")
        use_last = input("Use this configuration to build now? (y/n): ").strip().lower()
        if use_last == 'y':
            success = run_cmake(last_config)
            if success:
                if last_config['compiler'] == 'MSVC':
                    print("Project generation succeeded and opened in IDE.")
                else:
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