# 🛠️ Qt OpenGL Modular CMake Project Setup Guide

This guide walks you through setting up a **modular cross-platform Qt6 + OpenGL solution** using CMake, with a professional project structure that supports reusable libraries, external dependencies (like GLEW and GLM), and both Windows and Linux development.

---

## 📁 Final Project Structure

```
AnotherSolution/
├── bin/                    # Runtime output
│   └── [windows|linux]/[Debug|Release]/
├── lib/                    # External binary libs
│   ├── glew/               # GLEW .dll and .lib
│   └── qt/                 # Qt runtime .dll/.so
├── include/               # Header-only dependencies (e.g. GLM)
│   └── glm/
├── src/
│   ├── FancyWidgets/       # Custom widget library (e.g. FancyButton)
│   ├── SolutionView/       # OpenGL library (e.g. GLViewport)
│   └── QtOpenGLWidgetApp/  # Main application
├── temp/                  # Build directory
│   └── [windows|linux]/[Debug|Release]/
├── tools/                 # Build and run helper scripts
├── .vscode/               # VS Code config files
└── CMakeLists.txt         # Top-level CMake
```

---

## ✅ Step-by-Step Setup

### 1. 🧱 Create the Project Structure

```bash
mkdir -p AnotherSolution/src/FancyWidgets/include/FancyWidgets
mkdir -p AnotherSolution/src/SolutionView/include/SolutionView
mkdir -p AnotherSolution/src/SolutionView/shaders
mkdir -p AnotherSolution/src/QtOpenGLWidgetApp
mkdir -p AnotherSolution/include/glm
mkdir -p AnotherSolution/lib/glew/{Debug,Release}
mkdir -p AnotherSolution/lib/qt/{Debug,Release}
mkdir -p AnotherSolution/bin/windows/{Debug,Release}
mkdir -p AnotherSolution/bin/linux/{Debug,Release}
mkdir -p AnotherSolution/temp
mkdir -p AnotherSolution/tools
```

---

### 2. 🧠 Implement Libraries

- `FancyWidgets` exports reusable Qt widgets (like `FancyButton`)
- `SolutionView` exports OpenGL logic with `QOpenGLWidget` and shaders

---

### 3. ⚙️ Create General CMakeLists.txt

Each module has a CMake file using:
- `file(GLOB_RECURSE ...)`
- `AUTOMOC`, `AUTOUIC`
- `target_include_directories(...)`
- `target_compile_definitions(...)` for DLL exports

---

### 4. 🎯 Link External Libraries

#### GLM
- Header-only → copy to `include/glm`
- No compile/link step needed

#### GLEW
- Put `glew32.lib` + `glew32.dll` in `lib/glew/{Debug,Release}`
- Headers in `include/GL/`
- Use `target_include_directories()` and `target_link_libraries()` in CMake

---

### 5. 🧩 Connect Everything in CMake

At the root `src/CMakeLists.txt`:

```cmake
add_subdirectory(FancyWidgets)
add_subdirectory(SolutionView)
add_subdirectory(QtOpenGLWidgetApp)
```

In `QtOpenGLWidgetApp/CMakeLists.txt`:

```cmake
target_link_libraries(QtOpenGLWidgetApp
    PRIVATE FancyWidgets SolutionView Qt6::Widgets
)
```

---

### 6. 🧰 VS Code Integration

Create `.vscode/settings.json`:

```json
{
  "cmake.generator": "Ninja",
  "cmake.buildDirectory": "${workspaceFolder}/temp",
  "cmake.configureSettings": {
    "CMAKE_PREFIX_PATH": "${env:QT_PREFIX_PATH}",
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
  }
}
```

Create `.vscode/c_cpp_properties.json` with Qt + GLEW + GLM include paths.

---

### 7. 🚀 Build and Run

**Set environment before launching VS Code:**

```bash
# On Linux
export QT_PREFIX_PATH=/path/to/Qt/lib/cmake
export CMAKE_BUILD_DIR=./temp/linux
code .

# On Windows (run in .bat file)
set QT_PREFIX_PATH=C:\Qt.x.x\msvc2022_64\lib\cmake
set CMAKE_BUILD_DIR=.	emp\windows
code .
```

Then:
- `Ctrl+Shift+P → CMake: Configure`
- `Ctrl+Shift+P → CMake: Build`

---

### 8. 🧪 Post-Build: Copy DLLs

In CMake:
```cmake
add_custom_command(TARGET QtOpenGLWidgetApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/lib/glew/$<CONFIG>/glew32.dll
        $<TARGET_FILE_DIR:QtOpenGLWidgetApp>
)
```

---

## ✅ Done!

You now have a robust, maintainable, cross-platform Qt6 + OpenGL application architecture.