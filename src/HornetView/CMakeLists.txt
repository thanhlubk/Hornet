project(HornetView LANGUAGES CXX)

find_package(OpenGL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS OpenGL Widgets OpenGLWidgets)

# Automatically handle moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Glob all sources and headers
file(GLOB_RECURSE HORNETVIEW_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc
)

# Define shared library
add_library(HornetView SHARED
    ${HORNETVIEW_SOURCES}
)

# Target properties
target_include_directories(HornetView
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

message(STATUS "HORNETVIEW_SOURCES: ${HORNETVIEW_SOURCES}")

target_compile_definitions(HornetView PRIVATE HORNETVIEW_LIBRARY)

target_link_libraries(HornetView
    PUBLIC 
    Qt6::Widgets 
    Qt6::OpenGL 
    Qt6::OpenGLWidgets 
    OpenGL::GL 
    ${CMAKE_SOURCE_DIR}/lib/${PLATFORM_NAME}/glew32.lib
)

set_target_properties(HornetView PROPERTIES
    PUBLIC_HEADER "${HORNETVIEW_HEADERS}"
)