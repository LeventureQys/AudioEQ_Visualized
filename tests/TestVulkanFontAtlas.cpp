#include <QtTest>
#include <QFile>
#include "vulkan/VulkanContext.h"
#include "vulkan/VulkanFontAtlas.h"

class TestVulkanFontAtlas : public QObject {
	Q_OBJECT
private slots:
	void testAtlasCreation();
	void testGlyphMetrics();
};

void TestVulkanFontAtlas::testAtlasCreation()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	QVERIFY(ctx.initialize());

	QFile fontFile(":/fonts/NotoSans-Regular.ttf");
	QVERIFY(fontFile.open(QIODevice::ReadOnly));
	QByteArray fontData = fontFile.readAll();
	fontFile.close();

	VulkanFontAtlas fontAtlas;
	bool ok = fontAtlas.initialize(&ctx, fontData, 13.0f, 1.0f);
	QVERIFY(ok);
	QVERIFY(fontAtlas.image() != VK_NULL_HANDLE);
	QVERIFY(fontAtlas.imageView() != VK_NULL_HANDLE);
	QVERIFY(fontAtlas.sampler() != VK_NULL_HANDLE);
	QVERIFY(fontAtlas.atlasSize().width > 0);
	QVERIFY(fontAtlas.atlasSize().height > 0);

	fontAtlas.cleanup();
	ctx.cleanup();
}

void TestVulkanFontAtlas::testGlyphMetrics()
{
	if (!VulkanContext::isVulkanSupported()) {
		QSKIP("Vulkan is not available on this system");
	}

	VulkanContext ctx;
	QVERIFY(ctx.initialize());

	QFile fontFile(":/fonts/NotoSans-Regular.ttf");
	QVERIFY(fontFile.open(QIODevice::ReadOnly));
	QByteArray fontData = fontFile.readAll();
	fontFile.close();

	VulkanFontAtlas fontAtlas;
	bool ok = fontAtlas.initialize(&ctx, fontData, 13.0f, 1.0f);
	QVERIFY(ok);

	QVERIFY(fontAtlas.hasGlyph('A'));
	QVERIFY(fontAtlas.hasGlyph('0'));
	QVERIFY(!fontAtlas.hasGlyph(QChar(0x4E00)));

	const GlyphMetrics& mA = fontAtlas.glyphMetrics('A');
	QVERIFY(mA.texCoords.width() > 0.0f);
	QVERIFY(mA.texCoords.height() > 0.0f);
	QVERIFY(mA.advanceX > 0.0f);

	fontAtlas.cleanup();
	ctx.cleanup();
}

QTEST_MAIN(TestVulkanFontAtlas)
#include "TestVulkanFontAtlas.moc"
