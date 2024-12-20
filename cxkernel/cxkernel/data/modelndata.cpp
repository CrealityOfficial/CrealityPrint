#include "modelndata.h"
#include "cxkernel/data/trimeshutils.h"

#include "qhullWrapper/hull/meshconvex.h"
#include "qhullWrapper/hull/hullface.h"

#include "msbase/mesh/checker.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/mesh/tinymodify.h"
#include "msbase/mesh/merge.h"

#include "qtusercore/module/progressortracer.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

namespace cxkernel
{
	ModelNData::ModelNData() 
		: defaultColor(0)
	{
	
	}

	ModelNData::~ModelNData()
	{
	}

	ModelNDataPtr ModelNData::clone()
	{
		ModelNData* clones = new ModelNData;

		trimesh::TriMesh* tempMesh = new trimesh::TriMesh;
		msbase::copyTrimesh2Trimesh(mesh.get(), tempMesh);
		clones->mesh.reset(tempMesh);

		tempMesh = new trimesh::TriMesh;
		msbase::copyTrimesh2Trimesh(hull.get(), tempMesh);
		clones->hull.reset(tempMesh);

		clones->colors =  colors;
		clones->defaultColor = defaultColor;
		clones->spreadFaces = spreadFaces;
		clones->spreadFaceCount = spreadFaceCount;
		clones->seams = seams;
		clones->supports = supports;
		clones->offset = offset;
		clones->input.mesh = clones->mesh;
		clones->input.colors = clones->colors;
		clones->input.seams = clones->seams;
		clones->input.supports = clones->supports;
		clones->input.fileName = input.fileName;
		clones->input.name = input.name;
		clones->input.description = input.description;
		clones->input.type = input.type;

		return ModelNDataPtr(clones);
	}

	int ModelNData::primitiveNum()
	{
		return mesh ? (int)mesh->faces.size() : 0;
	}

	trimesh::box3 ModelNData::calculateBox(const trimesh::fxform& matrix)
	{
		trimesh::box3 b;

		if (hull)
		{
			auto total_box = tbb::parallel_reduce(
				tbb::blocked_range<int>(0, hull->vertices.size()),
				trimesh::box3(),
				[&](tbb::blocked_range<int> r, trimesh::box3 totalBox)
				{
					for (int i = r.begin(); i < r.end(); ++i)
					{
						totalBox += matrix * hull->vertices[i];
					}

					return totalBox;
				}, std::plus<trimesh::box3>());

			b = total_box;

			//for (const trimesh::vec3& v : hull->vertices)
			//	b += matrix * v;
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
			qhullWrapper::hullFacesFromMeshNear(mesh, _faces);
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

	void ModelNData::adaptSmallBox(const trimesh::box3& box)
	{
		if (!box.valid)
			return;

		trimesh::xform xf = trimesh::xform::scale(1000.0);

		if (mesh)
			trimesh::apply_xform(mesh.get(), xf);
		if (hull)
			trimesh::apply_xform(hull.get(), xf);
	}

	void ModelNData::adaptBigBox(const trimesh::box3& box)
	{
		trimesh::box3 _box = box;
		trimesh::box3 _b = calculateBox();

		if (!_box.valid)
			return;

		if (!_b.valid)
			return;

		trimesh::vec3 bsize = 0.9f * _box.size();
		trimesh::vec3 scale = bsize / _b.size();
		float s = scale.min();
		trimesh::xform xf = trimesh::xform::scale(s);

		if (mesh)
			trimesh::apply_xform(mesh.get(), xf);
		if (hull)
			trimesh::apply_xform(hull.get(), xf);
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
			//if (tracer)
			//	tracer->resetProgressScope();

			input.mesh->normals.clear();

// 			bool processResult = true;
// 			if (param.dumplicate)
// 			{
// 				processResult = msbase::dumplicateMesh(input.mesh.get(), tracer);
// 				if (!processResult)
// 				{
// 					return nullptr;
// 				}
// 			}

			std::vector<bool> valids;
			bool have = msbase::checkDegenerateFace(input.mesh.get(), valids, true);
			if (have)
			{
				msbase::mantainValids(input.colors, valids);
				msbase::mantainValids(input.seams, valids);
				msbase::mantainValids(input.supports, valids);
				qDebug() << QString("msbase::checkDegenerateFace true : [have degenerate face]");
			}

			input.mesh->clear_bbox();
			input.mesh->need_bbox();

			trimesh::vec3 offset;
			if(param.toCenter)
				offset = msbase::moveTrimesh2Center(input.mesh.get(), false);

            bool processResult = true;
            if (param.dumplicate)
            {
                processResult = msbase::dumplicateMesh(input.mesh.get(), tracer);
                if (!processResult)
                {
                    return nullptr;
                }
            }

			ModelNDataPtr data(new ModelNData());
			data->mesh = input.mesh;
			data->offset = offset;
			data->defaultColor = input.defaultColor;
			data->colors.swap(input.colors);
			data->seams.swap(input.seams);
			data->supports.swap(input.supports);
			data->input = input;

			trimesh::TriMesh* hull = qhullWrapper::convex_hull_3d(input.mesh.get());  
			msbase::dumplicateMesh(hull);

			data->hull.reset(hull);

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