#include "modelninput.h"
#include "msbase/utils/box2dgrid.h"
#include "msbase/primitive/primitive.h"

namespace creative_kernel
{
	ModelNInput::ModelNInput(ModelN* model, QObject* parent)
		:ModelInput(parent)
		, m_model(model)
	{
		settings->merge(m_model->setting());
		trimesh::TriMesh* mmesh = new trimesh::TriMesh();
		*mmesh = *m_model->globalMesh();

		m_model->mergePaintSupport();
		m_model->mergePaintSupport_anti_overhang();
		m_model->mergePaintSeam();
		m_model->mergePaintSeam_anti();

		m_mesh = TriMeshPtr(mmesh);
		outline_ObjectExclude = model->getoutline_ObjectExclude();
	}

	ModelNInput::~ModelNInput()
	{
	}

	TriMeshPtr ModelNInput::ptr()
	{
		return m_model->globalMesh(false);
	}

	void ModelNInput::tiltSliceSet(trimesh::vec axis, float angle)
	{
		float theta = angle * M_PIf / 180.0f;
		trimesh::rot(m_mesh.get(), theta, axis);
	}

	trimesh::TriMesh* ModelNInput::generateSlopeSupport(float rotation_angle, trimesh::vec axis, float support_angle, bool bottomBigger)
	{
		auto RotateByVector = [](trimesh::vec3 old_pt, trimesh::vec3 axit, trimesh::vec3 offset, double theta)
		{
			old_pt -= offset;
			double r = theta * M_PIf / 180;
			double c = cos(r);
			double s = sin(r);
			double new_x = (axit.x * axit.x * (1 - c) + c) * old_pt.x + (axit.x * axit.y * (1 - c) - axit.z * s) * old_pt.y + (axit.x * axit.z * (1 - c) + axit.y * s) * old_pt.z;
			double new_y = (axit.y * axit.x * (1 - c) + axit.z * s) * old_pt.x + (axit.y * axit.y * (1 - c) + c) * old_pt.y + (axit.y * axit.z * (1 - c) - axit.x * s) * old_pt.z;
			double new_z = (axit.x * axit.z * (1 - c) - axit.y * s) * old_pt.x + (axit.y * axit.z * (1 - c) + axit.x * s) * old_pt.y + (axit.z * axit.z * (1 - c) + c) * old_pt.z;
			if (new_z < 0 && new_z > -100) 
				new_z = 0;
			trimesh::vec3 output = trimesh::vec3(new_x, new_y, new_z);
			return output;
		};

		trimesh::TriMesh* mesh_temp = new trimesh::TriMesh();
		*mesh_temp = *m_mesh;
		msbase::Box2DGrid grid_rot_before;
		trimesh::fxform xf = trimesh::fxform::identity();
		grid_rot_before.build(mesh_temp, xf, false);

		mesh_temp->need_bbox();
		trimesh::box3 box = mesh_temp->bbox;
		float uplift_height = -box.min.z + M_SQRT2f * fmax(fmax(box.size().x, box.size().y), box.size().z);
		trimesh::xform txf = trimesh::xform::trans(trimesh::vec3(0.0, 0.0, uplift_height));
		trimesh::apply_xform(mesh_temp, txf);

		float supportSize = std::max(box.size().x, box.size().y) / 30.0f;

		std::vector<msbase::VerticalSeg> supports;
		msbase::Box2DGrid grid;
		grid.build(mesh_temp, xf, false);
		grid.buildGlobalProperties();
		grid.autoSupport(supports, supportSize, support_angle, false);
		size_t sizeOfFaceSupport = supports.size();
		grid.autoSupportOfVecAndSeg(supports, supportSize, false);
		size_t size_all = supports.size();
		if (size_all == 0)
			return nullptr;

		for (int i = 0; i < size_all; ++i)
		{
			msbase::VerticalSeg& seg = supports.at(i);
			seg.t = RotateByVector(seg.t, axis, trimesh::vec3(0, 0, uplift_height), -rotation_angle);
			seg.b = seg.t - trimesh::vec3(0, 0, seg.t[2]);
		}
		supports = grid_rot_before.recheckVertexSupports(supports);

		size_all = supports.size();
		trimesh::TriMesh* supportMesh = new trimesh::TriMesh();
		int vertexNum = 12 * 3 * size_all;
		supportMesh->vertices.resize(vertexNum);
		for (int i = 0; i < size_all; ++i)
		{
			msbase::VerticalSeg& seg = supports.at(i);
			float topSize = supportSize;
			float bottomSize = supportSize;
			if (i >= sizeOfFaceSupport)
			{
				topSize = topSize > 5.0f ? topSize : 5.0f;
				seg.t[2] += 2.0;
				if (bottomBigger)
					bottomSize = (1 + seg.t[2] / 100) * topSize;
				else
					bottomSize = topSize;
			}

			msbase::fillCylinderSoup(&supportMesh->vertices.at(0) + 36 * i, bottomSize, seg.b,
				topSize, seg.t, 4, -rotation_angle);
		}

		size_t vertexSize = supportMesh->vertices.size();
		if (vertexSize < 3)
		{
			delete supportMesh;
			return nullptr;
		}

		supportMesh->faces.reserve(vertexSize / 3);
		for (size_t i = 0; i < vertexSize / 3; ++i)
		{
			trimesh::TriMesh::Face f;
			f.x = (int)(3 * i);
			f.y = (int)(3 * i + 1);
			f.z = (int)(3 * i + 2);
			supportMesh->faces.push_back(f);
		}

		return supportMesh;
	}

}