#include "trimeshinput.h"
#include "data/modeln.h"

#include "mmesh/trimesh/trimeshutil.h"
#include "mmesh/trimesh/shapecreator.h"
#include "mmesh/trimesh/filler.h"
#include "qcxutil/trimesh2/conv.h"

using namespace trimesh;

namespace creative_kernel
{
	TrimeshInput::TrimeshInput(QObject* parent)
		:ModelInput(parent)
		, m_model(nullptr)
		, m_mesh(nullptr)
	{
	}
	TrimeshInput::~TrimeshInput()
	{
		if (m_mesh)
		{
			delete m_mesh;
			m_mesh = nullptr;
		}
	}

	void TrimeshInput::build()
	{
		trimesh::TriMesh* mesh = m_model->mesh();
		m_mesh = new trimesh::TriMesh();
		if (mesh->vertices.size() == 3 * mesh->faces.size())
		{
			*m_mesh = *mesh;
		}
		else   //indices
		{
			size_t fnum = mesh->faces.size();
			if (fnum > 0)
			{
				m_mesh->vertices.resize(3 * fnum);
				for (size_t i = 0; i < fnum; ++i)
				{
					trimesh::TriMesh::Face& face = mesh->faces.at(i);
					for (int j = 0; j < 3; ++j)
					{
						int index = face[j];
						m_mesh->vertices.at(3 * i + j) = mesh->vertices.at(index);
					}
				}
			}
		}

		QMatrix4x4 globalMatrix = m_model->globalMatrix();
		trimesh::fxform xf = qcxutil::qMatrix2Xform(globalMatrix);
		size_t size = m_mesh->vertices.size();
		for (size_t i = 0; i < size; ++i)
		{
			trimesh::vec3 v = m_mesh->vertices.at(i);
			m_mesh->vertices.at(i) = xf * v;
		}
	}

	void TrimeshInput::tiltSliceSet(trimesh::vec axis, float angle)
	{
		float theta = angle * M_PIf / 180.0f;
		trimesh::rot(m_mesh, theta, axis);
	}

	void TrimeshInput::beltSet()
	{
#if 1
		trimesh::fxform xf = mmesh::beltXForm(m_offset, 45.0f);
		trimesh::apply_xform(m_mesh, trimesh::xform(xf));

		m_mesh->need_bbox();
#else
		double w = 200;
		trimesh::box3 box0;
		m_mesh->clear_bbox();
		m_mesh->need_bbox();
		box0 += m_mesh->bbox;
		trimesh::xform txf3 = trimesh::xform::trans(trimesh::vec3(w / 2.0 - box0.center().x, 0.0, -box0.min.z));
		trimesh::apply_xform(m_mesh, txf3);

		trimesh::box3 bbox1;
		for (trimesh::vec3& v : m_mesh->vertices)
		{
			trimesh::vec3 vv = v;
			vv.y += v.z;
			bbox1 += vv;
		}
		trimesh::xform ttxf3 = trimesh::xform::trans(0.0, -bbox1.min.y, 0.0);
		trimesh::apply_xform(m_mesh, ttxf3);

		trimesh::xform ttxf33 = trimesh::xform::trans(-(w / 2.0 - box0.center().x), bbox1.min.y, box0.min.z);
		trimesh::apply_xform(m_mesh, ttxf33);

		trimesh::xform xf = trimesh::xform::rot(-45.0 * M_PI / 180.0, 1.0, 0.0, 0.0);
		trimesh::apply_xform(m_mesh, xf);

		trimesh::box3 box;
		m_mesh->clear_bbox();
		m_mesh->need_bbox();
		box += m_mesh->bbox;

		trimesh::xform txf = trimesh::xform::trans(trimesh::vec3(w / 2.0 - box.center().x, 0.0, -box.min.z));
		trimesh::apply_xform(m_mesh, txf);
		box.clear();
		m_mesh->clear_bbox();
		m_mesh->need_bbox();
		box += m_mesh->bbox;

		trimesh::box3 bbox;
		for (trimesh::vec3& v : m_mesh->vertices)
		{
			trimesh::vec3 vv = v;
			vv.y += v.z;
			bbox += vv;
		}

		trimesh::xform ttxf = trimesh::xform::trans(0.0, -bbox.min.y, 0.0);
		trimesh::apply_xform(m_mesh, ttxf);
		for (trimesh::vec3& v : m_mesh->vertices)
		{
			float y = v.y + v.z;
		}

		trimesh::box3 box2;
		m_mesh->clear_bbox();
		m_mesh->need_bbox();
		box2 += m_mesh->bbox;
#endif
	}

