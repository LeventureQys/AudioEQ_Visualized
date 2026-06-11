add_executable(SimpleDemo main.cpp)
target_link_libraries(SimpleDemo PRIVATE AudioEQ Qt6::Widgets)

# Auto-deploy runtime DLLs next to the executable
add_custom_command(TARGET SimpleDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:SimpleDemo>
        $<TARGET_FILE_DIR:SimpleDemo>
    COMMAND_EXPAND_LISTS
)

# Copy Qt platform plugin
add_custom_command(TARGET SimpleDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:SimpleDemo>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/Devtools/Qt/6.8.3/msvc2022_64/plugins/platforms/qwindowsd.dll"
        "$<TARGET_FILE_DIR:SimpleDemo>/platforms/"
)

# Copy OpenGL software renderer fallback (used by Qt6Gui)
add_custom_command(TARGET SimpleDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/Devtools/Qt/6.8.3/msvc2022_64/bin/opengl32sw.dll"
        "$<TARGET_FILE_DIR:SimpleDemo>/"
)

# Generate qt.conf for plugin discovery
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
    "[Platforms]\n"
    "Arguments = platforms\n"
)
add_custom_command(TARGET SimpleDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        "$<TARGET_FILE_DIR:SimpleDemo>/qt.conf"
)
