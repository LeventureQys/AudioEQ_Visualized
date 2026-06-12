# Export macro for shared library
if(BUILD_SHARED_LIBS)
    if(WIN32)
        add_compile_definitions(AUDIOEQ_EXPORT=__declspec\(dllexport\))
    else()
        add_compile_definitions(AUDIOEQ_EXPORT=__attribute__\(\(visibility\(\"default\"\)\)\))
    endif()
endif()

add_library(AudioEQ
    # Core
    AudioEQ.h          AudioEQ.cpp
    AudioEQTypes.h
    CoordinateMapper.h CoordinateMapper.cpp
    EqualizerModel.h   EqualizerModel.cpp
    CurveEngine.h      CurveEngine.cpp

    # UI
    ViewEqualizer.h    ViewEqualizer.cpp
    BandHandle.h       BandHandle.cpp
    LpfHandle.h        LpfHandle.cpp
    HpfHandle.h        HpfHandle.cpp

    # Filter
    filter/FilterAlgorithm.h
    filter/FilterAlgorithmFactory.h  filter/FilterAlgorithmFactory.cpp
    filter/ButterworthIIR.h          filter/ButterworthIIR.cpp

    # Vulkan
    vulkan/VulkanRenderer.h        vulkan/VulkanRenderer.cpp
    vulkan/VulkanQtIntegration.h   vulkan/VulkanQtIntegration.cpp
    vulkan/VulkanContext.h         vulkan/VulkanContext.cpp
    vulkan/VulkanSwapchain.h       vulkan/VulkanSwapchain.cpp
    vulkan/VulkanPipeline.h        vulkan/VulkanPipeline.cpp
    vulkan/VulkanBufferPool.h      vulkan/VulkanBufferPool.cpp
    vulkan/VulkanFontAtlas.h       vulkan/VulkanFontAtlas.cpp
    vulkan/VulkanFrameSync.h       vulkan/VulkanFrameSync.cpp
)

target_include_directories(AudioEQ PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(AudioEQ PUBLIC
    Qt6::Widgets
    Qt6::Gui
    volk
    glm
)

# ── Embed SPIR-V shaders ──────────────────────
include(EmbedSpirv)

set(SPV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders/spv)
embed_spirv(
    ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedShaders.h
    ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedShaders.cpp
    ${SPV_DIR}/grid.vert.spv
    ${SPV_DIR}/grid.frag.spv
    ${SPV_DIR}/curve.vert.spv
    ${SPV_DIR}/curve.frag.spv
    ${SPV_DIR}/fill.vert.spv
    ${SPV_DIR}/fill.frag.spv
    ${SPV_DIR}/glyph.vert.spv
    ${SPV_DIR}/glyph.frag.spv
)
set(EMBEDDED_SHADERS_SRC ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedShaders.h ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedShaders.cpp)

target_sources(AudioEQ PRIVATE ${EMBEDDED_SHADERS_SRC})
target_include_directories(AudioEQ PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# ── Embed font ─────────────────────────────────
include(EmbedFont)
set(FONT_FILE ${CMAKE_SOURCE_DIR}/thirdparty/font/Noto_Sans/static/NotoSans-Regular.ttf)
embed_font(
    ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedFont.h
    ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedFont.cpp
    ${FONT_FILE}
    noto_sans_regular
)
set(EMBEDDED_FONT_SRC ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedFont.h ${CMAKE_CURRENT_BINARY_DIR}/EmbeddedFont.cpp)
target_sources(AudioEQ PRIVATE ${EMBEDDED_FONT_SRC})
