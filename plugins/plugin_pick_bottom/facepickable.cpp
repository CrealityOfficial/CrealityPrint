#include "facepickable.h"
#include "qtuser3d/entity/pickfaceentity.h"

	FacePickable::FacePickable(trimesh::TriMesh* mesh,QObject* parent)
		:Pickable(parent)
		, m_mesh(mesh)
		, m_hoverColor(QVector4D(1.0f, 0.824f, 0.0f, 1.0f))
		, m_noHoverColor(QVector4D(1.0f, 1.0f, 1.0f, 0.4f))
	{
		m_face = new qtuser_3d::PickFaceEntity(nullptr);
		std::vector<QVector3D> vertexData;
		for (size_t i = 0; i < mesh->vertices.size(); i++)
		{
			QVector3D newPosition = QVector3D(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z);
			vertexData.push_back(newPosition);
		}
		m_face->updateData(vertexData);
		m_mesh->normals[0];

		setEnableSelect(false);
	}

	FacePickable::~FacePickable()
	{
		m_face->setParent((Qt3DCore::QNode*)nullptr);
		delete m_face;
	}

	int FacePickable::primitiveNum()
	{
		return m_mesh->faces.size();
	}


	void FacePickable::setEntity(qtuser_3d::PickFaceEntity* faceEntity)
	{
		m_face = faceEntity;
	}

	void FacePickable::setOriginFacesArea(trimesh::TriMesh* mesh)
	{
		auto triangleArea = [](trimesh::point& p0, trimesh::point& p1, trimesh::point& p2)
		{
			trimesh::vec3 a = p0 - p1;
			trimesh::vec3 b = p1 - p2;
			return sqrt(pow((a.y * b.z - a.z * b.y), 2) + pow((a.z * b.x - a.x * b.z), 2)
				+ pow((a.x * b.y - a.y * b.x), 2)) / 2.0f;
		};
		m_origin_faces_area = 0;
		trimesh::point this_normal = gNormal();
		for (size_t i = 0; i < mesh->faces.size(); i++)
		{
			int test_pt_idx = mesh->faces.at(i).x;
			trimesh::point nor_pt = mesh->vertices[test_pt_idx] - m_mesh->vertices.back();
			double distance2face = nor_pt ^ this_normal;
			if (std::abs(distance2face) > 0.2) continue;
			trimesh::point normal_ptr = trimesh::normalized(trimesh::trinorm(mesh->vertices[mesh->faces.at(i).x],
				mesh->vertices[mesh->faces.at(i).y],
				mesh->vertices[mesh->faces.at(i).z]));
			if (std::abs(this_normal.at(0) - normal_ptr.at(0)) < 0.001
				&& std::abs(this_normal.at(1) - normal_ptr.at(1)) < 0.001
				&& std::abs(this_normal.at(2) - normal_ptr.at(2)) < 0.001)
			{
				m_origin_faces_area += triangleArea(mesh->vertices[mesh->faces.at(i).x]
					, mesh->vertices[mesh->faces.at(i).y]
					, mesh->vertices[mesh->faces.at(i).z]);
			}
		}
	}

	double FacePickable::getOriginFacesArea()
	{
		return m_origin_faces_area;
	}

	trimesh::vec3 FacePickable::gNormal()
	{
		return m_mesh->normals.at(0);
	}

	qtuser_3d::PickFaceEntity* FacePickable::getEntity()
	{
		if (m_face)
		{
			return m_face;
		}
		return nullptr;
	}

	void FacePickable::onStateChanged(qtuser_3d::ControlState state)
	{
		if (m_face)
		{
			m_face->setState((float)state);
			if (state == qtuser_3d::ControlState::hover)
			{
				m_face->setColor(m_hoverColor);
			}
			else
			{
				m_face->setColor(m_noHoverColor);
			}
			
		}
	}

	void FacePickable::faceBaseChanged(int faceBase)
	{
		if (m_face)
		{
			QPoint vertexBase(0,0);
			vertexBase.setX(faceBase * 3);
			vertexBase.setY(m_noPrimitive ? 1 : 0);
			m_face->setVertexBase(vertexBase);
		}
	}
