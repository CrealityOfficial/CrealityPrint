#include "texturecreator.h"
#include "qtuser3d/utils/textureload.h"

namespace qtuser_3d
{
	Qt3DRender::QTexture2D* createFromSource(const QUrl& url)
	{
		Qt3DRender::QTextureImage* backgroundImage = new Qt3DRender::QTextureImage();
		backgroundImage->setSource(url);

		Qt3DRender::QTexture2D* imageTexture = new Qt3DRender::QTexture2D();
		imageTexture->addTextureImage(backgroundImage);
		imageTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::LinearMipMapLinear);
		imageTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::LinearMipMapLinear);
		return imageTexture;
	}

	class ImageTexture1 : public Qt3DRender::QAbstractTextureImage
	{
	public:
		ImageTexture1(const QImage& image, Qt3DCore::QNode* parent = nullptr)
			:Qt3DRender::QAbstractTextureImage(parent)
			, m_image(image)
		{

		}

		~ImageTexture1()
		{

		}

		Qt3DRender::QTextureImageDataGeneratorPtr dataGenerator() const override
		{
			Qt3DRender::QTextureImageData* textureData = new Qt3DRender::QTextureImageData();
			textureData->setImage(m_image);

			return Qt3DRender::QTextureImageDataGeneratorPtr(new LogoImageDataGenerator(textureData));
		}

		QImage m_image;
	};

	Qt3DRender::QTexture2D* createFromImage(const QImage& image)
	{
		ImageTexture1* it = new ImageTexture1(image);

		Qt3DRender::QTexture2D* imageTexture = new Qt3DRender::QTexture2D();
		imageTexture->addTextureImage(it);
		imageTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::LinearMipMapLinear);
		imageTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::LinearMipMapLinear);
		return imageTexture;
	}
}