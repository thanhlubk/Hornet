#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path
from typing import Any, Dict, Tuple, Optional, List
import subprocess
import sys

# (Same associations you had in your settings templates; keep as-is)
FILES_ASSOCIATIONS: Dict[str, str] = {
    "qscrollbar": "cpp",
    "qpushbutton": "cpp",
    "qtoolbutton": "cpp",
    "qscrollarea": "cpp",
    "iterator": "cpp",
    "qwidget": "cpp",
    "qapplication": "cpp",
    "qstyle": "cpp",
    "chrono": "cpp",
    "future": "cpp",
    "system_error": "cpp",
    "xlocale": "cpp",
    "qvboxlayout": "cpp",
    "xiosbase": "cpp",
    "qpropertyanimation": "cpp",
    "format": "cpp",
    "array": "cpp",
    "bitset": "cpp",
    "initializer_list": "cpp",
    "list": "cpp",
    "span": "cpp",
    "type_traits": "cpp",
    "vector": "cpp",
    "xhash": "cpp",
    "xstring": "cpp",
    "xtree": "cpp",
    "xutility": "cpp",
    "qlabel": "cpp",
    "qpainterpath": "cpp",
    "qstackedwidget": "cpp",
    "qmap": "cpp",
    "functional": "cpp",
    "qthread": "cpp",
    "unordered_map": "cpp",
    "optional": "cpp",
    "forward_list": "cpp",
    "algorithm": "cpp",
    "xmemory": "cpp",
    "qevent": "cpp",
    "utility": "cpp",
    "xtr1common": "cpp",
    "qmouseevent": "cpp",
    "qlistwidget": "cpp",
    "qpainter": "cpp",
    "qstyleoption": "cpp",
    "tuple": "cpp",
    "qpointer": "cpp",
    "variant": "cpp",
    "qjsondocument": "cpp",
    "filesystem": "cpp",
    "qparallelanimationgroup": "cpp",
    "qbitmap": "cpp",
    "qwheelevent": "cpp",
    "cmath": "cpp",
    "qmargins": "cpp",
    "memory": "cpp",
    "qgraphicsdropshadoweffect": "cpp",
    "qjsonparseerror": "cpp",
    "qjsonarray": "cpp",
    "qjsonobject": "cpp",
    "atomic": "cpp",
    "bit": "cpp",
    "cctype": "cpp",
    "charconv": "cpp",
    "clocale": "cpp",
    "compare": "cpp",
    "concepts": "cpp",
    "condition_variable": "cpp",
    "cstdarg": "cpp",
    "cstddef": "cpp",
    "cstdint": "cpp",
    "cstdio": "cpp",
    "cstdlib": "cpp",
    "cstring": "cpp",
    "ctime": "cpp",
    "cwchar": "cpp",
    "exception": "cpp",
    "coroutine": "cpp",
    "resumable": "cpp",
    "iomanip": "cpp",
    "ios": "cpp",
    "iosfwd": "cpp",
    "istream": "cpp",
    "limits": "cpp",
    "locale": "cpp",
    "map": "cpp",
    "mutex": "cpp",
    "new": "cpp",
    "numeric": "cpp",
    "ostream": "cpp",
    "ratio": "cpp",
    "sstream": "cpp",
    "stdexcept": "cpp",
    "stop_token": "cpp",
    "streambuf": "cpp",
    "string": "cpp",
    "thread": "cpp",
    "typeinfo": "cpp",
    "xfacet": "cpp",
    "xlocbuf": "cpp",
    "xlocinfo": "cpp",
    "xlocmes": "cpp",
    "xlocmon": "cpp",
    "xlocnum": "cpp",
    "xloctime": "cpp",
    "xstddef": "cpp",
    "qdir": "cpp",
    "qfontdatabase": "cpp",
    "qsplitter": "cpp",
    "qhboxlayout": "cpp",
    "qtabwidget": "cpp",
    "qtimer": "cpp",
    "qshowevent": "cpp",
    "qframe": "cpp",
    "qlist": "cpp",
    "qmetaenum": "cpp",
    "qboxlayout": "cpp",
    "qmodelindex": "cpp",
    "source_location": "cpp",
    "qtwidgets": "cpp",
    "memory_resource": "cpp",
    "random": "cpp",
    "qabstractitemmodel": "cpp",
    "qtreeview": "cpp",
    "qicon": "cpp",
    "qtreewidget": "cpp",
    "qtextstream": "cpp",
    "qstringlist": "cpp",
    "unordered_set": "cpp",
    "qquaternion": "cpp",
    "qsurfaceformat": "cpp",
    "qopenglfunctions": "cpp",
    "qopenglshaderprogram": "cpp",
    "qfileinfo": "cpp",
    "qfile": "cpp",
    "qkeyevent": "cpp",
    "qmatrix4x4": "cpp",
    "qopenglextrafunctions": "cpp",
    "fstream": "cpp",
    "qdebug": "cpp",
    "qregularexpression": "cpp",
    "qvector3d": "cpp",
    "qelapsedtimer": "cpp",
    "qvector4d": "cpp",
    "qobject": "cpp",
    "typeindex": "cpp",
    "set": "cpp",
    "string_view": "cpp",
    "cinttypes": "cpp",
}