	void TrimeshInput::setModel(creative_kernel::ModelN* model)
	{
		m_model = model;
		settings->merge(m_model->setting());
	}

	int TrimeshInput::vertexNum() const
	{
		return (int)m_mesh->vertices.size();
	}

	float* TrimeshInput::position() const
	{
		return (m_mesh->vertices.size() > 0) ? (float*)&m_mesh->vertices.at(0) : nullptr;
	}

	float* TrimeshInput::normal() const
	{
		return nullptr;
	}

	int TrimeshInput::triangleNum() const
	{
		//return (int)m_mesh.faces.size();
		return m_mesh->vertices.size() / 3;
	}

	int* TrimeshInput::indices() const
	{
		return (m_mesh->faces.size() > 0) ? (int*)&m_mesh->faces.at(0) : nullptr;
	}

	const creative_kernel::TriMeshPtr TrimeshInput::ptr() const
	{
		return m_model->globalMesh();
	}

	void TrimeshInput::setBeltOffset(const trimesh::vec3& offset)
	{
		m_offset = offset;
	}

	const creative_kernel::TriMeshPtr TrimeshInput::beltMesh() const
	{
		trimesh::TriMesh* mesh = new trimesh::TriMesh();
		*mesh = *m_model->globalMesh().get();
		creative_kernel::TriMeshPtr beltmesh(mesh);
		trimesh::fxform xf = mmesh::beltXForm(m_offset, 45.0f);
		trimesh::apply_xform(beltmesh.get(), trimesh::xform(xf));
		beltmesh->need_bbox();
		return beltmesh;
	}

