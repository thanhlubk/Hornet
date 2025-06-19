# 🚀 Getting Started with Hornet in VS Code

This guide explains how to open, build, and run your cross-platform Qt + CMake project (`Hornet`) in **Visual Studio Code**.

---

## ✅ Prerequisites

- Visual Studio Code
- CMake Tools extension
- C++ extension for VS Code
- Qt installed (e.g., `C:/Qt/6.6.0/msvc2022_64`)
- Visual Studio (MSVC 2022) installed

---

## 📁 Project Structure

```
Hornet/
├── .vscode/               ← VS Code settings (already configured)
├── src/                   ← Source code for FancyWidgets + QtOpenGLWidgetApp
├── bin/Debug/             ← Compiled executables and DLLs
├── lib/Debug/             ← Qt DLLs to be copied automatically
├── temp/                  ← CMake build directory
├── CMakeLists.txt         ← Top-level project configuration
```

---

## 🧭 Steps to Build & Run

### 1. **Open VS Code**
Open the `Hornet/` folder as your workspace.

---

### 2. **Select a CMake Kit**
Click the CMake kit selector at the bottom bar and choose:
```
Visual Studio 17 2022 Release - amd64
```

---

### 3. **Configure the Project**
Let the CMake Tools extension run configure step. It will:
- Find Qt using the prefix path in `.vscode/settings.json`
- Create build files in `temp/`
- Detect MSVC as your compiler

---

### 4. **Build the Solution**
Click the build button in the bottom bar, or press:
```
Ctrl + Shift + B
```

CMake will build both:
- `FancyWidgets.dll` and `.lib`
- `QtOpenGLWidgetApp.exe`

Artifacts will go into:
```
bin/Debug/
```

Qt DLLs from `lib/Debug/` are automatically copied too.

---

### 5. **Run or Debug**
Press `F5` or go to the Run tab → "Launch QtOpenGLWidgetApp".

This will launch:
```
bin/Debug/QtOpenGLWidgetApp.exe
```

---

## 🔄 Switch to Release Mode

Click `Debug` in the bottom bar → choose `Release` → rebuild.

---

## 🧠 Notes

- If IntelliSense isn't working, press `Ctrl + Shift + P → CMake: Delete Cache and Reconfigure`
- You can build from terminal using `build_debug.bat` or `build_release.bat`

---

Happy coding! 🎉