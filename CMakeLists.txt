cmake_minimum_required(VERSION 3.20)
project(AudioEQ VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_SHARED_LIBS "Build shared library" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

# volk (Vulkan loader) — NO find_package(Vulkan), use built-in headers
add_library(volk STATIC thirdparty/code/volk/volk.c)
target_include_directories(volk PUBLIC
    thirdparty/code/volk
    thirdparty/code/vulkan-headers/include)

if(WIN32)
    target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_METAL_EXT)
elseif(UNIX)
    target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_XCB_KHR VK_USE_PLATFORM_WAYLAND_KHR)
endif()

# GLM (header-only, for Vulkan layer only)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE thirdparty/code/glm)

# googletest (conditional — submodule or built-in)
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/code/googletest/CMakeLists.txt")
    add_subdirectory(thirdparty/code/googletest)
endif()

add_subdirectory(src)
add_subdirectory(examples/SimpleDemo)
enable_testing()
add_subdirectory(tests)
