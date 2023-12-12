#ifndef QTUSER_3D_ZEBRATEXTUREIMAGE_1614918060933_H
#define QTUSER_3D_ZEBRATEXTUREIMAGE_1614918060933_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QTextureImageDataGenerator>
#include <Qt3DRender/QTextureImage>
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class ZebraImageDataGenerator : public Qt3DRender::QTextureImageDataGenerator
	{
	public:
		ZebraImageDataGenerator(Qt3DRender::QTextureImageDataPtr data);
		virtual ~ZebraImageDataGenerator() override;

		Qt3DRender::QTextureImageDataPtr operator()() override;
		bool operator ==(const QTextureImageDataGenerator& other) const override;

		QT3D_FUNCTOR(ZebraImageDataGenerator)
	protected:
		Qt3DRender::QTextureImageDataPtr m_textureData;
	};

	class QTUSER_3D_API ZebraTextureImage : public Qt3DRender::QAbstractTextureImage
	{
	public:
		ZebraTextureImage(const QList<float>& values, float bottom, float top, 
			const QVector4D& baseColor, const QVector4D& sColor, Qt3DCore::QNode* node = nullptr);
		Qt3DRender::QTextureImageDataGeneratorPtr dataGenerator() const;

	protected:
		QList<float> m_values;
		float m_bottom;
		float m_top;
		QVector4D m_baseColor;
		QVector4D m_sColor;
	};
}

#endif // QTUSER_3D_ZEBRATEXTUREIMAGE_1614918060933_H