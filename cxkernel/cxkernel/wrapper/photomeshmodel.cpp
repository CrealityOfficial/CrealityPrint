#include "photomeshmodel.h"

#include <QtGui/QImage>
#include <QtCore/QDebug>

#include "enchase/default.h"
#include "enchase/surface.h"
#include "enchase/imagematrix.h"
#include "enchase/enchaser.h"
#include "enchase/mapper.h"
#include "enchase/imagematrixfsource.h"

namespace cxkernel
{
	class TextureSurface : public enchase::Surface
	{
	public:
		TextureSurface(const QString& fileName)
			: Surface()
			, m_fileName(fileName)
		{
		}

		virtual ~TextureSurface()
		{
		}

		enchase::MatrixF* produce() override
		{
			QImage image(m_fileName);
			uint width = (uint)image.width();
			uint height = (uint)image.height();
			if (width == 0 || height == 0)
			{
				qDebug() << QString("TextureSurface produce error. [%1]").arg(m_fileName);
				return nullptr;
			}

			enchase::MatrixF* imgMtx = new enchase::MatrixF(width, height, 1);
			float* imgData = imgMtx->ptr(0);
			for (int h = 0; h < (int)height; h++)
			{
				for (int w = 0; w < (int)width; w++)
				{
					QRgb qrgb = image.pixel(w, h);
					{
						//imgData[w] = (0.212655 * qRed(qrgb) + 0.715158 * qGreen(qrgb) + 0.072187 * qBlue(qrgb)) / 255.0;
					}
					unsigned char gray = qGray(qrgb);
					imgData[w] = (float)gray / 255.0f;
				}
				imgData = imgMtx->ptr(h);
			}
			return imgMtx;
		}
	protected:
		QString m_fileName;
	};

	PhotoMeshModel::PhotoMeshModel(QObject* parent)
		: QObject(parent)
	{
	}

	PhotoMeshModel::~PhotoMeshModel()
	{
	}

	void PhotoMeshModel::setBlur(int blur)
	{
		if (blur < 0)
		{
			blur = 0;
		}
		if (blur > 18)
		{
			blur = 18;
		}
		m_param.blur = blur;
	}

	void PhotoMeshModel::setLighterOrDarker(int index)
	{
		m_param.invert = index == 0 ? true : false;
	}

	void PhotoMeshModel::setBaseHeight(float baseHeight)
	{
		if (baseHeight > 0.0f)
			m_param.baseHeight = baseHeight;
	}

	void PhotoMeshModel::setMaxHeight(float maxHeight)
	{
		if (maxHeight > 0)
			m_param.maxHeight = maxHeight;
	}

	void PhotoMeshModel::setMeshWidth(float meshX)
	{
		m_param.meshXWidth = meshX;
	}

	TriMeshPtr PhotoMeshModel::build(const QString& imageName, ccglobal::Tracer* tracer)
	{
		enchase::Enchaser enchaser;
		enchase::Mapper mapper;

		if(tracer)
			tracer->progress(0.0f);

		TextureSurface surface(imageName);
		surface.useBlur = m_param.blur;
		surface.convertType = m_param.convertType;
		surface.baseHeight = m_param.baseHeight;
		surface.maxHeight = m_param.maxHeight;
		surface.invert = m_param.invert;
		surface.edgeType = enchase::EdgeType(m_param.edgeType);
		surface.useIndex = m_param.index;
		surface.colorSegment = m_param.colorSeg;

		enchase::MatrixF* matrix = surface.matrix(nullptr);
		if(tracer)
			tracer->progress(0.9f);

		if (matrix && matrix->width() > 0 && matrix->height() > 0)
		{
			int w = matrix->width();
			int h = matrix->height();

			float pixel2space = m_param.meshXWidth / m_param.pointXNum;

			m_param.pointYNum = (int)((float)m_param.pointXNum * (float)h / (float)w);
			enchaser.setSource(enchase::defaultPlane(m_param.pointXNum, m_param.pointYNum, pixel2space, m_param.baseHeight));
			enchase::defaultCoord(m_param.pointXNum, m_param.pointYNum, mapper.allTextureGroup());

			if(tracer)
				tracer->progress(0.95f);

			enchase::MatrixFSource* source = new enchase::MatrixFSource(matrix);
			mapper.setSource(source);
			enchaser.enchaseCache(&mapper, surface.index());

			if(tracer)
				tracer->progress(1.0f);

			return TriMeshPtr(enchaser.takeCurrent());
		}

		if (tracer)
		{
			tracer->progress(1.0f);
			tracer->failed("Photo2MeshJob::createMeshFromInput error.");
		}
		return nullptr;
	}
}