cmake_minimum_required(VERSION 3.20)
project(AudioEQ VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Vulkan QUIET)

qt_add_resources(AUDIOEQ_RESOURCES resources/resources.qrc)

set(AUDIOEQ_SOURCES
    src/AudioEQ.cpp
    src/ViewEqualizer.cpp
    src/EqualizerModel.cpp
    src/CurveEngine.cpp
    src/CoordinateMapper.cpp
    src/SampleRateManager.cpp
    src/BandHandle.cpp
    src/BandFocusManager.cpp
    src/LpfHandle.cpp
    src/HpfHandle.cpp
    src/filter/ButterworthIIR.cpp
    src/filter/FilterAlgorithmFactory.cpp
    ${AUDIOEQ_RESOURCES}
)

if(Vulkan_FOUND)
    list(APPEND AUDIOEQ_SOURCES
        thirdparty/code/volk/volk.c
        src/vulkan/VulkanRenderer.cpp
        src/vulkan/VulkanContext.cpp
        src/vulkan/VulkanSwapchain.cpp
        src/vulkan/VulkanPipeline.cpp
        src/vulkan/VulkanBufferPool.cpp
        src/vulkan/VulkanFontAtlas.cpp
        src/vulkan/VulkanFrameSync.cpp
        src/vulkan/VulkanQtIntegration.cpp
    )
endif()

add_library(AudioEQ SHARED ${AUDIOEQ_SOURCES})
set_target_properties(AudioEQ PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
target_compile_definitions(AudioEQ PRIVATE AUDIOEQ_LIBRARY)
target_compile_options(AudioEQ PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/FS>)

if(Vulkan_FOUND)
    target_compile_definitions(AudioEQ PRIVATE AUDIOEQ_WITH_VULKAN VK_USE_PLATFORM_WIN32_KHR)
endif()

target_include_directories(AudioEQ PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/code/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/code/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/code/glm
)

target_link_libraries(AudioEQ PUBLIC
    Qt6::Widgets
)
if(Vulkan_FOUND)
    target_link_libraries(AudioEQ PUBLIC Vulkan::Vulkan)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    find_program(GLSLC glslc)
    if(GLSLC)
        file(GLOB SHADER_SOURCES src/vulkan/shaders/*.vert src/vulkan/shaders/*.frag)
        foreach(SHADER_FILE ${SHADER_SOURCES})
            get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
            set(SPV_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
            add_custom_command(
                OUTPUT ${SPV_OUTPUT}
                COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/${SHADER_FILE} -o ${SPV_OUTPUT}
                DEPENDS ${CMAKE_SOURCE_DIR}/${SHADER_FILE}
                COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
            )
            list(APPEND SPV_FILES ${SPV_OUTPUT})
        endforeach()
        add_custom_target(AudioEQ_Shaders DEPENDS ${SPV_FILES})
        add_dependencies(AudioEQ AudioEQ_Shaders)
    endif()
endif()

install(TARGETS AudioEQ
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

add_subdirectory(tests)
add_subdirectory(examples/SimpleDemo)
