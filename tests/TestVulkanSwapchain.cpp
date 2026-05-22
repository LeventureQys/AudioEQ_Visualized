#ifdef Q_OS_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <QtTest>
#include <QWindow>
#include <QGuiApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanSwapchain.h"

class TestVulkanSwapchain : public QObject {
	Q_OBJECT
private slots:
	void testCreateAndDestroy();
	void testRecreate();
	void testMSAASamples();
};

static VkSurfaceKHR createTestSurface(VulkanContext* ctx, QWindow* window)
{
#ifdef Q_OS_WIN
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hinstance = GetModuleHandle(nullptr);
	createInfo.hwnd = reinterpret_cast<HWND>(window->winId());

	if (!createInfo.hwnd) {
		return VK_NULL_HANDLE;
	}

	auto pfnCreateWin32Surface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
		vkGetInstanceProcAddr(ctx->instance(), "vkCreateWin32SurfaceKHR"));
	if (!pfnCreateWin32Surface) {
		return VK_NULL_HANDLE;
	}

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result = pfnCreateWin32Surface(ctx->instance(), &createInfo, nullptr, &surface);
	if (result != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}
	return surface;
#else
	Q_UNUSED(ctx);
	Q_UNUSED(window);
	return VK_NULL_HANDLE;
#endif
}

static void destroyTestSurface(VulkanContext* ctx, VkSurfaceKHR surface)
{
	if (surface == VK_NULL_HANDLE) {
		return;
	}
	auto pfnDestroySurface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
		vkGetInstanceProcAddr(ctx->instance(), "vkDestroySurfaceKHR"));
	if (pfnDestroySurface) {
		pfnDestroySurface(ctx->instance(), surface, nullptr);
	}
}

void TestVulkanSwapchain::testCreateAndDestroy()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	if (!ctx.initialize()) {
		QSKIP("Failed to initialize VulkanContext");
	}

	int argc = 0;
	QGuiApplication app(argc, nullptr);

	QWindow window;
	window.setSurfaceType(QSurface::VulkanSurface);
	window.resize(800, 600);
	window.create();
	if (!window.isVisible()) {
		window.show();
	}
	QTest::qWait(10);

	VkSurfaceKHR surface = createTestSurface(&ctx, &window);
	if (surface == VK_NULL_HANDLE) {
		QSKIP("Cannot create Vulkan surface (missing VK_KHR_win32_surface extension)");
	}

	VulkanSwapchain swapchain;
	bool ok = swapchain.initialize(&ctx, surface, QSize(800, 600), 1.0f);
	destroyTestSurface(&ctx, surface);

	if (!ok) {
		QSKIP("Swapchain creation failed (missing device extension or incompatible surface)");
	}
	QVERIFY(swapchain.swapchain() != VK_NULL_HANDLE);
	QVERIFY(swapchain.renderPass() != VK_NULL_HANDLE);
	QVERIFY(swapchain.imageCount() > 0);
	QCOMPARE(swapchain.extent().width, 800u);
	QCOMPARE(swapchain.extent().height, 600u);

	swapchain.cleanup();
	ctx.cleanup();
}

void TestVulkanSwapchain::testRecreate()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	if (!ctx.initialize()) {
		QSKIP("Failed to initialize VulkanContext");
	}

	int argc = 0;
	QGuiApplication app(argc, nullptr);

	QWindow window;
	window.setSurfaceType(QSurface::VulkanSurface);
	window.resize(800, 600);
	window.create();
	if (!window.isVisible()) {
		window.show();
	}
	QTest::qWait(10);

	VkSurfaceKHR surface = createTestSurface(&ctx, &window);
	if (surface == VK_NULL_HANDLE) {
		QSKIP("Cannot create Vulkan surface (missing VK_KHR_win32_surface extension)");
	}

	VulkanSwapchain swapchain;
	bool ok = swapchain.initialize(&ctx, surface, QSize(800, 600), 1.0f);
	destroyTestSurface(&ctx, surface);

	if (!ok) {
		QSKIP("Swapchain creation failed (missing device extension)");
	}

	QWindow window2;
	window2.setSurfaceType(QSurface::VulkanSurface);
	window2.resize(400, 300);
	window2.create();
	if (!window2.isVisible()) {
		window2.show();
	}
	QTest::qWait(10);

	VkSurfaceKHR surface2 = createTestSurface(&ctx, &window2);
	if (surface2 == VK_NULL_HANDLE) {
		swapchain.cleanup();
		ctx.cleanup();
		QSKIP("Cannot create second Vulkan surface");
	}

	VulkanSwapchain swapchain2;
	bool ok2 = swapchain2.initialize(&ctx, surface2, QSize(400, 300), 2.0f);
	destroyTestSurface(&ctx, surface2);

	if (!ok2) {
		swapchain.cleanup();
		ctx.cleanup();
		QSKIP("Second swapchain creation failed");
	}

	QVERIFY(swapchain2.swapchain() != VK_NULL_HANDLE);
	QVERIFY(swapchain2.imageCount() > 0);
	QCOMPARE(swapchain2.extent().width, 800u);
	QCOMPARE(swapchain2.extent().height, 600u);

	swapchain2.cleanup();

	bool reOk = swapchain.recreate(QSize(1024, 768), 1.0f);
	QVERIFY(reOk);
	QVERIFY(swapchain.imageCount() > 0);
	QCOMPARE(swapchain.extent().width, 1024u);
	QCOMPARE(swapchain.extent().height, 768u);

	swapchain.cleanup();
	ctx.cleanup();
}

void TestVulkanSwapchain::testMSAASamples()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	if (!ctx.initialize()) {
		QSKIP("Failed to initialize VulkanContext");
	}

	VkSampleCountFlagBits ctxSamples = ctx.msaaSamples();

	int argc = 0;
	QGuiApplication app(argc, nullptr);

	QWindow window;
	window.setSurfaceType(QSurface::VulkanSurface);
	window.resize(800, 600);
	window.create();
	if (!window.isVisible()) {
		window.show();
	}
	QTest::qWait(10);

	VkSurfaceKHR surface = createTestSurface(&ctx, &window);
	if (surface == VK_NULL_HANDLE) {
		QSKIP("Cannot create Vulkan surface (missing VK_KHR_win32_surface extension)");
	}

	VulkanSwapchain swapchain;
	bool ok = swapchain.initialize(&ctx, surface, QSize(800, 600), 1.0f);
	destroyTestSurface(&ctx, surface);

	if (!ok) {
		QSKIP("Swapchain creation failed (missing device extension)");
	}

	VkSampleCountFlagBits scSamples = swapchain.msaaSamples();
	QCOMPARE(scSamples, ctxSamples);

	if (scSamples != VK_SAMPLE_COUNT_1_BIT) {
		QVERIFY(scSamples == VK_SAMPLE_COUNT_2_BIT ||
			scSamples == VK_SAMPLE_COUNT_4_BIT ||
			scSamples == VK_SAMPLE_COUNT_8_BIT ||
			scSamples == VK_SAMPLE_COUNT_16_BIT ||
			scSamples == VK_SAMPLE_COUNT_32_BIT ||
			scSamples == VK_SAMPLE_COUNT_64_BIT);
	}

	swapchain.cleanup();
	ctx.cleanup();
}

QTEST_MAIN(TestVulkanSwapchain)
#include "TestVulkanSwapchain.moc"
