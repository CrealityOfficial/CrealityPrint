#include "qtuser3d/framegraph/texturerendertarget.h"
#include "qtuser3d//module/glcompatibility.h"

namespace qtuser_3d
{
    TextureRenderTarget::TextureRenderTarget(Qt3DCore::QNode* parent, const QSize& size, bool createPosTexture, bool createNormalTexture)
        :QRenderTarget(parent)
        , m_size(size)
        , m_colorOutput(nullptr)
        , m_colorTexture(nullptr)
        , m_depthOutput(nullptr)
        , m_depthTexture(nullptr)
        , m_worldPosOutput(nullptr)
        , m_worldPosTexture(nullptr)
        , m_worldNormalOutput(nullptr)
        , m_worldNormalTexture(nullptr)

    {
        // Create a render target output for rendering colour.
        m_colorOutput = new Qt3DRender::QRenderTargetOutput(this);
        m_colorOutput->setAttachmentPoint(Qt3DRender::QRenderTargetOutput::Color0);

        // Create a texture to render into.
        m_colorTexture = new Qt3DRender::QTexture2D(m_colorOutput);
        m_colorTexture->setSize(size.width(), size.height());
        if (qtuser_3d::isGles())
        {
            m_colorTexture->setFormat(Qt3DRender::QAbstractTexture::RGBAFormat); //Automatic ; RGBAFormat; RGBA4
        }
        else
        {
            m_colorTexture->setFormat(Qt3DRender::QAbstractTexture::RGBA8_UNorm); //   RGBA8_UNorm; Unsigned normalized formats,  will cause effect for the pick pass of the xyz indicator( object translate )
        }
        m_colorTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
        m_colorTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);

        // Hook the texture up to our output, and the output up to this object.
        m_colorOutput->setTexture(m_colorTexture);
        addOutput(m_colorOutput);

        if (createPosTexture)
        {
            m_worldPosOutput = new Qt3DRender::QRenderTargetOutput(this);
            m_worldPosOutput->setAttachmentPoint(Qt3DRender::QRenderTargetOutput::Color1);

            m_worldPosTexture = new Qt3DRender::QTexture2D(m_worldPosOutput);
            m_worldPosTexture->setSize(size.width(), size.height());
            m_worldPosTexture->setFormat(Qt3DRender::QAbstractTexture::RGB16F);
            m_worldPosTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
            m_worldPosTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);

            m_worldPosOutput->setTexture(m_worldPosTexture);
            addOutput(m_worldPosOutput);
        }

        if (createNormalTexture)
        {
            m_worldNormalOutput = new Qt3DRender::QRenderTargetOutput(this);
            m_worldNormalOutput->setAttachmentPoint(Qt3DRender::QRenderTargetOutput::Color2);

            m_worldNormalTexture = new Qt3DRender::QTexture2D(m_worldNormalOutput);
            m_worldNormalTexture->setSize(size.width(), size.height());
            m_worldNormalTexture->setFormat(Qt3DRender::QAbstractTexture::RGB16F);
            m_worldNormalTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
            m_worldNormalTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);

            m_worldNormalOutput->setTexture(m_worldNormalTexture);
            addOutput(m_worldNormalOutput);
        }
		
        m_depthOutput = new Qt3DRender::QRenderTargetOutput(this);
        m_depthOutput->setAttachmentPoint(Qt3DRender::QRenderTargetOutput::Depth);
        m_depthTexture = new Qt3DRender::QTexture2D(m_depthOutput);
        m_depthTexture->setSize(size.width(), size.height());

        if (qtuser_3d::isGles())
        {
            //ToDo
            m_depthTexture->setFormat(Qt3DRender::QAbstractTexture::DepthFormat);//D16  ; Automatic  ; DepthFormat
        }
        else
        {
            m_depthTexture->setFormat(Qt3DRender::QAbstractTexture::DepthFormat);
        }

        m_depthTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);// Linear
        m_depthTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);

        // Hook up the depth texture
        m_depthOutput->setTexture(m_depthTexture);

        if (qtuser_3d::isGles())
        {
            // ToDo : GLES 2.0 supports a depth renderbuffer but not a depth texture buffer.
            //addOutput(m_depthOutput);
        }
        else
        {
            addOutput(m_depthOutput);
        }
        
    }

    TextureRenderTarget::~TextureRenderTarget()
    {

    }

    void TextureRenderTarget::resize(const QSize& size)
    {
        if (size.width() == 0 || size.height() == 0 || m_size == size)
            return;

        m_size = size;
        m_depthTexture->setSize(m_size.width(), m_size.height());
        m_colorTexture->setSize(m_size.width(), m_size.height());
        if (m_worldPosTexture)
        {
            m_worldPosTexture->setSize(m_size.width(), m_size.height());
        }
        if (m_worldNormalTexture)
        {
            m_worldNormalTexture->setSize(m_size.width(), m_size.height());
        }
    }

	Qt3DRender::QTexture2D* TextureRenderTarget::colorTexture()
	{
		return m_colorTexture;
	}

    Qt3DRender::QTexture2D* TextureRenderTarget::depthTexture()
    {
        return m_depthTexture;
    }

    Qt3DRender::QTexture2D* TextureRenderTarget::worldPosTexture()
    {
        return m_worldPosTexture;
    }

    Qt3DRender::QTexture2D* TextureRenderTarget::worldNormalTexture()
    {
        return m_worldNormalTexture;
    }
}
