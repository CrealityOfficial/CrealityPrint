#include "basicshapecreatehelper.h"
#include "qtuser3d/geometry/bufferhelper.h"
#include <math.h>
#include <QVector3D>
#include <qtuser3d/module/glcompatibility.h>


namespace trimesh
{
	class TriMesh;
}

namespace qtuser_3d
{
	BasicShapeCreateHelper::BasicShapeCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}
	
	BasicShapeCreateHelper::~BasicShapeCreateHelper()
	{
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createGeometry(Qt3DCore::QNode* parent, std::vector<float>* vertexDatas, std::vector<float>* normalDatas, QVector<unsigned>* indices)
	{
		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&(*vertexDatas)[0], AttribueSlot::Position, vertexDatas->size() / 3);
		Qt3DRender::QAttribute* normalAttribute = nullptr;
		if (normalDatas != nullptr)
		{
			normalAttribute = BufferHelper::CreateVertexAttribute((const char*)&(*normalDatas)[0], AttribueSlot::Normal, normalDatas->size() / 3);
		}
		Qt3DRender::QAttribute* indicesAttribute = nullptr;
		if (indices != nullptr)
		{
			indicesAttribute = BufferHelper::CreateIndexAttribute((const char*)&(*indices)[0], indices->size() / 3);
		}
		return GeometryCreateHelper::create(parent, positionAttribute, normalAttribute, indicesAttribute, nullptr);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createGeometryEx(Qt3DCore::QNode* parent, std::vector<float>* vertexDatas, std::vector<float>* normalDatas, QVector<unsigned>* indices)
	{
		uint vertexCount = vertexDatas->size() / 3;
		if (qtuser_3d::isGles())
		{
			vertexCount = vertexDatas->size() / 4;
		}
		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttributeEx((const char*)&(*vertexDatas)[0], AttribueSlot::Position, vertexCount);
		Qt3DRender::QAttribute* normalAttribute = nullptr;
		if (normalDatas != nullptr)
		{
			normalAttribute = BufferHelper::CreateVertexAttribute((const char*)&(*normalDatas)[0], AttribueSlot::Normal, normalDatas->size() / 3);
		}
		Qt3DRender::QAttribute* indicesAttribute = nullptr;
		if (indices != nullptr)
		{
			indicesAttribute = BufferHelper::CreateIndexAttribute((const char*)&(*indices)[0], indices->size() / 3);
		}
		return GeometryCreateHelper::create(parent, positionAttribute, normalAttribute, indicesAttribute, nullptr);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createCylinder(Qt3DCore::QNode* parent)
	{
		std::vector<float> datas;
		createCylinderData(0.3, 6, 36, datas);
		return createGeometry(parent, &datas);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createRectangle(float w, float h, Qt3DCore::QNode* parent)
	{
		std::vector<float> datas = { 
			0, 0, 0, 
			w, 0, 0, 
			w, h, 0, 
			0, h, 0 };

		QVector<unsigned> indices = {
			0, 1, 2,
			0, 2, 3 };

		return createGeometry(parent, &datas, nullptr, &indices);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createPen(Qt3DCore::QNode* parent)
	{
		std::vector<float> vertexDatas, normalDatas;
		createPenData(1.8, 3, 14, 36, vertexDatas, normalDatas);
		return createGeometry(parent, &vertexDatas, &normalDatas);

	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createBall(QVector3D center, float r, float angleSpan, Qt3DCore::QNode* parent)
	{
		std::vector<float> vertexDatas, normalDatas;
		createBallData(center, r, angleSpan, vertexDatas, normalDatas);
		return createGeometry(parent, &vertexDatas, &normalDatas);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createInstructionsDLP(float cylinderR, float cylinderLen, float axisR, float axisLen, Qt3DCore::QNode* parent)
	{
		std::vector<float> vertexDatas, normalDatas;
		//createInstructionsData(cylinderR, cylinderLen, axisR, axisLen, 18, vertexDatas, normalDatas);
		createInstructionsDataDLP(cylinderR, cylinderLen, axisR, axisLen, 36, vertexDatas, normalDatas);
		return createGeometryEx(parent, &vertexDatas, &normalDatas);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createScaleIndicatorDLP(float cylinderR, float cylinderLen, int seg, float squarelen, Qt3DCore::QNode* parent)
	{
		std::vector<float> vertexDatas, normalDatas;
		createScaleIndicatorDataDLP(cylinderR, cylinderLen, seg, squarelen, vertexDatas, normalDatas);
		return createGeometry(parent, &vertexDatas, &normalDatas);
	}
	
	Qt3DRender::QGeometry* BasicShapeCreateHelper::createInstructions(float cylinderR, float cylinderLen, float axisR, float axisLen, Qt3DCore::QNode* parent)
	{
		std::vector<float> vertexDatas;
		std::vector<float> normals;
		createInstructionsData(cylinderR, cylinderLen, axisR, axisLen, 18, vertexDatas, normals);
		return createGeometry(parent, &vertexDatas, &normals);
	}

	Qt3DRender::QGeometry* BasicShapeCreateHelper::createScaleIndicator(float cylinderR, float cylinderLen, int seg, float squarelen, Qt3DCore::QNode* parent)
	{
		std::vector<float> vertexDatas;
		std::vector<float> normals;
		createScaleIndicatorData(cylinderR, cylinderLen, seg, squarelen, vertexDatas, normals);
		return createGeometry(parent, &vertexDatas, &normals);
	}

	int BasicShapeCreateHelper::createCylinderData(float r, float h, int seg, std::vector<float> &datas)
	{
		const float PI = 3.1415926535897932384;
		float hoffset = h/2;

		float angdesSpan = 360.0 / seg;
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			datas.push_back(0);
			datas.push_back(h / 2 + hoffset);
			datas.push_back(0);

			datas.push_back(-r * sin(angrad));
			datas.push_back(h / 2 + hoffset);
			datas.push_back(-r * cos(angrad));

			datas.push_back(-r * sin(angradNext));
			datas.push_back(h / 2 + hoffset);
			datas.push_back(-r * cos(angradNext));
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			datas.push_back(0);
			datas.push_back(-h / 2 + hoffset);
			datas.push_back(0);

			datas.push_back(-r * sin(angradNext));
			datas.push_back(-h / 2 + hoffset);
			datas.push_back(-r * cos(angradNext));

			datas.push_back(-r * sin(angrad));
			datas.push_back(-h / 2 + hoffset);
			datas.push_back(-r * cos(angrad));
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			datas.push_back(-r * sin(angrad));
			datas.push_back(-h / 2 + hoffset);
			datas.push_back(-r * cos(angrad));

			datas.push_back(-r * sin(angradNext));
			datas.push_back(h / 2 + hoffset);
			datas.push_back(-r * cos(angradNext));

			datas.push_back(-r * sin(angrad));
			datas.push_back(h / 2 + hoffset);
			datas.push_back(-r * cos(angrad));

			datas.push_back(-r * sin(angrad));
			datas.push_back(-h / 2 + hoffset);
			datas.push_back(-r * cos(angrad));

			datas.push_back(-r * sin(angradNext));
			datas.push_back(-h / 2 + hoffset);
			datas.push_back(-r * cos(angradNext));

			datas.push_back(-r * sin(angradNext));
			datas.push_back(h / 2 + hoffset);
			datas.push_back(-r * cos(angradNext));
		}

		return 0;
	}

	int BasicShapeCreateHelper::createVerticalCylinderData(float r, float h, int seg, float offset_per, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		const float PI = 3.1415926535897932384;
		float hoffset = h * offset_per;

		float angdesSpan = 360.0 / seg;
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, h / 2 + hoffset);
			QVector3D v2(-r * sin(angrad), -r * cos(angrad), h / 2 + hoffset);
			QVector3D v3(-r * sin(angradNext), -r * cos(angradNext), h / 2 + hoffset);

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, -h / 2 + hoffset);
			QVector3D v2(-r * sin(angradNext), -r * cos(angradNext), -h / 2 + hoffset);
			QVector3D v3(-r * sin(angrad), -r * cos(angrad), -h / 2 + hoffset);

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(-r * sin(angrad), -r * cos(angrad), -h / 2 + hoffset);
			QVector3D v2(-r * sin(angradNext), -r * cos(angradNext), h / 2 + hoffset);
			QVector3D v3(-r * sin(angrad), -r * cos(angrad), h / 2 + hoffset);

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);

			QVector3D v4(-r * sin(angrad), -r * cos(angrad), -h / 2 + hoffset);
			QVector3D v5(-r * sin(angradNext), -r * cos(angradNext), -h / 2 + hoffset);
			QVector3D v6(-r * sin(angradNext), -r * cos(angradNext), h / 2 + hoffset);

			QVector3D n4 = QVector3D::normal(v4, v5, v6);
			addFaceDataWithQVector3D(v4, v5, v6, n4, vertexDatas, normalDatas);
		}

		return 0;
	}

	int BasicShapeCreateHelper::createPenData(float r, float headh, float bodyh, int seg, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		const float PI = 3.1415926535897932384;
		float angdesSpan = 360.0 / seg;

		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(-r * sin(angrad), -r * cos(angrad), headh);
			QVector3D v2(-r * sin(angradNext), -r * cos(angradNext), bodyh);
			QVector3D v3(-r * sin(angrad), -r * cos(angrad), bodyh);

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);

			QVector3D v4(-r * sin(angrad), -r * cos(angrad), headh);
			QVector3D v5(-r * sin(angradNext), -r * cos(angradNext), headh);
			QVector3D v6(-r * sin(angradNext), -r * cos(angradNext), bodyh);

			QVector3D n2 = QVector3D::normal(v4, v5, v6);
			addFaceDataWithQVector3D(v4, v5, v6, n2, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, bodyh);
			QVector3D v2(-r * sin(angrad), -r * cos(angrad), bodyh);
			QVector3D v3(-r * sin(angradNext), -r * cos(angradNext), bodyh);

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, headh * 0.05);
			QVector3D v2(-r * sin(angrad), -r * cos(angrad), headh);
			QVector3D v3(-r * sin(angradNext), -r * cos(angradNext), headh);

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
		}

		return 0;
	}

	int BasicShapeCreateHelper::createBallData(QVector3D center, float r, float angleSpan, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		const float PI = 3.1415926535897932384;
		for (float vAngle = -90; vAngle < 90; vAngle = vAngle + angleSpan)
		{
			float usevAngle = vAngle * PI / 180.0;
			float nvAngle = (vAngle + angleSpan) * PI / 180.0;

			for (float hAngle = 0; hAngle < 360; hAngle = hAngle + angleSpan)
			{
				float usehAngle = hAngle * PI / 180.0;
				float nhAngle = (hAngle + angleSpan) * PI / 180.0;

//				QVector3D v0(cos(usevAngle) * cos(usehAngle), cos(usevAngle) * sin(usehAngle), sin(usevAngle));

				float nx0 = (float)(cos(usevAngle) * cos(usehAngle));
				float ny0 = (float)(cos(usevAngle) * sin(usehAngle));
				float nz0 = (float)(sin(usevAngle));

				float nx1 = (float)(cos(usevAngle) * cos(nhAngle));
				float ny1 = (float)(cos(usevAngle) * sin(nhAngle));
				float nz1 = (float)(sin(usevAngle));

				float nx2 = (float)(cos(nvAngle) * cos(nhAngle));
				float ny2 = (float)(cos(nvAngle) * sin(nhAngle));
				float nz2 = (float)(r * sin(nvAngle));

				float nx3 = (float)(cos(nvAngle) * cos(usehAngle));
				float ny3 = (float)(cos(nvAngle) * sin(usehAngle));
				float nz3 = (float)(sin(nvAngle));

				float x0 = (float)(r * cos(usevAngle) * cos(usehAngle)) + center.x();
				float y0 = (float)(r * cos(usevAngle) * sin(usehAngle)) + center.y();
				float z0 = (float)(r * sin(usevAngle)) + center.z();

				float x1 = (float)(r * cos(usevAngle) * cos(nhAngle)) + center.x();
				float y1 = (float)(r * cos(usevAngle) * sin(nhAngle)) + center.y();
				float z1 = (float)(r * sin(usevAngle)) + center.z();

				float x2 = (float)(r * cos(nvAngle) * cos(nhAngle)) + center.x();
				float y2 = (float)(r * cos(nvAngle) * sin(nhAngle)) + center.y();
				float z2 = (float)(r * sin(nvAngle)) + center.z();

				float x3 = (float)(r * cos(nvAngle) * cos(usehAngle)) + center.x();
				float y3 = (float)(r * cos(nvAngle) * sin(usehAngle)) + center.y();
				float z3 = (float)(r * sin(nvAngle)) + center.z();

				vertexDatas.push_back(x1); vertexDatas.push_back(y1); vertexDatas.push_back(z1);
				vertexDatas.push_back(x3); vertexDatas.push_back(y3); vertexDatas.push_back(z3);
				vertexDatas.push_back(x0); vertexDatas.push_back(y0); vertexDatas.push_back(z0);

				vertexDatas.push_back(x1); vertexDatas.push_back(y1); vertexDatas.push_back(z1);
				vertexDatas.push_back(x2); vertexDatas.push_back(y2); vertexDatas.push_back(z2);
				vertexDatas.push_back(x3); vertexDatas.push_back(y3); vertexDatas.push_back(z3);


				normalDatas.push_back(nx1); normalDatas.push_back(ny1); normalDatas.push_back(nz1);
				normalDatas.push_back(nx3); normalDatas.push_back(ny3); normalDatas.push_back(nz3);
				normalDatas.push_back(nx0); normalDatas.push_back(ny0); normalDatas.push_back(nz0);

				normalDatas.push_back(nx1); normalDatas.push_back(ny1); normalDatas.push_back(nz1);
				normalDatas.push_back(nx2); normalDatas.push_back(ny2); normalDatas.push_back(nz2);
				normalDatas.push_back(nx3); normalDatas.push_back(ny3); normalDatas.push_back(nz3);

			}
		}
		return 0;
	}

	int BasicShapeCreateHelper::createInstructionsDataDLP(float cylinderR, float cylinderLen, float axisR, float axisLen, int seg, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		const float PI = 3.1415926535897932384;
		float angdesSpan = 360.0 / seg;

		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, 0);
			QVector3D v2(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v3(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3DEx(v1, v2, v3, n, vertexDatas, normalDatas);
			
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v2(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));
			QVector3D v3(-cylinderR * sin(angrad), cylinderLen, -cylinderR * cos(angrad));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3DEx(v1, v2, v3, n, vertexDatas, normalDatas);

			QVector3D v4(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v5(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));
			QVector3D v6(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));

			QVector3D n2 = QVector3D::normal(v4, v5, v6);
			addFaceDataWithQVector3DEx(v4, v5, v6, n2, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, cylinderLen, 0);
			QVector3D v2(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));
			QVector3D v3(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3DEx(v1, v2, v3, n, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, cylinderLen + axisLen, 0);
			QVector3D v2(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));
			QVector3D v3(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3DEx(v1, v2, v3, n, vertexDatas, normalDatas);
		}

		return 0;
	}


	int BasicShapeCreateHelper::createScaleIndicatorDataDLP(float cylinderR, float cylinderLen, int seg, float squarelen, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		const float PI = 3.1415926535897932384;
		float angdesSpan = 360.0 / seg;

		float halflen = squarelen / 2.0;
		float extraLen = 0.05;

        float axisR = 0.08;
        float axisLen = 0.3;

        float disLen = cylinderLen + axisLen + 0.05;
		QVector3D vs[8] = {
            QVector3D(-halflen, disLen, -halflen),
            QVector3D(-halflen, disLen, halflen),
            QVector3D(halflen, disLen, halflen),
            QVector3D(halflen, disLen, -halflen),

            QVector3D(-halflen, disLen + squarelen + extraLen, -halflen),
            QVector3D(-halflen, disLen + squarelen + extraLen, halflen),
            QVector3D(halflen, disLen + squarelen + extraLen, halflen),
            QVector3D(halflen, disLen + squarelen + extraLen, -halflen),
		};

		auto f = [&vs, &vertexDatas, &normalDatas](int p1, int p2, int p3, int p4)
		{
			QVector3D v1 = vs[p1];
			QVector3D v2 = vs[p2];
			QVector3D v3 = vs[p3];
			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);

			v1 = vs[p1];
			v2 = vs[p3];
			v3 = vs[p4];
			n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
		};

		f(0, 3, 2, 1);
		f(3, 7, 6, 2);
		f(0, 1, 5, 4);
		f(0, 4, 7, 3);
		f(2, 6, 5, 1);
		f(6, 7, 4, 5);

		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, 0);
			QVector3D v2(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v3(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v2(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));
			QVector3D v3(-cylinderR * sin(angrad), cylinderLen, -cylinderR * cos(angrad));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);

			QVector3D v4(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v5(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));
			QVector3D v6(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));

			QVector3D n2 = QVector3D::normal(v4, v5, v6);
			addFaceDataWithQVector3D(v4, v5, v6, n2, vertexDatas, normalDatas);
		}
        for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
        {
            float angrad = angdeg * PI / 180.0;
            float angradNext = (angdeg + angdesSpan) * PI / 180.0;

            QVector3D v1(0, cylinderLen, 0);
            QVector3D v2(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));
            QVector3D v3(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));

            QVector3D n = QVector3D::normal(v1, v2, v3);
            addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
        }
        for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
        {
            float angrad = angdeg * PI / 180.0;
            float angradNext = (angdeg + angdesSpan) * PI / 180.0;

            QVector3D v1(0, cylinderLen + axisLen, 0);
            QVector3D v2(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));
            QVector3D v3(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));

            QVector3D n = QVector3D::normal(v1, v2, v3);
            addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normalDatas);
        }

		return 0;
	}
	
	int BasicShapeCreateHelper::createInstructionsData(float cylinderR, float cylinderLen, float axisR, float axisLen, int seg, std::vector<float>& vertexDatas, std::vector<float>& normals)
	{
		const float PI = 3.1415926535897932384;
		float angdesSpan = 360.0 / seg;

		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, 0);
			QVector3D v2(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v3(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v2(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));
			QVector3D v3(-cylinderR * sin(angrad), cylinderLen, -cylinderR * cos(angrad));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);

			QVector3D v4(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v5(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));
			QVector3D v6(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));

			QVector3D n2 = QVector3D::normal(v4, v5, v6);
			addFaceDataWithQVector3D(v4, v5, v6, n2, vertexDatas, normals);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, cylinderLen, 0);
			QVector3D v2(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));
			QVector3D v3(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, cylinderLen + axisLen, 0);
			QVector3D v2(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));
			QVector3D v3(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);
		}

		return 0;
	}

	int BasicShapeCreateHelper::createScaleIndicatorData(float cylinderR, float cylinderLen, int seg, float squarelen, std::vector<float>& vertexDatas, std::vector<float>& normals)
	{
		const float PI = 3.1415926535897932384;
		float angdesSpan = 360.0 / seg;

		float halflen = squarelen / 2.0;
		float extraLen = 0.00;

        float axisR = 0.08;
        float axisLen = 0.25;
		axisLen = 0.0;

		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(0, 0, 0);
			QVector3D v2(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v3(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);
		}
		for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
		{
			float angrad = angdeg * PI / 180.0;
			float angradNext = (angdeg + angdesSpan) * PI / 180.0;

			QVector3D v1(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v2(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));
			QVector3D v3(-cylinderR * sin(angrad), cylinderLen, -cylinderR * cos(angrad));

			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);

			QVector3D v4(-cylinderR * sin(angrad), 0, -cylinderR * cos(angrad));
			QVector3D v5(-cylinderR * sin(angradNext), 0, -cylinderR * cos(angradNext));
			QVector3D v6(-cylinderR * sin(angradNext), cylinderLen, -cylinderR * cos(angradNext));

			QVector3D n2 = QVector3D::normal(v4, v5, v6);
			addFaceDataWithQVector3D(v4, v5, v6, n2, vertexDatas, normals);
		}
        //for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
        //{
        //    float angrad = angdeg * PI / 180.0;
        //    float angradNext = (angdeg + angdesSpan) * PI / 180.0;

        //    QVector3D v1(0, cylinderLen, 0);
        //    QVector3D v2(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));
        //    QVector3D v3(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));

        //    QVector3D n = QVector3D::normal(v1, v2, v3);
        //    addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas);
        //}
        //for (float angdeg = 0; ceil(angdeg) < 360; angdeg += angdesSpan)
        //{
        //    float angrad = angdeg * PI / 180.0;
        //    float angradNext = (angdeg + angdesSpan) * PI / 180.0;

        //    QVector3D v1(0, cylinderLen + axisLen, 0);
        //    QVector3D v2(-axisR * sin(angradNext), cylinderLen, -axisR * cos(angradNext));
        //    QVector3D v3(-axisR * sin(angrad), cylinderLen, -axisR * cos(angrad));

        //    QVector3D n = QVector3D::normal(v1, v2, v3);
        //    addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas);
        //}

		float disLen = cylinderLen + axisLen + 0.0;
		QVector3D vs[8] = {
			QVector3D(-halflen, disLen, -halflen),
			QVector3D(-halflen, disLen, halflen),
			QVector3D(halflen, disLen, halflen),
			QVector3D(halflen, disLen, -halflen),

			QVector3D(-halflen, disLen + squarelen + extraLen, -halflen),
			QVector3D(-halflen, disLen + squarelen + extraLen, halflen),
			QVector3D(halflen, disLen + squarelen + extraLen, halflen),
			QVector3D(halflen, disLen + squarelen + extraLen, -halflen),
		};

		auto f = [&vs, &vertexDatas, &normals](int p1, int p2, int p3, int p4)
		{
			QVector3D v1 = vs[p1];
			QVector3D v2 = vs[p2];
			QVector3D v3 = vs[p3];
			QVector3D n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);

			v1 = vs[p1];
			v2 = vs[p3];
			v3 = vs[p4];
			n = QVector3D::normal(v1, v2, v3);
			addFaceDataWithQVector3D(v1, v2, v3, n, vertexDatas, normals);
		};

		f(0, 3, 2, 1);
		f(3, 7, 6, 2);
		f(0, 1, 5, 4);
		f(0, 4, 7, 3);
		f(2, 6, 5, 1);
		f(6, 7, 4, 5);

		return 0;
	}

	int BasicShapeCreateHelper::addFaceDataWithQVector3D(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, const QVector3D& n, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		vertexDatas.push_back(v1.x());
		vertexDatas.push_back(v1.y());
		vertexDatas.push_back(v1.z());

		vertexDatas.push_back(v2.x());
		vertexDatas.push_back(v2.y());
		vertexDatas.push_back(v2.z());

		vertexDatas.push_back(v3.x());
		vertexDatas.push_back(v3.y());
		vertexDatas.push_back(v3.z());

		for (int i = 0; i < 3; i++)
		{
			normalDatas.push_back(n.x());
			normalDatas.push_back(n.y());
			normalDatas.push_back(n.z());
		}

		return 0;
	}

	int BasicShapeCreateHelper::addFaceDataWithQVector3D(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, const QVector3D& n, std::vector<float>& vertexDatas)
	{
		vertexDatas.push_back(v1.x());
		vertexDatas.push_back(v1.y());
		vertexDatas.push_back(v1.z());

		vertexDatas.push_back(v2.x());
		vertexDatas.push_back(v2.y());
		vertexDatas.push_back(v2.z());

		vertexDatas.push_back(v3.x());
		vertexDatas.push_back(v3.y());
		vertexDatas.push_back(v3.z());

		return 0;
	}

	int BasicShapeCreateHelper::addFaceDataWithQVector3DEx(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3, const QVector3D& n, std::vector<float>& vertexDatas, std::vector<float>& normalDatas)
	{
		int vertexIndex = vertexDatas.size() / 3;

		vertexDatas.push_back(v1.x());
		vertexDatas.push_back(v1.y());
		vertexDatas.push_back(v1.z());

		if (qtuser_3d::isGles())
		{
			vertexDatas.push_back(float(vertexIndex));
		}

		vertexDatas.push_back(v2.x());
		vertexDatas.push_back(v2.y());
		vertexDatas.push_back(v2.z());

		if (qtuser_3d::isGles())
		{
			vertexDatas.push_back(float(vertexIndex + 1));
		}

		vertexDatas.push_back(v3.x());
		vertexDatas.push_back(v3.y());
		vertexDatas.push_back(v3.z());

		if (qtuser_3d::isGles())
		{
			vertexDatas.push_back(float(vertexIndex + 2));
		}

		for (int i = 0; i < 3; i++)
		{
			normalDatas.push_back(n.x());
			normalDatas.push_back(n.y());
			normalDatas.push_back(n.z());
		}

		return 0;
	}

}
