#include "modelndata.h"
#include "cxkernel/data/trimeshutils.h"

#include "qhullWrapper/hull/meshconvex.h"
#include "qhullWrapper/hull/hullface.h"

#include "msbase/mesh/checker.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/mesh/tinymodify.h"

#include "qtusercore/module/progressortracer.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QDataStream>

namespace cxkernel
{
	ModelNData::ModelNData()
	{

	}

	ModelNData::~ModelNData()
	{

	}

	int ModelNData::primitiveNum()
	{
		return mesh ? (int)mesh->faces.size() : 0;
	}

	void ModelNData::updateRenderData()
	{
		if (mesh && ((int)mesh->faces.size() != renderData.fcount))
			generateGeometryDataFromMesh(mesh.get(), renderData);
	}

	void ModelNData::updateRenderDataForced()
	{
		generateGeometryDataFromMesh(mesh.get(), renderData);
	}

	void ModelNData::updateIndexRenderData()
	{
		if (mesh && ((int)mesh->faces.size() != renderData.fcount))
			cxkernel::generateIndexGeometryDataFromMesh(mesh.get(), renderData);
	}

	trimesh::box3 ModelNData::calculateBox(const trimesh::fxform& matrix)
	{
		trimesh::box3 b;

		if (hull)
		{
			for (const trimesh::vec3& v : hull->vertices)
				b += matrix * v;
		}
		else if (mesh)
		{
			if (mesh->flags.size() == mesh->vertices.size())
			{
				for (int i : mesh->flags)
					b += matrix * mesh->vertices.at(i);
			}
			else
			{
				int size = (int)mesh->vertices.size();
				for (int i = 0; i < size; ++i)
				{
					trimesh::vec3 v = mesh->vertices.at(i);
					b += matrix * v;
				}
			}
		}

		return b;
	}

	trimesh::box3 ModelNData::localBox()
	{
		trimesh::box3 b;
		if (mesh)
			b = mesh->bbox;
		return b;
	}

	float ModelNData::localZ()
	{
		trimesh::box3 b = localBox();
		return b.min.z - offset.z;
	}

	void ModelNData::calculateFaces()
	{
		if (faces.size() == 0)
		{
			std::vector<qhullWrapper::HullFace> _faces;
			qhullWrapper::hullFacesFromConvexMesh(hull.get(), _faces);

			int size = _faces.size();
			if (size > 0)
			{
				faces.resize(size);
				for (int i = 0; i < size; ++i)
				{
					KernelHullFace& face = faces.at(i);
					const qhullWrapper::HullFace& _face = _faces.at(i);
					face.mesh = _face.mesh;
					face.normal = _face.normal;
					face.hullarea = _face.hullarea;
				}
			}
		}
	}

	void ModelNData::resetHull()
	{
		if (!hull)
		{
			hull.reset(qhullWrapper::convex_hull_3d(mesh.get()));
		}
	}

	void ModelNData::convex(const trimesh::fxform& matrix, std::vector<trimesh::vec3>& datas)
	{
		std::vector<trimesh::vec2> hullPoints2D;
		if (hull)
		{
			int size = (int)hull->vertices.size();
			hullPoints2D.reserve(size);
			for (const trimesh::vec3& v : hull->vertices)
			{
				trimesh::vec3 p = matrix * v;

				hullPoints2D.emplace_back(trimesh::vec2(p.x, p.y));
			}
		}
		else if (mesh->flags.size() > 0)
		{
			int size = (int)mesh->vertices.size();
			for (int i : mesh->flags)
			{
				if (i >= 0 && i < size)
				{
					trimesh::vec3 p = matrix * mesh->vertices.at(i);
					hullPoints2D.emplace_back(trimesh::vec2(p.x, p.y));
				}
			}
		}

		if (hullPoints2D.size() == 0)
			return;

		trimesh::TriMesh* m = qhullWrapper::convex2DPolygon((const float*)hullPoints2D.data(), hullPoints2D.size());
		if (!m)
			return;

		datas.clear();
		for (const trimesh::vec3& p : m->vertices)
			datas.emplace_back(trimesh::vec3(p.x, p.y, 0.0f));

		delete m;
	}