# ==========================
# Qt runtime copy lists
# ==========================
QT_DLLS_WINDOWS_DEBUG: List[str] = [
    "Qt6Cored.dll",
    "Qt6Guid.dll",
    "Qt6Widgetsd.dll",
    "Qt6OpenGLd.dll",
    "Qt6OpenGLWidgetsd.dll",
    "Qt6Svgd.dll",
]

QT_DLLS_WINDOWS_RELEASE: List[str] = [
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6OpenGL.dll",
    "Qt6OpenGLWidgets.dll",
    "Qt6Svg.dll",
]

# Linux: Qt libs are typically in <qtPath>/lib (adjust names as needed)
QT_LIBS_LINUX_COMMON: List[str] = [
    "libQt6Core.so.6",
    "libQt6Gui.so.6",
    "libQt6Widgets.so.6",
    "libQt6OpenGL.so.6",
    "libQt6OpenGLWidgets.so.6",
    "libQt6Svg.so.6",
]

QT_PLUGINS_TO_COPY: List[str] = [
    "platforms",
    "imageformats",
]

# Defaults (not stored in env_configure.json)
DEFAULT_APP_NAME: str = "HornetMain"
DEFAULT_CPP_STANDARD: str = "c++20"

def read_json(path: Path) -> Dict[str, Any]:
    """Read env_configure.json.

    Note: The file must be valid JSON (strings must be quoted; no trailing commas).
    """
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON in {path}: {e}") from e

def write_json(path: Path, data: Any, force: bool) -> None:
    if path.exists() and not force:
        print(f"[skip] {path} exists (use --force to overwrite)")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"[write] {path}")

def write_text_windows(path: Path, text: str, force: bool) -> None:
    """Write text file with CRLF endings (for .bat)."""
    if path.exists() and not force:
        print(f"[skip] {path} exists (use --force to overwrite)")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.replace("\r\n", "\n").replace("\n", "\r\n"), encoding="utf-8")
    print(f"[write] {path}")

def write_text_unix(path: Path, text: str, force: bool, make_executable: bool = False) -> None:
    """Write text file with LF endings (for .sh)."""
    if path.exists() and not force:
        print(f"[skip] {path} exists (use --force to overwrite)")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.replace("\r\n", "\n"), encoding="utf-8")
    if make_executable:
        try:
            path.chmod(path.stat().st_mode | 0o111)
        except Exception:
            pass
    print(f"[write] {path}")

def norm_slash(p: str) -> str:
    return p.replace("\\", "/")

def ensure_dir(p: Path) -> None:
    p.mkdir(parents=True, exist_ok=True)

