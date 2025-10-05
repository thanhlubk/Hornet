# Top-level CMakeLists.txt for Hornet with cross-platform layout support

cmake_minimum_required(VERSION 3.16)
project(Hornet VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Enforces that C++20 must be used
set(CMAKE_CXX_EXTENSIONS OFF)        # Optional: disables compiler-specific extensions like GNU++20

# Detect platform
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
elseif(UNIX)
    set(PLATFORM_NAME "linux")
endif()

# Set runtime and library output directories based on platform and build type
foreach(OUTPUTCONFIG IN ITEMS Debug Release)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)

    # Set build run time output (.exe and .dll) to build folder
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_SOURCE_DIR}/build/${PLATFORM_NAME}/${OUTPUTCONFIG})
    # Set build library output (.dll (import lib) dll that used seperately) to bin folder
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${OUTPUTCONFIG})
    # Set build static archive output (.lib) to lib folder
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_SOURCE_DIR}/lib/${PLATFORM_NAME}/${OUTPUTCONFIG})
endforeach()

# Add subdirectories
add_subdirectory(src/FancyWidgets)
add_subdirectory(src/HornetMain)
add_subdirectory(src/HornetView)
add_subdirectory(src/HornetBase)
add_subdirectory(src/HornetExecute)
add_subdirectory(src/HornetUtil)