	bool ModelNData::traitTriangle(int faceID, std::vector<trimesh::vec3>& position, const trimesh::fxform& matrix, bool _offset)
	{
		if (!mesh || faceID < 0 || faceID >= (int)mesh->faces.size())
			return false;

		const trimesh::TriMesh::Face& face = mesh->faces.at(faceID);
		position.clear();
		position.push_back(matrix * mesh->vertices.at(face.x));
		position.push_back(matrix * mesh->vertices.at(face.y));
		position.push_back(matrix * mesh->vertices.at(face.z));

		if (_offset)
		{
			trimesh::vec3 n = trimesh::trinorm(position.at(0), position.at(1), position.at(2));
			trimesh::normalize(n);

			position.at(0) += 0.1f * n;
			position.at(1) += 0.1f * n;
			position.at(2) += 0.1f * n;
		}
		return true;
	}

	TriMeshPtr ModelNData::createGlobalMesh(const trimesh::fxform& matrix)
	{
		if (!mesh)
			return nullptr;

		trimesh::TriMesh* newMesh = new trimesh::TriMesh();
		*newMesh = *mesh;
		trimesh::apply_xform(newMesh, trimesh::xform(matrix));
		return TriMeshPtr(newMesh);
	}

	bool ModelNData::traitTriangleEx(int faceID, std::vector<trimesh::vec3>& position, trimesh::vec3& normal, const trimesh::fxform& matrix, float offsetValue, bool _offset)
	{
		if (!mesh || faceID < 0 || faceID >= (int)mesh->faces.size())
			return false;

		const trimesh::TriMesh::Face& face = mesh->faces.at(faceID);
		position.clear();
		position.push_back(matrix * mesh->vertices.at(face.x));
		position.push_back(matrix * mesh->vertices.at(face.y));
		position.push_back(matrix * mesh->vertices.at(face.z));

		if (_offset)
		{
			normal = trimesh::trinorm(position.at(0), position.at(1), position.at(2));
			trimesh::normalize(normal);

			position.at(0) += offsetValue * normal;
			position.at(1) += offsetValue * normal;
			position.at(2) += offsetValue * normal;
		}
		return true;
	}

	ModelNDataPtr createModelNData(ModelCreateInput input, ccglobal::Tracer* tracer,
		const ModelNDataCreateParam& param)
	{
		if (input.mesh)
		{
			if (tracer)
				tracer->resetProgressScope();

			input.mesh->normals.clear();

			bool processResult = true;
			if (param.dumplicate)
			{
				processResult = msbase::dumplicateMesh(input.mesh.get(), tracer);
				if (!processResult)
				{
					return nullptr;
				}
			}

			bool have = msbase::checkDegenerateFace(input.mesh.get(), true);
			if (have)
			{
				qDebug() << QString("msbase::checkDegenerateFace true : [have degenerate face]");
			}

			input.mesh->clear_bbox();
			input.mesh->need_bbox();

			trimesh::vec3 offset;
			if(param.toCenter)
				offset = msbase::moveTrimesh2Center(input.mesh.get(), false);

			ModelNDataPtr data(new ModelNData());
			data->mesh = input.mesh;
			data->input = input;
			data->offset = offset;
			data->colors = input.colors;
			data->seams = input.seams;
			data->supports = input.supports;

			trimesh::TriMesh* hull = qhullWrapper::convex_hull_3d(input.mesh.get());
			msbase::dumplicateMesh(hull);

			data->hull.reset(hull);
			
			if (param.indexRender && input.mesh->colors.size() == 0)
				data->updateIndexRenderData();
			else
				data->updateRenderData();

			return data;
		}

		return nullptr;
	}

	ModelNDataPtr createModelNData(TriMeshPtr mesh, const QString& name,
		ModelNDataType type, bool toCenter, ccglobal::Tracer* tracer)
	{
		ModelCreateInput input;
		input.mesh = mesh;
		input.name = name;
		input.type = type;

		ModelNDataCreateParam param;
		param.toCenter = toCenter;

		return createModelNData(input, tracer, param);
	}
}