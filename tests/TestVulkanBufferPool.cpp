#include <QtTest>
#include <cstring>
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanBufferPool.h"

class TestVulkanBufferPool : public QObject {
	Q_OBJECT
private slots:
	void testVertexBuffer();
	void testIndexBuffer();
	void testUniformBuffer();
	void testBufferCleanup();
};

void TestVulkanBufferPool::testVertexBuffer()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	QVERIFY(ctx.initialize());

	VulkanBufferPool pool;
	QVERIFY(pool.initialize(&ctx));

	float vertexData[9] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f };
	VkBuffer vb = pool.allocateVertexBuffer(sizeof(vertexData), vertexData);
	QVERIFY(vb != VK_NULL_HANDLE);

	pool.freeBuffer(vb);
	ctx.cleanup();
}

void TestVulkanBufferPool::testIndexBuffer()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	QVERIFY(ctx.initialize());

	VulkanBufferPool pool;
	QVERIFY(pool.initialize(&ctx));

	uint16_t indexData[3] = { 0, 1, 2 };
	VkBuffer ib = pool.allocateIndexBuffer(sizeof(indexData), indexData);
	QVERIFY(ib != VK_NULL_HANDLE);

	pool.freeBuffer(ib);
	ctx.cleanup();
}

void TestVulkanBufferPool::testUniformBuffer()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	QVERIFY(ctx.initialize());

	VulkanBufferPool pool;
	QVERIFY(pool.initialize(&ctx));

	float uniformData[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
	VkBuffer ub = pool.allocateUniformBuffer(sizeof(uniformData));
	QVERIFY(ub != VK_NULL_HANDLE);

	pool.updateUniformBuffer(ub, sizeof(uniformData), uniformData);

	float expected[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
	pool.updateUniformBuffer(ub, sizeof(expected), expected);

	pool.freeBuffer(ub);
	ctx.cleanup();
}

void TestVulkanBufferPool::testBufferCleanup()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	QVERIFY(ctx.initialize());

	VulkanBufferPool pool;
	QVERIFY(pool.initialize(&ctx));

	float data[12] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	for (int i = 0; i < 10; ++i) {
		VkBuffer vb = pool.allocateVertexBuffer(sizeof(data), data);
		QVERIFY(vb != VK_NULL_HANDLE);
		pool.freeBuffer(vb);
	}

	ctx.cleanup();
}

QTEST_MAIN(TestVulkanBufferPool)
#include "TestVulkanBufferPool.moc"
