#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDir>

class TestShaderCompile : public QObject
{
	Q_OBJECT

private:
	QString shaderDir;
	QString glslcPath;
	bool glslcAvailable;

	void initTestCase()
	{
		shaderDir = QDir::cleanPath(QCoreApplication::applicationDirPath()
			+ "/../../src/vulkan/shaders/");

		QProcess proc;
		proc.start("glslc", {"--version"});
		glslcAvailable = proc.waitForFinished(3000) && proc.exitCode() == 0;

		if (!glslcAvailable) {
			proc.start("glslc.exe", {"--version"});
			glslcAvailable = proc.waitForFinished(3000) && proc.exitCode() == 0;
		}

		if (glslcAvailable)
			glslcPath = "glslc";
	}

	void verifyFileExists(const QString &name)
	{
		QFile file(shaderDir + name);
		QVERIFY2(file.exists(), qPrintable(name + " does not exist"));
		QVERIFY2(file.size() > 0, qPrintable(name + " is empty"));
	}

	void compileShader(const QString &name)
	{
		if (!glslcAvailable)
			QSKIP("glslc not found in PATH");

		QString inputPath = shaderDir + name;
		QString outputPath = QDir::tempPath() + "/" + name + ".spv";

		QProcess proc;
		proc.start(glslcPath, {"-o", outputPath, inputPath});
		QVERIFY2(proc.waitForFinished(10000),
			qPrintable("glslc timed out for " + name));
		QVERIFY2(proc.exitCode() == 0,
			qPrintable("glslc failed for " + name + ": " + proc.readAllStandardError()));

		QFile::remove(outputPath);
	}

private slots:
	void testShaderFilesExist()
	{
		verifyFileExists("grid.vert");
		verifyFileExists("grid.frag");
		verifyFileExists("curve.vert");
		verifyFileExists("curve.frag");
		verifyFileExists("fill.vert");
		verifyFileExists("fill.frag");
		verifyFileExists("glyph.vert");
		verifyFileExists("glyph.frag");
	}

	void testGridShaderCompiles()
	{
		compileShader("grid.vert");
		compileShader("grid.frag");
	}

	void testCurveShaderCompiles()
	{
		compileShader("curve.vert");
		compileShader("curve.frag");
	}

	void testFillShaderCompiles()
	{
		compileShader("fill.vert");
		compileShader("fill.frag");
	}

	void testGlyphShaderCompiles()
	{
		compileShader("glyph.vert");
		compileShader("glyph.frag");
	}
};

QTEST_MAIN(TestShaderCompile)
#include "TestShaderCompile.moc"
