project(HornetExecute VERSION 1.0 LANGUAGES CXX)

file(GLOB_RECURSE HORNETEXECUTE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
)

file(GLOB_RECURSE HORNETEXECUTE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HornetBase/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HornetUtil/*.h
)

add_library(HornetExecute SHARED 
    ${HORNETEXECUTE_SOURCES}
    ${HORNETEXECUTE_HEADERS}
)

target_include_directories(HornetExecute
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(HornetExecute PRIVATE HORNETEXECUTE_LIBRARY)

target_link_libraries(HornetExecute
    PUBLIC 
    HornetBase 
    HornetUtil
)
set_target_properties(HornetExecute PROPERTIES
    PUBLIC_HEADER "${HORNETEXECUTE_HEADERS}"
)