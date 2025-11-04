project(HornetBase VERSION 1.0 LANGUAGES CXX)

file(GLOB_RECURSE HORNETBASE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
)

file(GLOB_RECURSE HORNETBASE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HornetBase/*.h
)

add_library(HornetBase SHARED 
    ${HORNETBASE_SOURCES}
    ${HORNETBASE_HEADERS}
)

target_include_directories(HornetBase
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(HornetBase PRIVATE HORNETBASE_LIBRARY)

target_link_libraries(HornetBase
    PUBLIC 
    HornetUtil
)

set_target_properties(HornetBase PROPERTIES
    PUBLIC_HEADER "${HORNETBASE_HEADERS}"
)