def validate_cfg(cfg: Dict[str, Any], cfg_path: Path) -> Tuple[str, str, str, str, Optional[str]]:
    required = ["qtPath", "compilerPath", "cmakeGenerator"]
    missing = [k for k in required if k not in cfg or not str(cfg.get(k, "")).strip()]
    if missing:
        raise ValueError(f"Missing keys in {cfg_path}: {', '.join(missing)}")

    # cpp_standard = str(cfg["cppStandard"])
    cpp_standard = "c++20" # default to C++20 for now
    qt_path = norm_slash(str(cfg["qtPath"]))
    compiler_path = norm_slash(str(cfg["compilerPath"]))
    cmake_generator = str(cfg["cmakeGenerator"])

    glew_path = cfg.get("glewPath")
    glew_path = norm_slash(str(glew_path)) if glew_path and str(glew_path).strip() else None

    return cpp_standard, qt_path, compiler_path, cmake_generator, glew_path

def ensure_required_folders(repo_root: Path, os_name: str) -> None:
    rels = [
        f"bin/{os_name}/Debug",
        f"bin/{os_name}/Release",
        f"build/{os_name}/Debug",
        f"build/{os_name}/Release",
        f"lib/{os_name}/Debug",
        f"lib/{os_name}/Release",
        f"plugins/{os_name}",
        f"temp/{os_name}",
        "include",
    ]
    created = 0
    for r in rels:
        p = repo_root / r
        if not p.exists():
            p.mkdir(parents=True, exist_ok=True)
            created += 1
            print(f"[mkdir] {p}")

    if created:
        print(f"[done] Created {created} folder(s).")
    else:
        print("[done] All required folders already exist.")

def ensure_standard_folders(repo_root: Path, os_name: str) -> None:
    rels = [
        f"bin/{os_name}/Debug",
        f"bin/{os_name}/Release",
        f"build/{os_name}/Debug",
        f"build/{os_name}/Release",
        f"lib/{os_name}/Debug",
        f"lib/{os_name}/Release",
        f"plugins/{os_name}",
        f"temp/{os_name}",
        "include",
    ]
    for r in rels:
        ensure_dir(repo_root / r)

def copy_file_if_exists(src: Path, dst: Path) -> None:
    if not src.exists():
        print(f"[warn] missing file: {src}")
        return
    if not src.is_file():
        print(f"[warn] not a file: {src}")
        return
    ensure_dir(dst.parent)
    shutil.copy2(src, dst)  # overwrites if exists
    print(f"[copy] {src} -> {dst}")

def copy_tree_merge(src_dir: Path, dst_dir: Path) -> None:
    if not src_dir.exists():
        print(f"[warn] missing folder: {src_dir}")
        return
    if not src_dir.is_dir():
        print(f"[warn] not a folder: {src_dir}")
        return
    ensure_dir(dst_dir)
    shutil.copytree(src_dir, dst_dir, dirs_exist_ok=True)
    print(f"[copytree] {src_dir} -> {dst_dir}")

def copy_file_list(src_dir: Path, file_list: List[str], dst_dir: Path) -> None:
    ensure_dir(dst_dir)
    copied = 0
    missing = 0

    for name in file_list:
        src = src_dir / name
        if not src.exists():
            print(f"[warn] missing file: {src}")
            missing += 1
            continue
        if not src.is_file():
            print(f"[warn] not a file: {src}")
            missing += 1
            continue

        shutil.copy2(src, dst_dir / src.name)
        copied += 1
        print(f"[copy] {src} -> {dst_dir / src.name}")

    print(f"[qt] copied={copied}, missing={missing}, dst={dst_dir}")

def copy_plugin_items(qt_plugins_dir: Path, plugin_names: List[str], dst_root: Path) -> None:
    ensure_dir(dst_root)
    for name in plugin_names:
        src = qt_plugins_dir / name
        dst = dst_root / name

        if not src.exists():
            print(f"[warn] missing plugin item: {src}")
            continue

        if src.is_dir():
            shutil.copytree(src, dst, dirs_exist_ok=True)
            print(f"[copytree] {src} -> {dst}")
        else:
            ensure_dir(dst.parent)
            shutil.copy2(src, dst)
            print(f"[copy] {src} -> {dst}")

