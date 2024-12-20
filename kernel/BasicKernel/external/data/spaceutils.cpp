#include "spaceutils.h"

namespace creative_kernel
{
	trimesh::dbox3 groupsGlobalBoundingBox(const QList<ModelGroup*>& groups)
	{
		trimesh::dbox3 box;
		for (ModelGroup* group : groups)
		{
			box += group->globalBoundingBox();
		}
		return box;
	}

	trimesh::dbox3 modelsBox(const QList<ModelN*>& models)
	{
		trimesh::dbox3 box;
		for (ModelN* model : models)
			box += model->globalBoundingBox();
		return box;
	}

	qtuser_3d::Box3D convert(const trimesh::dbox3& box)
	{
		qtuser_3d::Box3D b;
		b += QVector3D(box.max.x, box.max.y, box.max.z);
		b += QVector3D(box.min.x, box.min.y, box.min.z);
		return b;
	}

	QList<ModelGroup*> findRelativeGroups(const QList<ModelN*>& models)
	{
		QList<ModelGroup*> groups;
		for (ModelN* model : models)
		{
			if (!groups.contains(model->getModelGroup()))
				groups.append(model->getModelGroup());
		}
		return groups;
	}

	bool fuzzyCompareDvec3(const trimesh::dvec3& v1, const trimesh::dvec3& v2)
	{
		return qFuzzyCompare(v1.x, v2.x) && qFuzzyCompare(v1.y, v2.y) && qFuzzyCompare(v1.z, v2.z);
	}

	void changeMeshFaceNormal(trimesh::TriMesh* oriMesh, trimesh::TriMesh* outMesh, const trimesh::fxform& normalMatrix)
	{
		if (!oriMesh || !outMesh)
			return;

		if (oriMesh->faces.size() != outMesh->faces.size())
			return;

		size_t faceSize = oriMesh->faces.size();
		for (int n = 0; n < faceSize; ++n)
		{
			//(1) caculate the original mesh normal
			trimesh::TriMesh::Face f = oriMesh->faces.at(n);
			trimesh::vec3 v12 = oriMesh->vertices.at(f[1]) - oriMesh->vertices.at(f[0]);
			trimesh::vec3 v13 = oriMesh->vertices.at(f[2]) - oriMesh->vertices.at(f[0]);

			//(2) transform the original normal with the  (normal matrix)
			trimesh::normalize(v12);
			trimesh::normalize(v13);
			trimesh::vec3 d = normalMatrix * (v12 TRICROSS v13);
			trimesh::normalize(d);

			//(3) caculate the global mesh normal
			trimesh::vec3 nv12 = outMesh->vertices.at(f[1]) - outMesh->vertices.at(f[0]);
			trimesh::vec3 nv13 = outMesh->vertices.at(f[2]) - outMesh->vertices.at(f[0]);
			trimesh::vec3 nd = nv12 TRICROSS nv13;
			trimesh::normalize(nd);

			//(4) compare the angle between (original normal) and the (global mesh normal)
			if (trimesh::dot(nd, d) < 0.0f)
			{
				outMesh->faces.at(n)[1] = oriMesh->faces.at(n)[2];
				outMesh->faces.at(n)[2] = oriMesh->faces.at(n)[1];
			}

		}
	}

}