#include "texfaces.h"
#include "qtuser3d/geometry/trianglescreatehelper.h"

//#include <Qt3DRender/QBlendEquationArguments>
//#include <Qt3DRender/QDepthTest>
//#include <Qt3DRender/QNoDepthMask>
//#include <Qt3DRender/QRenderPass>
//#include <Qt3DRender/QTextureImage>
//#include <Qt3DRender/QTexture>
//#include <Qt3DRender/QMultiSampleAntiAliasing>
//#include <Qt3DRender/QDithering>
//#include <Qt3DRender/QLineWidth>
//#include <Qt3DRender/QPolygonOffset>

#include <QVector2D>

#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	TexFaces::TexFaces(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		XRenderPass* pass = new PureRenderPass(this);
		pass->addFilterKeyMask("view", 0);

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(pass);
		setEffect(effect);

		m_colorParameter = setParameter("color", QVector4D(0.27f, 0.27f, 0.27f, 1.0f));
	}
	
	TexFaces::~TexFaces()
	{
	}

	void TexFaces::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}

	void TexFaces::updateBox(const Box3D& box)
	{
		int vertexNum = 4;
		int triangleNum = 2;
		QVector<QVector3D> positions;
		QVector<QVector3D> normals;
		QVector<unsigned> indices;
		float minX = box.min.x();
		float maxX = box.max.x();
		float minY = box.min.y();
		float maxY = box.max.y();
		float minZ = box.min.z();
		//float maxZ = box.max.z();
//		minZ -= 0.001;
		positions.push_back(QVector3D(minX, minY, minZ));
		positions.push_back(QVector3D(maxX, minY, minZ));
		positions.push_back(QVector3D(minX, maxY, minZ));
		positions.push_back(QVector3D(maxX, maxY, minZ));

		normals.push_back(QVector3D(0, 0, 1));
		normals.push_back(QVector3D(0, 0, 1));
		normals.push_back(QVector3D(0, 0, 1));
		normals.push_back(QVector3D(0, 0, 1));

		std::vector<float> texcoords = { 0, 1, 1, 1, 0, 0, 1, 0 };
		//down
		indices.push_back(0); indices.push_back(1); indices.push_back(3);
		indices.push_back(0); indices.push_back(3); indices.push_back(2);
		Qt3DRender::QGeometry* geometry = qtuser_3d::TrianglesCreateHelper::createWithTex(vertexNum, (float*)&positions.at(0), (float*)&normals.at(0), (float*)&texcoords.at(0), triangleNum, (unsigned*)&indices.at(0));
		setGeometry(geometry);
	}
}