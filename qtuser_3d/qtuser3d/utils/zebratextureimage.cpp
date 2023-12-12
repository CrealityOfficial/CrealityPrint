#include "zebratextureimage.h"

namespace qtuser_3d
{
	ZebraImageDataGenerator::ZebraImageDataGenerator(Qt3DRender::QTextureImageDataPtr data)
		:m_textureData(data)
	{
	}

	ZebraImageDataGenerator::~ZebraImageDataGenerator()
	{
	}

	Qt3DRender::QTextureImageDataPtr ZebraImageDataGenerator::operator()()
	{
		return m_textureData;
	}

	bool ZebraImageDataGenerator::operator ==(const Qt3DRender::QTextureImageDataGenerator& other) const
	{
		const ZebraImageDataGenerator* otherFunctor = Qt3DRender::functor_cast<ZebraImageDataGenerator>(&other);
		return (otherFunctor == this);
	}

	ZebraTextureImage::ZebraTextureImage(const QList<float>& values, float bottom, float top,
		const QVector4D& baseColor, const QVector4D& sColor, Qt3DCore::QNode* node)
		: QAbstractTextureImage(node)
		, m_values(values)
		, m_bottom(bottom)
		, m_top(top)
		, m_baseColor(baseColor)
		, m_sColor(sColor)
	{
	}

	Qt3DRender::QTextureImageDataGeneratorPtr ZebraTextureImage::dataGenerator() const
	{
		Qt3DRender::QTextureImageData* textureData = new Qt3DRender::QTextureImageData();
		QByteArray bytes;
		bytes.resize(200 * 4);
		unsigned char* d = (unsigned char*)bytes.data();

		std::vector<bool> flags(200, false);
		float delta = m_top - m_bottom;
		if (delta < 1.0f)
			delta = 1.0f;
		for (float v : m_values)
		{
			int index = (int)(200.0f * (v - m_bottom) / delta);
			if (index > 0 && index < 200)
				flags.at(index) = true;
		}

		bool useBase = true;
		for (int i = 0; i < 200; ++i)
		{
			if (flags.at(i))
				useBase = !useBase;

			QVector4D color = useBase ? m_baseColor : m_sColor;
			unsigned char* dd = d + 4 * i;
			for (int j = 0; j < 3; ++j)
				*(dd + j) = (unsigned char)(255.0f * color[j]);
			*(dd + 3) = 255;
		}
		textureData->setData(bytes, 4);
		textureData->setWidth(200);
		textureData->setHeight(1);
		textureData->setDepth(1);
		textureData->setFaces(1);
		textureData->setLayers(1);
		textureData->setMipLevels(1);
		textureData->setPixelFormat(QOpenGLTexture::PixelFormat::RGBA);
		textureData->setPixelType(QOpenGLTexture::PixelType::UInt8);
		textureData->setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
		textureData->setTarget(QOpenGLTexture::Target::Target1D);

		return Qt3DRender::QTextureImageDataGeneratorPtr(
			new ZebraImageDataGenerator(Qt3DRender::QTextureImageDataPtr(textureData)));
	}
}