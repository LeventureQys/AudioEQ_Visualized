find_package(Qt6 REQUIRED COMPONENTS Test)
enable_testing()

add_executable(TestBuildValidation
    TestBuildValidation.cpp
)

target_link_libraries(TestBuildValidation PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestBuildValidation COMMAND TestBuildValidation)

add_executable(TestEqualizerModel
    TestEqualizerModel.cpp
)

target_link_libraries(TestEqualizerModel PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestEqualizerModel COMMAND TestEqualizerModel)

add_executable(TestFilterAlgorithm
    TestFilterAlgorithm.cpp
)

target_link_libraries(TestFilterAlgorithm PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestFilterAlgorithm COMMAND TestFilterAlgorithm)

add_executable(TestVulkanContext
    TestVulkanContext.cpp
)

target_link_libraries(TestVulkanContext PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestVulkanContext COMMAND TestVulkanContext)

add_executable(TestTypes
    TestTypes.cpp
)

target_link_libraries(TestTypes PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestTypes COMMAND TestTypes)

add_executable(TestCoordinateMapper
    TestCoordinateMapper.cpp
)

target_link_libraries(TestCoordinateMapper PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestCoordinateMapper COMMAND TestCoordinateMapper)

add_executable(TestShaderCompile
    TestShaderCompile.cpp
)

target_link_libraries(TestShaderCompile PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestShaderCompile COMMAND TestShaderCompile)

add_executable(TestVulkanFontAtlas
    TestVulkanFontAtlas.cpp
)

target_link_libraries(TestVulkanFontAtlas PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestVulkanFontAtlas COMMAND TestVulkanFontAtlas)

add_executable(TestVulkanSwapchain
    TestVulkanSwapchain.cpp
)

target_link_libraries(TestVulkanSwapchain PRIVATE
    AudioEQ
    Qt6::Test
)

target_compile_definitions(TestVulkanSwapchain PRIVATE VK_USE_PLATFORM_WIN32_KHR)

add_test(NAME TestVulkanSwapchain COMMAND TestVulkanSwapchain)

add_executable(TestButterworthIIR
    TestButterworthIIR.cpp
)

target_link_libraries(TestButterworthIIR PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestButterworthIIR COMMAND TestButterworthIIR)

add_executable(TestVulkanBufferPool
    TestVulkanBufferPool.cpp
)

target_link_libraries(TestVulkanBufferPool PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestVulkanBufferPool COMMAND TestVulkanBufferPool)

add_executable(TestCurveEngine
    TestCurveEngine.cpp
)

target_link_libraries(TestCurveEngine PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestCurveEngine COMMAND TestCurveEngine)

add_executable(TestVulkanPipeline
    TestVulkanPipeline.cpp
)

target_link_libraries(TestVulkanPipeline PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestVulkanPipeline COMMAND TestVulkanPipeline)

add_executable(TestVulkanFrameSync
    TestVulkanFrameSync.cpp
)

target_link_libraries(TestVulkanFrameSync PRIVATE
    AudioEQ
    Qt6::Test
)

target_compile_definitions(TestVulkanFrameSync PRIVATE VK_USE_PLATFORM_WIN32_KHR)

add_test(NAME TestVulkanFrameSync COMMAND TestVulkanFrameSync)

add_executable(TestVulkanRenderer
    TestVulkanRenderer.cpp
)

target_link_libraries(TestVulkanRenderer PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestVulkanRenderer COMMAND TestVulkanRenderer)

add_executable(TestBandHandle
    TestBandHandle.cpp
)

target_link_libraries(TestBandHandle PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestBandHandle COMMAND TestBandHandle)

add_executable(TestSampleRateManager
    TestSampleRateManager.cpp
)

target_link_libraries(TestSampleRateManager PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestSampleRateManager COMMAND TestSampleRateManager)

add_executable(TestLpfHpfHandle
    TestLpfHpfHandle.cpp
)

target_link_libraries(TestLpfHpfHandle PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestLpfHpfHandle COMMAND TestLpfHpfHandle)

add_executable(TestVulkanQtIntegration
    TestVulkanQtIntegration.cpp
)

target_link_libraries(TestVulkanQtIntegration PRIVATE
    AudioEQ
    Qt6::Test
)

add_executable(TestAudioEQ
    TestAudioEQ.cpp
)

target_link_libraries(TestAudioEQ PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestAudioEQ COMMAND TestAudioEQ)

add_test(NAME TestVulkanQtIntegration COMMAND TestVulkanQtIntegration)

add_executable(TestBandFocusManager
    TestBandFocusManager.cpp
)

target_link_libraries(TestBandFocusManager PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestBandFocusManager COMMAND TestBandFocusManager)

add_executable(TestCompatibility
    TestCompatibility.cpp
)

target_link_libraries(TestCompatibility PRIVATE
    AudioEQ
    Qt6::Test
)

add_test(NAME TestCompatibility COMMAND TestCompatibility)