def copy_qt_runtime(repo_root: Path, os_name: str, qt_root_str: str) -> None:
    qt_root = Path(qt_root_str)
    qt_bin = qt_root / "bin"
    qt_lib = qt_root / "lib"

    # pick runtime source dir
    runtime_src = qt_bin if qt_bin.exists() else qt_lib
    if not runtime_src.exists():
        print(f"[warn] Qt runtime folder not found: {runtime_src}")
        return

    dst_debug = repo_root / "bin" / os_name / "Debug"
    dst_release = repo_root / "bin" / os_name / "Release"

    if os_name == "windows":
        copy_file_list(runtime_src, QT_DLLS_WINDOWS_DEBUG, dst_debug)
        copy_file_list(runtime_src, QT_DLLS_WINDOWS_RELEASE, dst_release)
    else:
        # Linux: copy same list into both by default
        copy_file_list(runtime_src, QT_LIBS_LINUX_COMMON, dst_debug)
        copy_file_list(runtime_src, QT_LIBS_LINUX_COMMON, dst_release)

    qt_plugins = qt_root / "plugins"
    if not qt_plugins.exists():
        print(f"[warn] Qt plugins folder not found: {qt_plugins}")
        return

    dst_plugins = repo_root / "plugins" / os_name
    copy_plugin_items(qt_plugins, QT_PLUGINS_TO_COPY, dst_plugins)

# ----------------------------
# Copy: Qt & GLEW
# ----------------------------
def copy_qt_windows(repo_root: Path, os_name: str, qt_path: str, cfg: Dict[str, Any]) -> None:
    qt_root = Path(qt_path)
    qt_bin = qt_root / "bin"
    qt_plugins = qt_root / "plugins"

    # allow overriding lists from JSON
    debug_list = cfg.get("qtDebugDlls", QT_DLLS_WINDOWS_DEBUG)
    release_list = cfg.get("qtReleaseDlls", QT_DLLS_WINDOWS_RELEASE)
    plugins_list = cfg.get("qtPlugins", QT_PLUGINS_TO_COPY)

    if not qt_bin.exists():
        print(f"[warn] Qt bin not found: {qt_bin}")
        return

    dst_debug = repo_root / "bin" / os_name / "Debug"
    dst_release = repo_root / "bin" / os_name / "Release"
    copy_file_list(qt_bin, debug_list, dst_debug)
    copy_file_list(qt_bin, release_list, dst_release)

    if qt_plugins.exists():
        dst_plugins = repo_root / "plugins" / os_name
        copy_plugin_items(qt_plugins, plugins_list, dst_plugins)
    else:
        print(f"[warn] Qt plugins folder not found: {qt_plugins}")

def copy_qt_linux(repo_root: Path, os_name: str, qt_path: str) -> None:
    """Copy a minimal Qt runtime set for Linux into bin/<os>/{Debug,Release} and plugins/<os>."""
    qt_root = Path(qt_path)
    qt_lib = qt_root / "lib"
    qt_plugins = qt_root / "plugins"

    if not qt_lib.exists():
        print(f"[warn] Qt lib not found: {qt_lib}")
        return

    dst_debug = repo_root / "bin" / os_name / "Debug"
    dst_release = repo_root / "bin" / os_name / "Release"
    copy_file_list(qt_lib, QT_LIBS_LINUX_COMMON, dst_debug)
    copy_file_list(qt_lib, QT_LIBS_LINUX_COMMON, dst_release)

    if qt_plugins.exists():
        dst_plugins = repo_root / "plugins" / os_name
        copy_plugin_items(qt_plugins, QT_PLUGINS_TO_COPY, dst_plugins)
    else:
        print(f"[warn] Qt plugins folder not found: {qt_plugins}")

def copy_glew(repo_root: Path, os_name: str, glew_path: Optional[str]) -> None:
    if not glew_path:
        print("[glew] glewPath not set; skipping.")
        return

    glew_root = Path(glew_path)
    src_bin = glew_root / "bin"
    src_inc = glew_root / "include"
    src_lib = glew_root / "lib"

    # DLL -> Debug and Release
    src_dll = src_bin / "glew32.dll"
    copy_file_if_exists(src_dll, repo_root / "bin" / os_name / "Debug" / "glew32.dll")
    copy_file_if_exists(src_dll, repo_root / "bin" / os_name / "Release" / "glew32.dll")

    # Headers: include/GL -> repo include/GL
    copy_tree_merge(src_inc / "GL", repo_root / "include" / "GL")

    # Import lib: lib/glew32.lib -> repo lib/<os>/glew32.lib
    copy_file_if_exists(src_lib / "glew32.lib", repo_root / "lib" / os_name / "glew32.lib")

