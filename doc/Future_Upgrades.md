# 🚀 Future Upgrade Ideas for Hornet

This document outlines advanced features and professional upgrades that can be added to the Qt + CMake solution over time.

---

## 1. 🧠 Enable `.pdb` Generation for Release Builds

By default, `.pdb` files are not generated in Release mode. To enable them (for crash analysis or debug symbols in optimized builds), add this to your `CMakeLists.txt`:

```cmake
if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
endif()
```

> This tells MSVC to emit debug symbols (`/Zi`) and to keep them (`/DEBUG`).

---

## 2. 💥 Generate `.dmp` Crash Dump Files

Add this to your application code (Windows only):

```cpp
#include <Windows.h>
#include <DbgHelp.h>

void writeMiniDump(EXCEPTION_POINTERS* exceptionPointers) {
    HANDLE hFile = CreateFile(L"crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionPointers;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpNormal,
            &mdei,
            nullptr,
            nullptr
        );

        CloseHandle(hFile);
    }
}

LONG WINAPI exceptionFilter(EXCEPTION_POINTERS* exceptionPointers) {
    writeMiniDump(exceptionPointers);
    return EXCEPTION_EXECUTE_HANDLER;
}
```

And in `main()`:

```cpp
SetUnhandledExceptionFilter(exceptionFilter);
```

---

## 3. 🧹 Clean `temp/` Automatically After Build

Add this line at the end of your `.bat` file:

```bat
rmdir /s /q temp
```

Or use a simple PowerShell script:

```powershell
Start-Process cmake -ArgumentList "--build . --config Release" -Wait
Remove-Item -Recurse -Force temp
```

Or CMake post-build:

```cmake
add_custom_target(clean_temp ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
)
```

---

## 4. 🐧 Cross-Platform Linux/macOS Section

Create platform-specific sections using:

```cmake
if (WIN32)
    # Windows specific
elseif(APPLE)
    # macOS
elseif(UNIX)
    # Linux
endif()
```

Use platform-specific tools:

| Platform | Compiler | Notes |
|----------|----------|-------|
| Windows  | MSVC     | Matches Qt MSVC kits |
| Linux    | GCC/Clang| Qt open-source builds |
| macOS    | Clang    | Use Homebrew Qt or official installer |

You can run CMake with:

```bash
cmake .. -DCMAKE_PREFIX_PATH="/path/to/Qt6/lib/cmake"
cmake --build . --config Release
```

---

## 5. 📦 Create an Installer

Use **CMake + CPack** or **Inno Setup** on Windows:

### CMake + CPack example:

```cmake
include(InstallRequiredSystemLibraries)

set(CPACK_GENERATOR "ZIP;NSIS")
set(CPACK_PACKAGE_NAME "MyQtApp")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_EXECUTABLES "QtOpenGLWidgetApp" "My App")

include(CPack)
```

Then run:

```bash
cmake --install . --config Release
cpack -C Release
```

---

## 6. 📦 Pack it and deliver the source to another developer

---

## 7. 🧠 Create a send and recieve message handler when database changed

---

Feel free to turn any of these into scripts, automation, or CI pipelines!