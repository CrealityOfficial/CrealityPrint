#include "shapefiller.h"
#include <QtGui/QVector3D>
#include <QtGui/QQuaternion>
#include <QtCore/qmath.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QStandardPaths>

#include "qtusercore/string/resourcesfinder.h"

namespace creative_kernel
{
	ShapeFiller::ShapeFiller(QObject* parent)
		:QObject(parent)
	{
	}
	
	ShapeFiller::~ShapeFiller()
	{
	}

	int ShapeFiller::fillSphere(trimesh::vec3* data, float radius, trimesh::vec3 center)
	{
		int hPart = 20;
		int vPart = 10;

		std::vector<trimesh::vec3> points(hPart * vPart + 2);
		float hDelta = M_PIf * 2.0f / (float)(hPart);
		float vDelta = M_PIf / (float)(vPart + 1);
		points.at(0) = center + trimesh::vec3(0.0f, 0.0f, radius);
		for (int v = 0; v < vPart; ++v)
		{
			float theta1 = (float)(v + 1) * vDelta;
			float z = cosf(theta1);
			float r = sinf(theta1);
			for (int h = 0; h < hPart; ++h)
			{
				trimesh::vec3& p = points.at(v * hPart + h + 1);
				float theta2 = (float)(h)*hDelta;
				p.z = z;
				p.x = r * cosf(theta2);
				p.y = r * sinf(theta2);

				p = center + radius * p;
			}
		}
		points.back() = center - trimesh::vec3(0.0f, 0.0f, radius);

		int faceNum = hPart * (vPart - 1) * 2 + 2 * hPart;

		auto fvindex = [&hPart](int layer, int index)->int {
			return layer * hPart + index + 1;
		};
		for (int i = 0; i < hPart; ++i)
		{
			*data++ = points.at(0);
			*data++ = points.at(fvindex(0, i));
			*data++ = points.at(fvindex(0, (i + 1) % hPart));
		}
		for (int i = 0; i < vPart - 1; ++i)
		{
			for (int j = 0; j < hPart; ++j)
			{
				int v1 = fvindex(i, j);
				int v2 = fvindex(i, (j + 1) % hPart);
				int v3 = fvindex(i + 1, j);
				int v4 = fvindex(i + 1, (j + 1) % hPart);
				*data++ = points.at(v1);
				*data++ = points.at(v3);
				*data++ = points.at(v2);
				*data++ = points.at(v2);
				*data++ = points.at(v3);
				*data++ = points.at(v4);
			}
		}
		for (int i = 0; i < hPart; ++i)
		{
			*data++ = points.at(fvindex(vPart - 1, (i + 1) % hPart));
			*data++ = points.at(fvindex(vPart - 1, i));
			*data++ = points.back();
		}
		return 3 * faceNum;
	}

