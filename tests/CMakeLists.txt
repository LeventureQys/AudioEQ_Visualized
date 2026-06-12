include(GoogleTest)

find_package(Qt6 REQUIRED COMPONENTS Test)

set(QT_BIN_DIR "D:/Devtools/Qt/6.8.3/msvc2022_64/bin")
set(AUDIOEQ_BIN_DIR "${CMAKE_BINARY_DIR}/src/Debug")

function(add_audioeq_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE AudioEQ gtest gtest_main Qt6::Core Qt6::Test)
    if(WIN32)
        target_compile_definitions(${name} PRIVATE AUDIOEQ_EXPORT=__declspec\(dllimport\))
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${name}>
                $<TARGET_FILE_DIR:${name}>
            COMMAND_EXPAND_LISTS
        )
    endif()
    gtest_discover_tests(${name})
endfunction()

add_audioeq_test(TestButterworthIIR TestButterworthIIR.cpp)
add_audioeq_test(TestCoordinateMapper TestCoordinateMapper.cpp)
add_audioeq_test(TestCurveEngine TestCurveEngine.cpp)

# Standalone Vulkan pipeline test (no gtest, console app)
add_executable(test_vulkan_pipeline test_vulkan_pipeline.cpp)
target_link_libraries(test_vulkan_pipeline PRIVATE AudioEQ Qt6::Core)
if(WIN32)
    target_compile_definitions(test_vulkan_pipeline PRIVATE AUDIOEQ_EXPORT=__declspec\(dllimport\))
    add_custom_command(TARGET test_vulkan_pipeline POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:test_vulkan_pipeline>
            $<TARGET_FILE_DIR:test_vulkan_pipeline>
        COMMAND_EXPAND_LISTS
    )
endif()
