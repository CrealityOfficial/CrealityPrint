#ifndef TEXTURE_LOAD_H
#define TEXTURE_LOAD_H

#include "qtuser3d/qtuser3dexport.h"
#include <qtextureimagedatagenerator.h>
#include <qabstracttextureimage.h>


namespace qtuser_3d
{

class QTUSER_3D_API LogoImageDataGenerator : public Qt3DRender::QTextureImageDataGenerator
{
	Qt3DRender::QTextureImageData* m_textureData;

public:
	LogoImageDataGenerator(Qt3DRender::QTextureImageData* textureData);

	virtual ~LogoImageDataGenerator() override;

	Qt3DRender::QTextureImageDataPtr operator()() override;

	bool operator ==(const QTextureImageDataGenerator& other) const override;


	QT3D_FUNCTOR(LogoImageDataGenerator)

};


class QTUSER_3D_API LogoTextureImage : public Qt3DRender::QAbstractTextureImage
{

public:
	LogoTextureImage(Qt3DCore::QNode* p = nullptr);

	Qt3DRender::QTextureImageDataGeneratorPtr dataGenerator() const;
};


extern uchar logodata[];


class QTUSER_3D_API ImageTexture : public Qt3DRender::QAbstractTextureImage
{
	QString m_filename;
	int m_imageWidth;
	int m_imageHeight;
	QImage* m_image;

public:
	ImageTexture(QString filename, Qt3DCore::QNode* p = nullptr);

	Qt3DRender::QTextureImageDataGeneratorPtr dataGenerator() const;

	int width() const;
	int height() const;
};

}


#endif //