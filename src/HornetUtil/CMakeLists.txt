project(HornetUtil VERSION 1.0 LANGUAGES CXX)

file(GLOB_RECURSE HORNETUTIL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
)

file(GLOB_RECURSE HORNETUTIL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HornetUtil/*.h
)

add_library(HornetUtil SHARED 
    ${HORNETUTIL_SOURCES}
    ${HORNETUTIL_HEADERS}
)

target_include_directories(HornetUtil
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(HornetUtil PRIVATE HORNETUTIL_LIBRARY)

set_target_properties(HornetUtil PROPERTIES
    PUBLIC_HEADER "${HORNETUTIL_HEADERS}"
)