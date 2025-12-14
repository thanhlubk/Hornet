#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path
from typing import Any, Dict, Tuple, Optional, List

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


def read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: Any, force: bool) -> None:
    if path.exists() and not force:
        print(f"[skip] {path} exists (use --force to overwrite)")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
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


def copy_glew(repo_root: Path, os_name: str, glew_path_str: Optional[str]) -> None:
    if not glew_path_str:
        print("[glew] glewPath not set; skipping GLEW copy.")
        return

    glew_root = Path(glew_path_str)
    src_bin = glew_root / "bin"
    src_inc = glew_root / "include"
    src_lib = glew_root / "lib"

    # ✅ DLL: <glewPath>/bin/glew32.dll -> bin/<os>/Debug AND bin/<os>/Release
    src_dll = src_bin / "glew32.dll"
    dst_debug = repo_root / "bin" / os_name / "Debug" / "glew32.dll"
    dst_release = repo_root / "bin" / os_name / "Release" / "glew32.dll"
    copy_file_if_exists(src_dll, dst_debug)
    copy_file_if_exists(src_dll, dst_release)

    # Headers: <glewPath>/include/GL -> <repo_root>/include/GL
    copy_tree_merge(src_inc / "GL", repo_root / "include" / "GL")

    # Lib: <glewPath>/lib/glew32.lib -> <repo_root>/lib/<os>/glew32.lib
    dst_lib_os = repo_root / "lib" / os_name
    copy_file_if_exists(src_lib / "glew32.lib", dst_lib_os / "glew32.lib")

    print("[glew] done.")


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


def make_c_cpp_properties(os_name: str, cpp_standard: str, qt_path: str, compiler_path: str, app_name: str, glew_path: Optional[str]) -> Dict[str, Any]:
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


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate <repo_root>/.vscode files from setup_local_*.json")
    ap.add_argument("--os", choices=["windows", "linux"], required=True, help="Generate config for this OS")
    ap.add_argument("--config", required=True, help="Path to setup_local_*.json")
    ap.add_argument("--repo-root", required=True, help="Repo root (where .vscode will be created)")
    ap.add_argument("--app", default="HornetMain", help="Executable name (default: HornetMain)")
    ap.add_argument("--force", action="store_true", help="Overwrite existing .vscode/*.json")
    ap.add_argument("--no-copy-qt", action="store_true", help="Do not copy Qt runtime/plugins into repo folders")
    args = ap.parse_args()

    repo_root = Path(args.repo_root).resolve()
    cfg_path = Path(args.config).resolve()

    if not cfg_path.exists():
        print(f"[error] Config not found: {cfg_path}")
        return 2

    cfg = read_json(cfg_path)
    try:
        cpp_standard, qt_path, compiler_path, cmake_generator, glew_path = validate_cfg(cfg, cfg_path)
    except ValueError as e:
        print(f"[error] {e}")
        return 2

    ensure_required_folders(repo_root, args.os)

    # Copy Qt runtime/plugins unless disabled
    if not args.no_copy_qt:
        copy_qt_runtime(repo_root, args.os, qt_path)

    # Copy GLEW (if glewPath exists in config)
    copy_glew(repo_root, args.os, glew_path)

    vscode_dir = repo_root / ".vscode"
    ensure_dir(vscode_dir)

    write_json(vscode_dir / "launch.json", make_launch(args.os, args.app), force=args.force)
    write_json(
        vscode_dir / "c_cpp_properties.json",
        make_c_cpp_properties(args.os, cpp_standard, qt_path, compiler_path, args.app, glew_path),
        force=args.force,
    )
    write_json(vscode_dir / "settings.json", make_settings(args.os, qt_path, cmake_generator), force=args.force)

    print(f"[done] Generated: {vscode_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