	int ShapeFiller::fillLink(trimesh::vec3* data, float radius1, trimesh::vec3& center1, float radius2, trimesh::vec3& center2)
	{
		int hPart = 20;

		QVector3D start = QVector3D(center1.x, center1.y, center1.z);
		QVector3D end = QVector3D(center2.x, center2.y, center2.z);

		QVector3D dir = end - start;
		dir.normalize();
		QQuaternion q = QQuaternion::fromDirection(dir, QVector3D(0.0f, 0.0f, 1.0f));

		float deltaTheta = M_PIf * 2.0f / (float)(hPart);
		std::vector<float> cosValue;
		std::vector<float> sinValue;
		for (int i = 0; i < hPart; ++i)
		{
			cosValue.push_back(qCos(deltaTheta * (float)i));
			sinValue.push_back(qSin(deltaTheta * (float)i));
		}

		std::vector<QVector3D> baseNormals;
		for (int i = 0; i < hPart; ++i)
		{
			baseNormals.push_back(QVector3D(cosValue[i], sinValue[i], 0.0f));
		}

		int vertexNum = 2 * hPart;
		std::vector<trimesh::vec3> points(vertexNum);
		int faceNum = 4 * hPart - 4;

		int vertexIndex = 0;
		for (int i = 0; i < hPart; ++i)
		{
			QVector3D n = q * baseNormals[i];
			QVector3D s = start + n * radius1;
			points.at(vertexIndex) = trimesh::vec3(s.x(), s.y(), s.z());
			++vertexIndex;
			QVector3D e = end + n * radius2;
			points.at(vertexIndex) = trimesh::vec3(e.x(), e.y(), e.z());
			++vertexIndex;
		}

		auto fvindex = [&hPart](int layer, int index)->int {
			return layer + 2 * index;
		};

		for (int i = 0; i < hPart; ++i)
		{
			int v1 = fvindex(0, i);
			int v2 = fvindex(1, i);
			int v3 = fvindex(0, (i + 1) % hPart);
			int v4 = fvindex(1, (i + 1) % hPart);
			*data++ = points.at(v1);
			*data++ = points.at(v3);
			*data++ = points.at(v2);
			*data++ = points.at(v2);
			*data++ = points.at(v3);
			*data++ = points.at(v4);
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(0);
			*data++ = points.at(fvindex(0, i + 1));
			*data++ = points.at(fvindex(0, i));
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(1);
			*data++ = points.at(fvindex(1, i));
			*data++ = points.at(fvindex(1, i + 1));
		}

		return 3 * faceNum;
	}

	int ShapeFiller::fillCylinder(trimesh::vec3* data, float radius1, trimesh::vec3& center1, float radius2, trimesh::vec3& center2
		, int num, float theta)
	{
		int hPart = num;

		QVector3D start = QVector3D(center1.x, center1.y, center1.z);
		QVector3D end = QVector3D(center2.x, center2.y, center2.z);

		QVector3D dir = end - start;
		dir.normalize();
		QQuaternion q = QQuaternion::fromDirection(dir, QVector3D(0.0f, 0.0f, 1.0f));

		theta *= M_PIf / 180.0f;
		float deltaTheta = M_PIf * 2.0f / (float)(hPart);
		std::vector<float> cosValue;
		std::vector<float> sinValue;
		for (int i = 0; i < hPart; ++i)
		{
			cosValue.push_back(qCos(deltaTheta * (float)i + theta));
			sinValue.push_back(qSin(deltaTheta * (float)i + theta));
		}

		std::vector<QVector3D> baseNormals;
		for (int i = 0; i < hPart; ++i)
		{
			baseNormals.push_back(QVector3D(cosValue[i], sinValue[i], 0.0f));
		}

		int vertexNum = 2 * hPart;
		std::vector<trimesh::vec3> points(vertexNum);
		int faceNum = 4 * hPart - 4;

		int vertexIndex = 0;
		for (int i = 0; i < hPart; ++i)
		{
			QVector3D n = q * baseNormals[i];
			QVector3D s = start + n * radius1;
			points.at(vertexIndex) = trimesh::vec3(s.x(), s.y(), s.z());
			++vertexIndex;
			QVector3D e = end + n * radius2;
			points.at(vertexIndex) = trimesh::vec3(e.x(), e.y(), e.z());
			++vertexIndex;
		}

		auto fvindex = [&hPart](int layer, int index)->int {
			return layer + 2 * index;
		};

		for (int i = 0; i < hPart; ++i)
		{
			int v1 = fvindex(0, i);
			int v2 = fvindex(1, i);
			int v3 = fvindex(0, (i + 1) % hPart);
			int v4 = fvindex(1, (i + 1) % hPart);
			*data++ = points.at(v1);
			*data++ = points.at(v3);
			*data++ = points.at(v2);
			*data++ = points.at(v2);
			*data++ = points.at(v3);
			*data++ = points.at(v4);
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(0);
			*data++ = points.at(fvindex(0, i + 1));
			*data++ = points.at(fvindex(0, i));
		}

		for (int i = 1; i < hPart - 1; ++i)
		{
			*data++ = points.at(1);
			*data++ = points.at(fvindex(1, i));
			*data++ = points.at(fvindex(1, i + 1));
		}

		return 3 * faceNum;
	}
}