def copy_glm(repo_root: Path, glm_path: Optional[str]) -> None:
    if not glm_path:
        print("[glm] glmPath not set; skipping.")
        return

    glm_root = Path(glm_path)
    src_inc = glm_root / "include" / "glm"
    copy_tree_merge(src_inc, repo_root / "include" / "glm")

def copy_eigen3(repo_root: Path, eigen3_path: Optional[str]) -> None:
    if not eigen3_path:
        print("[eigen3] eigen3Path not set; skipping.")
        return

    eigen3_root = Path(eigen3_path)
    src_inc = eigen3_root / "include" / "eigen3"
    copy_tree_merge(src_inc, repo_root / "include" / "eigen3")

def write_build_bats_windows(repo_root: Path, *, qt_path: str, force: bool) -> None:
    """Generate tools/build_debug.bat and tools/build_release.bat for Windows (single-config generator)."""
    tools_dir = repo_root / "tools"
    ensure_dir(tools_dir)

    qt_prefix = norm_slash(qt_path)

    debug = f"""@echo off
cd /d %~dp0..
rmdir /s /q temp
mkdir temp
cd temp

cmake .. ^
 -DCMAKE_PREFIX_PATH="{qt_prefix}/lib/cmake"^
 -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug
"""
    release = f"""@echo off
cd /d %~dp0..
rmdir /s /q temp
mkdir temp
cd temp

cmake .. ^
 -DCMAKE_PREFIX_PATH="{qt_prefix}/lib/cmake" ^
 -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release
"""

    write_text_windows(tools_dir / "build_debug.bat", debug, force=force)
    write_text_windows(tools_dir / "build_release.bat", release, force=force)

def write_build_sh_linux(repo_root: Path, *, qt_path: str, force: bool) -> None:
    """Generate tools/build_debug.sh and tools/build_release.sh for Linux (single-config generator)."""
    tools_dir = repo_root / "tools"
    ensure_dir(tools_dir)

    qt_prefix = norm_slash(qt_path)

    debug = f"""#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")/.."

rm -rf temp
mkdir -p temp
cd temp

cmake .. \
  -DCMAKE_PREFIX_PATH="{qt_prefix}/lib/cmake" \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug
"""

    release = f"""#!/usr/bin/env sh
set -eu

cd "$(dirname "$0")/.."

rm -rf temp
mkdir -p temp
cd temp

cmake .. \
  -DCMAKE_PREFIX_PATH="{qt_prefix}/lib/cmake" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release
"""

    write_text_unix(tools_dir / "build_debug.sh", debug, force=force, make_executable=True)
    write_text_unix(tools_dir / "build_release.sh", release, force=force, make_executable=True)

def make_launch(os_name: str, app_name: str) -> Dict[str, Any]:
    if os_name == "windows":
        return {
            "version": "0.2.0",
            "configurations": [
                {
                    "name": f"Launch {app_name} Debug (Windows)",
                    "type": "cppvsdbg",
                    "request": "launch",
                    "program": f"${{workspaceFolder}}/build/windows/Debug/{app_name}.exe",
                    "cwd": "${workspaceFolder}/build/windows/Debug",
                    "stopAtEntry": False,
                    "console": "externalTerminal",
                },
                {
                    "name": f"Launch {app_name} Release (Windows)",
                    "type": "cppvsdbg",
                    "request": "launch",
                    "program": f"${{workspaceFolder}}/bin/windows/Release/{app_name}.exe",
                    "cwd": "${workspaceFolder}/bin/windows/Release",
                    "stopAtEntry": False,
                    "console": "externalTerminal",
                },
            ],
        }

    return {
        "version": "0.2.0",
        "configurations": [
            {
                "name": f"Launch {app_name} Debug (Linux)",
                "type": "cppdbg",
                "request": "launch",
                "program": f"${{workspaceFolder}}/build/linux/Debug/{app_name}",
                "cwd": "${workspaceFolder}/build/linux/Debug",
                "MIMode": "gdb",
                "stopAtEntry": False,
            },
            {
                "name": f"Launch {app_name} Release (Linux)",
                "type": "cppdbg",
                "request": "launch",
                "program": f"${{workspaceFolder}}/build/linux/Release/{app_name}",
                "cwd": "${workspaceFolder}/build/linux/Release",
                "MIMode": "gdb",
                "stopAtEntry": False,
            },
        ],
    }

