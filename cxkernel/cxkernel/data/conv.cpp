#include "conv.h"

namespace cxkernel
{
	QVector3D vec2qvector(const trimesh::vec3& vertex)
	{
		return QVector3D(vertex.x, vertex.y, vertex.z);
	}

	QVector<QVector3D> vecs2qvectors(const std::vector<trimesh::vec3>& vertices)
	{
		QVector<QVector3D> results;
		for (const trimesh::vec3& vertex : vertices)
			results.append(vec2qvector(vertex));
		return results;
	}

	QMatrix4x4 xform2QMatrix(const trimesh::fxform& xf)
	{
		QMatrix4x4 m;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m(i, j) = xf(i, j);
			}
		}
		return m;
	}

	trimesh::fxform qMatrix2Xform(const QMatrix4x4& matrix)
	{
		trimesh::fxform xf;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				xf(i, j) = matrix(i, j);
			}
		}
		return xf;
	}

	trimesh::vec3 qVector3D2Vec3(const QVector3D& vec)
	{
		return trimesh::vec3(vec.x(), vec.y(), vec.z());
	}

	trimesh::quaternion qQuaternion2tQuaternion(const QQuaternion& q)
	{
		trimesh::quaternion qq(q.scalar(), q.x(), q.y(), q.z());
		return qq;
	}

	QQuaternion tqua2qqua(trimesh::quaternion q)
	{
		return QQuaternion(q.wp, q.xp, q.yp, q.zp);
	}
}