	trimesh::TriMesh* TrimeshInput::generateBeltSupport(trimesh::TriMesh* mesh, float angle)
	{
		trimesh::TriMesh* mesh_temp = new trimesh::TriMesh();
		*mesh_temp = *mesh;
		QMatrix4x4 globalMatrix = m_model->globalMatrix();
		trimesh::fxform xf1 = qcxutil::qMatrix2Xform(globalMatrix);
		size_t size1 = mesh_temp->vertices.size();
		for (size_t i = 0; i < size1; ++i)
		{
			trimesh::vec3 v = mesh_temp->vertices.at(i);
			mesh_temp->vertices.at(i) = xf1 * v;
		}

		mesh_temp->need_bbox();
		trimesh::box3 box = mesh_temp->bbox;
		float supportSize = std::max(box.size().x, box.size().y) / 30.0f;

		std::vector<mmesh::VerticalSeg> supports;
		mmesh::Box2DGrid grid;


		trimesh::fxform xf = trimesh::fxform::identity();
		grid.build(mesh_temp, xf, false);
		grid.autoSupport(supports, supportSize, angle, false);
		size_t size = supports.size();

		if (size == 0)
			return nullptr;

		trimesh::TriMesh* supportMesh = new trimesh::TriMesh();
		float r = supportSize * sqrtf(2.0f) * 0.9f / 2.0f;

		int vertexNum = 12 * 3 * size;
		supportMesh->vertices.resize(vertexNum);
		for (int i = 0; i < size; ++i)
		{
			mmesh::VerticalSeg& seg = supports.at(i);
			mmesh::fillCylinderSoup(&supportMesh->vertices.at(0) + 36 * i, supportSize, seg.b,
				supportSize, seg.t, 4, 45.0f);
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

	trimesh::vec3 RotateByVector(trimesh::vec3 old_pt, trimesh::vec3 axit, trimesh::vec3 offset, double theta)
	{
		old_pt -= offset;
		double r = theta * M_PIf / 180;
		double c = cos(r);
		double s = sin(r);
		double new_x = (axit.x * axit.x * (1 - c) + c) * old_pt.x + (axit.x * axit.y * (1 - c) - axit.z * s) * old_pt.y + (axit.x * axit.z * (1 - c) + axit.y * s) * old_pt.z;
		double new_y = (axit.y * axit.x * (1 - c) + axit.z * s) * old_pt.x + (axit.y * axit.y * (1 - c) + c) * old_pt.y + (axit.y * axit.z * (1 - c) - axit.x * s) * old_pt.z;
		double new_z = (axit.x * axit.z * (1 - c) - axit.y * s) * old_pt.x + (axit.y * axit.z * (1 - c) + axit.x * s) * old_pt.y + (axit.z * axit.z * (1 - c) + c) * old_pt.z;
		if (new_z < 0 && new_z > -100) new_z = 0;
		trimesh::vec3 output = trimesh::vec3(new_x, new_y, new_z);
		return output;
	}

	trimesh::TriMesh* TrimeshInput::generateSlopeSupport(trimesh::TriMesh* mesh, float rotation_angle, trimesh::vec axis, float support_angle, bool bottomBigger)
	{
		trimesh::TriMesh* mesh_temp = new trimesh::TriMesh();
		*mesh_temp = *mesh;
		QMatrix4x4 globalMatrix = m_model->globalMatrix();
		trimesh::fxform xf1 = qcxutil::qMatrix2Xform(globalMatrix);
		size_t size1 = mesh_temp->vertices.size();
		for (size_t i = 0; i < size1; ++i)
		{
			trimesh::vec3 v = mesh_temp->vertices.at(i);
			mesh_temp->vertices.at(i) = xf1 * v;
		}

		mmesh::Box2DGrid grid_rot_before;
		trimesh::fxform xf = trimesh::fxform::identity();
		grid_rot_before.build(mesh_temp, xf, false);

		float theta = rotation_angle * M_PIf / 180.0f;
		trimesh::rot(mesh_temp, theta, axis);
		mesh_temp->need_bbox();
		trimesh::box3 box = mesh_temp->bbox;
		float uplift_height = -box.min.z + M_SQRT2f * fmax(fmax(box.size().x, box.size().y), box.size().z);
		trimesh::xform txf = trimesh::xform::trans(trimesh::vec3(0.0, 0.0, uplift_height));
		trimesh::apply_xform(mesh_temp, txf);

		float supportSize = std::max(box.size().x, box.size().y) / 30.0f;

		std::vector<mmesh::VerticalSeg> supports;
		mmesh::Box2DGrid grid;
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
			mmesh::VerticalSeg& seg = supports.at(i);
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
			mmesh::VerticalSeg& seg = supports.at(i);
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

			mmesh::fillCylinderSoup(&supportMesh->vertices.at(0) + 36 * i, bottomSize, seg.b,
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


		//txf = trimesh::xform::trans(trimesh::vec3(0.0, 0.0, -uplift_height));
		//trimesh::apply_xform(supportMesh, txf);
		//trimesh::rot(supportMesh, -theta, axis);

		//splitslot::SplitSlotParam aparam;
		//aparam.depth = 2.0;
		//aparam.gap = 2.0;
		//aparam.haveSlot = false;
		//aparam.redius = 2.0;
		//aparam.xyOffset = 0.3;
		//aparam.zOffset = 0.3;
		//splitslot::SplitPlane aplane;
		//aplane.normal = trimesh::vec3(0, 0, 1);
		//aplane.position = QVector3D(0, 0, 0);
		//std::vector<trimesh::TriMesh*> meshes;
		//splitslot::splitSlot(supportMesh, aplane, aparam, meshes);
		//for (trimesh::TriMesh* newSupportMesh : meshes)
		//{
		//	newSupportMesh->need_bbox();
		//	trimesh::box3 box1 = newSupportMesh->bbox;
		//	if (newSupportMesh->bbox.center().z > 0)
		//		return newSupportMesh;
		//}

		return supportMesh;
	}
}