def make_c_cpp_properties(os_name: str, cpp_standard: str, qt_path: str, compiler_path: str, app_name: str, glew_path: Optional[str], glm_path: Optional[str], eigen3_path: Optional[str]) -> Dict[str, Any]:
    cfg_name = "Windows-MSVC" if os_name == "windows" else "Linux-GCC"
    temp_os = "windows" if os_name == "windows" else "linux"

    include_paths = [
        "${workspaceFolder}/src/**",
        "${workspaceFolder}/src/FancyWidgets/include",
        "${workspaceFolder}/src/HornetView/include",
        "${workspaceFolder}/src/HornetUtil/include",
        "${workspaceFolder}/src/HornetBase/include",
        "${workspaceFolder}/src/HornetExecute/include",
        "${workspaceFolder}/include",  # contains GLEW headers: include/GL
        f"${{workspaceFolder}}/temp/{temp_os}/src/{app_name}/{app_name}_autogen/include",
        f"{qt_path}/include/**",
    ]

    # Add GLEW include root if provided in setup_local_*.json
    if glew_path:
        include_paths.append(f"{glew_path}/include/**")
    if glm_path:
        include_paths.append(f"{glm_path}/include/**")
    if eigen3_path:
        include_paths.append(f"{eigen3_path}/include/**")

    return {
        "configurations": [
            {
                "name": cfg_name,
                "includePath": include_paths,
                "compilerPath": compiler_path,
                "cppStandard": cpp_standard,
            }
        ],
        "version": 4,
    }

def make_settings(os_name: str, qt_path: str, cmake_generator: str) -> Dict[str, Any]:
    temp_os = "windows" if os_name == "windows" else "linux"
    return {
        "cmake.sourceDirectory": "${workspaceFolder}",
        "cmake.generator": cmake_generator,
        "cmake.buildDirectory": f"${{workspaceFolder}}/temp/{temp_os}",
        "cmake.configureSettings": {
            "CMAKE_PREFIX_PATH": f"{qt_path}/lib/cmake",
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        },
        "files.associations": FILES_ASSOCIATIONS,
    }

# ----------------------------
# Modes
# ----------------------------
def run_mode_vscode(repo_root: Path, env: Dict[str, Any], *, os_name: str, force: bool, no_copy_qt: bool) -> int:
    cfg = env.get("vsCode")
    if not isinstance(cfg, dict):
        print("[error] env_configure.json missing object: vsCode")
        return 2
    os_name = os_name.lower()
    app_name = DEFAULT_APP_NAME
    cpp_standard = DEFAULT_CPP_STANDARD
    qt_path = cfg.get("qtPath", "")
    compiler_path = cfg.get("compilerPath", "")
    cmake_generator = cfg.get("cmakeGenerator", "Ninja")
    glew_path = cfg.get("glewPath")
    glm_path = cfg.get("glmPath")
    eigen3_path = cfg.get("eigen3Path")

    if not qt_path or not compiler_path:
        print("[error] vsCode.qtPath and vsCode.compilerPath are required")
        return 2

    ensure_standard_folders(repo_root, os_name)

    # copy runtime
    if not no_copy_qt:
        if os_name == "windows":
            copy_qt_windows(repo_root, os_name, qt_path, cfg)
        else:
            copy_qt_linux(repo_root, os_name, qt_path)
    copy_glew(repo_root, os_name, glew_path)
    copy_glm(repo_root, glm_path)
    copy_eigen3(repo_root, eigen3_path)

    # generate build scripts
    if os_name == "windows":
        write_build_bats_windows(repo_root, qt_path=qt_path, force=force)
    else:
        write_build_sh_linux(repo_root, qt_path=qt_path, force=force)

    # generate .vscode
    vscode_dir = repo_root / ".vscode"
    ensure_dir(vscode_dir)

    write_json(vscode_dir / "launch.json", make_launch(os_name, app_name), force=force)
    write_json(
        vscode_dir / "c_cpp_properties.json",
        make_c_cpp_properties(os_name, cpp_standard, qt_path, compiler_path, app_name, glew_path, glm_path, eigen3_path),
        force=force,
    )
    write_json(vscode_dir / "settings.json", make_settings(os_name, qt_path, cmake_generator), force=force)

    print("[done] vscode environment initialized")
    return 0

