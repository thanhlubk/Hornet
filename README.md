# Developer Setup & Build

This repo supports two main workflows:

- **VS Code workflow** (Windows/Linux): generates `.vscode/` configs and prepares local folders + runtime deps.
- **Visual Studio workflow** (Windows): prepares local folders + runtime deps and configures a VS CMake build directory.

The setup is driven by **`tools/env_configure.json`**. Your current file is a *template* and must be filled with real paths (and valid JSON).

---

## Prerequisites

### Common
- **Git**
- **CMake** (available in `PATH`)
- **Python 3** (available in `PATH`)
- **Qt 6** installed (you must provide `qtPath`)
- **GLEW** (prebuilt or installed; you must provide `glewPath`)

### Windows
- **MSVC** toolchain (Visual Studio installed)
- If using VS Code: install VS Code + C/C++ extension + CMake Tools extension

### Linux
- GCC/Clang toolchain
- VS Code + C/C++ extension + CMake Tools extension (recommended)

---

## 1) Configure `tools/env_configure.json`

### IMPORTANT: make it valid JSON
Your template currently uses placeholders like `<qtPath>` and has a trailing comma; JSON requires **quoted strings** and **no trailing commas**. fileciteturn15file0

Use this **valid JSON template** (edit values for your machine):

```json
{
  "vsCode": {
    "os": "windows",
    "qtPath": "C:/Qt/6.8.3/msvc2022_64",
    "compilerPath": "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/<ver>/bin/Hostx64/x64/cl.exe",
    "glewPath": "D:/3rdparty/glew-2.2.0",
    "cmakeGenerator": "Ninja",
    "appName": "HornetMain",
    "cppStandard": "c++20"
  },
  "vs": {
    "qtPath": "C:/Qt/6.8.3/msvc2022_64",
    "glewPath": "D:/3rdparty/glew-2.2.0",
    "vsVersion": "Visual Studio 17 2022"
  }
}
```

### Linux example
```json
{
  "vsCode": {
    "os": "linux",
    "qtPath": "/opt/Qt/6.8.3/gcc_64",
    "compilerPath": "/usr/bin/g++",
    "glewPath": "/opt/glew",
    "cmakeGenerator": "Ninja",
  }
}
```

Notes:
- `qtPath` should point to the Qt **kit root** (the folder that contains `bin/`, `lib/`, `plugins/`, `include/`).
- `glewPath` should contain `bin/`, `include/`, and `lib/`.

---

## 2) Run bootstrap

All bootstrap scripts are in the `tools/` folder.

### Windows (VS Code)
```bat
tools\bootstrap_vscode.bat
```

Optional flags:
```bat
tools\bootstrap_vscode.bat --force
tools\bootstrap_vscode.bat --no-copy-qt
```

What it does:
- Creates standard repo folders (`bin/`, `build/`, `lib/`, `plugins/`, `temp/`, `include/`)
- Copies Qt runtime + plugins into repo folders (unless `--no-copy-qt`)
- Copies GLEW files into repo folders
- Generates `.vscode/launch.json`, `.vscode/c_cpp_properties.json`, `.vscode/settings.json`
- Generates build scripts (see below)

### Windows (Visual Studio)
```bat
tools\bootstrap_vs.bat
```

Optional flags:
```bat
tools\bootstrap_vs.bat --force
tools\bootstrap_vs.bat --no-copy-qt
```

What it does:
- Creates standard repo folders and copies Qt/GLEW
- Runs CMake configure for Visual Studio generator into `build-vs/`
- Generates build scripts

Open the generated solution from `build-vs/` (or use the build scripts).

### Linux (VS Code)
```sh
chmod +x tools/bootstrap_vscode.sh
tools/bootstrap_vscode.sh
```

Optional flags:
```sh
tools/bootstrap_vscode.sh --force
tools/bootstrap_vscode.sh --no-copy-qt
```

---

## 3) Open the repo in your IDE

### VS Code
Open the **repo root** folder in VS Code.  
CMake Tools should detect the config and you can build/debug from the UI.

### Visual Studio (Windows)
Open the generated solution under `build-vs/` or use “Open Folder” if that’s your preference.

---

## 4) Build (Debug / Release)

Bootstrap generates build scripts in `tools/`:

### Windows
- `tools\build_debug.bat`
- `tools\build_release.bat`

### Linux
- `tools/build_debug.sh`
- `tools/build_release.sh`

Run them from the repo:

**Windows**
```bat
tools\build_debug.bat
tools\build_release.bat
```

**Linux**
```sh
chmod +x tools/build_debug.sh tools/build_release.sh
tools/build_debug.sh
tools/build_release.sh
```

### Build output location
Build directories are placed under:

- `build/<os>/Debug`
- `build/<os>/Release`

(where `<os>` is `windows` or `linux`)

If you customize build directories in the generated scripts, the output will follow your script settings.

---

## Troubleshooting

- **CMake not found**: install CMake and ensure `cmake` is in `PATH`.
- **Python not found**: install Python 3 and ensure `python` / `python3` is in `PATH`.
- **Qt copy warnings**: if your Qt build doesn’t include a module listed in the script, you will see `[warn] missing file`. Adjust the built-in lists in `tools/init_env.py`.
- **Linux Qt runtime**: depending on distro/Qt packaging, you may need additional `.so` dependencies on your system. The script copies the main Qt `.so` files and plugins, but system dependencies are still required.
