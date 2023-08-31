#include "photo2meshjob.h"

#include <QtGui/QImage>
#include <QtCore/QDebug>

#include "cxkernel/interface/modelninterface.h"
#include "qtusercore/module/progressortracer.h"

#include "enchase/default.h"
#include "enchase/surface.h"
#include "enchase/imagematrix.h"
#include "enchase/enchaser.h"
#include "enchase/mapper.h"
#include "enchase/imagematrixfsource.h"

using namespace creative_kernel;
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

Photo2MeshJob::Photo2MeshJob(QObject* parent)
	: qtuser_core::Job(parent)
{
}

Photo2MeshJob::~Photo2MeshJob()
{
}

void Photo2MeshJob::setInput(const Photo2MeshInput& input)
{
	m_input = input;
}

QString Photo2MeshJob::name()
{
	return "Photo2MeshJob";
}

QString Photo2MeshJob::description()
{
	return "Photo2MeshJob";
}

void Photo2MeshJob::work(qtuser_core::Progressor* progressor)
{
	m_mesh = createMeshFromInput(progressor);
}

void Photo2MeshJob::failed()
{
}

void Photo2MeshJob::successed(qtuser_core::Progressor* progressor)
{
	QString shortName = m_input.fileName;
	QStringList stringList = shortName.split("/");
	if (stringList.size() > 0)
		shortName = stringList.back();

	cxkernel::ModelCreateInput input;
	input.fileName = m_input.fileName;
	input.mesh = m_mesh;
	input.name = shortName;
	input.type = cxkernel::ModelNDataType::mdt_algrithm;

	cxkernel::addModelFromCreateInput(input);
}

TriMeshPtr Photo2MeshJob::createMeshFromInput(qtuser_core::Progressor* progressor)
{
	enchase::Enchaser enchaser;
	enchase::Mapper mapper;
	qtuser_core::ProgressorTracer tracer(progressor);

	tracer.progress(0.0f);

	TextureSurface surface(m_input.fileName);
	surface.useBlur = m_input.blur;
	surface.convertType = m_input.convertType;
	surface.baseHeight = m_input.baseHeight;
	surface.maxHeight = m_input.maxHeight;
	surface.invert = m_input.invert;
	surface.edgeType = enchase::EdgeType(m_input.edgeType);
	surface.useIndex = m_input.index;
	surface.colorSegment = m_input.colorSeg;

	enchase::MatrixF* matrix = surface.matrix(nullptr);
	tracer.progress(0.9f);

	if (matrix && matrix->width() > 0 && matrix->height() > 0)
	{
		int w = matrix->width();
		int h = matrix->height();

		float pixel2space = m_input.meshXWidth / m_input.pointXNum;

		m_input.pointYNum = (int)((float)m_input.pointXNum * (float)h / (float)w);
		enchaser.setSource(enchase::defaultPlane(m_input.pointXNum, m_input.pointYNum, pixel2space, m_input.baseHeight));
		enchase::defaultCoord(m_input.pointXNum, m_input.pointYNum, mapper.allTextureGroup());

		tracer.progress(0.95f);

		enchase::MatrixFSource* source = new enchase::MatrixFSource(matrix);
		mapper.setSource(source);
		enchaser.enchaseCache(&mapper, surface.index());

		tracer.progress(1.0f);

		return TriMeshPtr(enchaser.takeCurrent());
	}

	tracer.progress(1.0f);
	tracer.failed("Photo2MeshJob::createMeshFromInput error.");
	return nullptr;
}