def run_mode_vs(repo_root: Path, env: Dict[str, Any], no_copy_qt: bool) -> int:
    cfg = env.get("vs")
    if not isinstance(cfg, dict):
        print("[error] env_configure.json missing object: vs")
        return 2

    # VS mode is Windows-only by design here
    os_name = "windows"
    qt_path = cfg.get("qtPath", "")
    glew_path = cfg.get("glewPath")
    glm_path = cfg.get("glmPath")
    eigen3_path = cfg.get("eigen3Path")
    vs_version = cfg.get("vsVersion", "")

    if not qt_path or not vs_version:
        print("[error] vs.qtPath and vs.vsVersion are required")
        return 2

    ensure_standard_folders(repo_root, os_name)

    if not no_copy_qt:
        copy_qt_windows(repo_root, os_name, qt_path, cfg)
    copy_glew(repo_root, os_name, glew_path)
    copy_glm(repo_root, glm_path)
    copy_eigen3(repo_root, eigen3_path)

    # generate build scripts
    if os_name == "windows":
        write_build_bats_windows(repo_root, qt_path=qt_path, force=True)
    else:
        write_build_sh_linux(repo_root, qt_path=qt_path, force=True)

    # Run cmake configure for Visual Studio generator
    cmd = [
        "cmake",
        "-S", str(repo_root),
        "-B", str(repo_root / "build/vs"),
        "-G", str(vs_version),
        "-A", "x64",
        f"-DCMAKE_PREFIX_PATH={norm_slash(qt_path)}",
    ]

    print("[cmd] " + " ".join(cmd))
    try:
        subprocess.run(cmd, check=True)
    except FileNotFoundError:
        print("[error] cmake not found in PATH")
        return 2
    except subprocess.CalledProcessError as e:
        print(f"[error] cmake failed: {e}")
        return 2

    print("[done] Visual Studio environment initialized (no .vscode created)")
    return 0

def main() -> int:
    ap = argparse.ArgumentParser(description="Initialize repo environment from env_configure.json")
    ap.add_argument("--mode", choices=["vsCode", "vscode", "vs"], default="vsCode", help="Mode: vsCode (default) or vs")
    ap.add_argument("--os", choices=["windows", "linux"], help="Target OS for vsCode mode (windows|linux).")
    ap.add_argument("--repo-root", required=True, help="Repo root path")
    ap.add_argument("--config", required=True, help="Path to env_configure.json")
    ap.add_argument("--force", action="store_true", help="Overwrite existing .vscode/*.json (vscode mode only)")
    ap.add_argument("--no-copy-qt", action="store_true", help="Do not copy Qt runtime/plugins into repo")
    args = ap.parse_args()

    repo_root = Path(args.repo_root).resolve()
    cfg_path = Path(args.config).resolve()

    if not cfg_path.exists():
        print(f"[error] config not found: {cfg_path}")
        return 2

    try:
        env = read_json(cfg_path)
    except ValueError as e:
        print(f"[error] {e}")
        return 2

    mode = args.mode
    if mode == "vscode":
        mode = "vsCode"  # alias

    if mode == "vsCode":
        cfg = env.get("vsCode")
        json_os = cfg.get("os") if isinstance(cfg, dict) else None
        os_name = args.os or json_os or "windows"
        return run_mode_vscode(
            repo_root,
            env,
            os_name=os_name,
            force=args.force,
            no_copy_qt=args.no_copy_qt,
        )
    else:
        # vs mode never writes .vscode anyway
        return run_mode_vs(repo_root, env, no_copy_qt=args.no_copy_qt)
    
if __name__ == "__main__":
    raise SystemExit(main())
