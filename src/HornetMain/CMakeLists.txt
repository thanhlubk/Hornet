project(HornetMain LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

qt_standard_project_setup()
qt_add_resources(APP_RESOURCE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)

# Automatically collect source files
file(GLOB_RECURSE APP_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.ui
)

add_executable(HornetMain 
    ${APP_SOURCES}
    ${APP_RESOURCE_RESOURCES})

qt6_add_lupdate(
    SOURCES
        ${APP_SOURCES}
    TS_FILES
        ${CMAKE_SOURCE_DIR}/translations/HornetMain_en.ts
        ${CMAKE_SOURCE_DIR}/translations/HornetMain_vi.ts
)

add_dependencies(HornetMain HornetMain_lupdate)

qt6_add_lrelease(
    TS_FILES
        ${CMAKE_SOURCE_DIR}/translations/HornetMain_en.ts
        ${CMAKE_SOURCE_DIR}/translations/HornetMain_vi.ts
    QM_FILES_OUTPUT_VARIABLE QM_OUTPUT
)

# install(FILES ${QM_OUTPUT} DESTINATION ${CMAKE_BINARY_DIR}/translations)
install(FILES ${QM_OUTPUT} DESTINATION ${CMAKE_SOURCE_DIR}/translations)

target_include_directories(HornetMain
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(HornetMain
    PRIVATE
    Qt6::Widgets
    FancyWidgets  # This is already defined in the top-level CMake
    HornetView
    HornetBase
    HornetExecute
    HornetUtil
)

# Auto-copy FancyWidgets.dll in bin to the build folder after build
add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FancyWidgets>
        $<TARGET_FILE_DIR:HornetMain>
)

add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:HornetView>
        $<TARGET_FILE_DIR:HornetMain>
)

add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:HornetBase>
        $<TARGET_FILE_DIR:HornetMain>
)

add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:HornetExecute>
        $<TARGET_FILE_DIR:HornetMain>
)

add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:HornetUtil>
        $<TARGET_FILE_DIR:HornetMain>
)

# Auto-copy Qt DLLs from bin/{Debug|Release} to build/{Debug|Release}
add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/$<CONFIG>
        $<TARGET_FILE_DIR:HornetMain>
)

add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/plugins/${PLATFORM_NAME}
        $<TARGET_FILE_DIR:HornetMain>
)

add_custom_command(TARGET HornetMain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:HornetMain>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QM_OUTPUT}
        $<TARGET_FILE_DIR:HornetMain>/translations
)