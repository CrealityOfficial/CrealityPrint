#include "supportinput.h"

#include "data/modeln.h"
#include "qtuser3d/trimesh2/conv.h"
#include "crslice/support.h"

namespace creative_kernel
{
	SupportInput::SupportInput(QObject* parent)
		: ModelInput(parent)
		, m_mesh(nullptr)
	{
	}

	SupportInput::~SupportInput()
	{
	}

	void SupportInput::setTrimesh(trimesh::TriMesh* mesh)
	{
		m_mesh = mesh;
	}

	void SupportInput::build()
	{
		trimesh::TriMesh* mesh = new trimesh::TriMesh;
		if (m_mesh->vertices.size() != 3 * m_mesh->faces.size())
		{
			size_t fnum = m_mesh->faces.size();
			if (fnum > 0)
			{
				mesh->vertices.resize(3 * fnum);
				for (size_t i = 0; i < fnum; ++i)
				{
					trimesh::TriMesh::Face& face = m_mesh->faces.at(i);
					for (int j = 0; j < 3; ++j)
					{
						int index = face[j];
						mesh->vertices.at(3 * i + j) = m_mesh->vertices.at(index);
					}
				}
			}
			m_mesh = mesh;
		}
	}

	void SupportInput::buildFDM()
	{
		QMatrix4x4 globalMatrix = m_model->globalMatrix();
		trimesh::fxform xf = qtuser_3d::qMatrix2Xform(globalMatrix);
		size_t size = m_mesh->vertices.size();
		for (size_t i = 0; i < size; ++i)
		{
			trimesh::vec3 v = m_mesh->vertices.at(i);
			m_mesh->vertices.at(i) = xf * v;
		}
	}

	void SupportInput::tiltSliceSet(trimesh::vec axis, float angle)
	{
		float theta = angle * M_PIf / 180.0f;
		trimesh::rot(m_mesh, theta, axis);
	}

	void SupportInput::setModel(creative_kernel::ModelN* model)
	{
		m_model = model;
	}

	int SupportInput::vertexNum() const
	{
		return (int)m_mesh->vertices.size();
	}

	float* SupportInput::position() const
	{
		return (m_mesh->vertices.size() > 0) ? (float*)&m_mesh->vertices.at(0) : nullptr;
	}

	float* SupportInput::normal() const
	{
		return nullptr;
	}

	int SupportInput::triangleNum() const
	{
		//return (int)m_mesh.faces.size();
		return m_mesh->vertices.size() / 3;
	}

	int* SupportInput::indices() const
	{
		return (m_mesh->faces.size() > 0) ? (int*)&m_mesh->faces.at(0) : nullptr;
	}

	const creative_kernel::TriMeshPtr SupportInput::ptr() const
	{
		creative_kernel::TriMeshPtr meh(m_mesh);
		return meh;
	}

	const creative_kernel::TriMeshPtr SupportInput::beltMesh() const
	{
		trimesh::TriMesh* mesh = new trimesh::TriMesh();
		*mesh = *m_model->globalMesh().get();
		creative_kernel::TriMeshPtr beltmesh(mesh);
		trimesh::fxform xf = crslice::beltXForm(m_offset, 45.0f);
		trimesh::apply_xform(beltmesh.get(), trimesh::xform(xf));
		beltmesh->need_bbox();
		return beltmesh;
	}

	void SupportInput::setBeltOffset(const trimesh::vec3& offset)
	{
		m_offset = offset;
	}

	void SupportInput::beltSet(trimesh::vec3 offset)
	{
#if 1
		trimesh::fxform xf = crslice::beltXForm(offset, 45.0f);
		trimesh::apply_xform(m_mesh, trimesh::xform(xf));

		m_mesh->need_bbox();
#else
		float theta = 45.0f * M_PIf / 180.0f;

		trimesh::fxform xf0 = trimesh::fxform::trans(offset);

		trimesh::fxform xf1 = trimesh::fxform::identity();
		xf1(2, 2) = 1.0f / sinf(theta);
		xf1(1, 2) = 1.0f / tanf(theta);

		trimesh::fxform xf2 = trimesh::fxform::identity();
		xf2(2, 2) = 0.0f;
		xf2(1, 1) = 0.0f;
		xf2(2, 1) = 1.0f;
		xf2(1, 2) = 1.0f;

		trimesh::fxform xf3 = trimesh::fxform::identity();
		xf3(1, 1) = 0.0f;
		xf3(2, 1) = 1.0f / tanf(theta);
		xf3(1, 2) = 1.0f / sinf(theta);

		//trimesh::fxform xf = xf2 * xf1 * xf0;
		trimesh::fxform xf = xf3 * xf0;
		trimesh::apply_xform(m_mesh, trimesh::xform(xf));

		m_mesh->need_bbox();
#endif
	}
}