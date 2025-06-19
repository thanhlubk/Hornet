project(FancyWidgets LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

qt_standard_project_setup()
qt_add_resources(APP_RESOURCE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)

# Automatically find all source/header/ui/qrc files
file(GLOB_RECURSE FANCYWIDGETS_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.ui
    # ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc
)

add_library(FancyWidgets SHARED ${FANCYWIDGETS_SOURCES})

target_sources(FancyWidgets PRIVATE ${APP_RESOURCE_RESOURCES})

target_include_directories(FancyWidgets
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(FancyWidgets PRIVATE FANCYWIDGETS_LIBRARY)

target_link_libraries(FancyWidgets 
    PUBLIC 
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui)

file(GLOB_RECURSE FANCY_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/FancyWidgets/*.h
)

set_target_properties(FancyWidgets PROPERTIES
    PUBLIC_HEADER "${FANCY_HEADERS}"
)