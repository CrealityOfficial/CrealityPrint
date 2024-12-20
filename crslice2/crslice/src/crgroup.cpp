#include "crgroup.h"

#include "jsonhelper.h"
#include "ccglobal/log.h"
#include "ccglobal/serial.h"

namespace crslice2
{
	CrGroup::CrGroup()
	{
		m_sceneObjectId = -1;
		m_settings.reset(new Settings());
	}

	CrGroup::~CrGroup()
	{

	}

	int CrGroup::addObject()
	{
		int objectID = (int)m_objects.size();
		m_objects.emplace_back(CrObject());
		return objectID;
	}

	void CrGroup::setObjectMesh(int objectID, TriMeshPtr mesh)
	{
		if (objectID < 0 || objectID >= (int)m_objects.size())
		{
			LOGE("CrGroup::setMesh [%d] not exist.", objectID);
			return;
		}

		CrObject& object = m_objects.at(objectID);
		object.m_mesh = mesh;
        object.m_mesh->need_bbox();

		trimesh::box3 b = mesh->bbox;

		trimesh::vec3 offset = trimesh::vec3(0.0f, 0.0f, 0.0f) - b.center();
		trimesh::trans(mesh.get(), offset);
		object.m_xform = trimesh::xform::trans(-offset);
	}

	void CrGroup::setObjectMeshPaint(int objectID, TriMeshPtr mesh, const trimesh::xform& componentXform
		, const std::vector<std::string>& colors2Facets
		, const std::vector<std::string>& seam2Facets
		, const std::vector<std::string>& support2Facets
		, const std::string& objectName
		, const std::vector<double>& layerHeight
		, const int modelType)
	{
		if (objectID < 0 || objectID >= (int)m_objects.size())
		{
			LOGE("CrGroup::setMesh [%d] not exist.", objectID);
			return;
		}

		CrObject& object = m_objects.at(objectID);
		object.m_mesh = mesh;
		object.m_mesh->need_bbox();
		object.m_colors2Facets = colors2Facets;
		object.m_seam2Facets = seam2Facets;
		object.m_support2Facets = support2Facets;
		object.m_objectName = objectName;
		object.m_layerHeight = layerHeight;
		object.modelType = modelType;
		//for (int i = 0; i < 16; i++)
		//{
		//	object.m_xform[i] = xform[i];
		//}

		object.m_xform = componentXform;
	}

	void CrGroup::setObjectSettings(int objectID, SettingsPtr settings)
	{
		if (objectID < 0 || objectID >= (int)m_objects.size())
		{
			LOGE("CrGroup::setObjectSettings [%d] not exist.", objectID);
			return;
		}

		CrObject& object = m_objects.at(objectID);
		object.m_settings = settings;
	}

	void CrGroup::setSettings(SettingsPtr settings)
	{
		m_settings = settings;
	}

	void CrGroup::setOffset(trimesh::vec3 offset)
	{
		m_offset = offset;
	}

	void CrGroup::load(std::ifstream& in)
	{
		m_settings->load(in);
		int objectCount = templateLoad<int>(in);
		if (objectCount > 0)
			m_objects.resize(objectCount);
		for (int i = 0; i < objectCount; ++i)
			m_objects.at(i).load(in);
	}

	void CrGroup::save(std::ofstream& out)
	{
		m_settings->save(out);
		int objectCount = (int)m_objects.size();
		templateSave<int>(objectCount, out);
		for (int i = 0; i < objectCount; ++i)
			m_objects.at(i).save(out);
	}

	void CrGroup::load(std::fstream& in, int version)
	{
		m_settings->load(in);
		int objectCount = 0;
		ccglobal::cxndLoadT(in, objectCount);
		if (objectCount > 0)
			m_objects.resize(objectCount);
		for (int i = 0; i < objectCount; ++i)
			m_objects.at(i).load(in, version);

		if (version >= 101)
			loadXForm(m_groupTransform, in);
	}

	void CrGroup::save(std::fstream& out, int version)
	{
		m_settings->save(out);
		int objectCount = (int)m_objects.size();
		ccglobal::cxndSaveT(out, objectCount);
		for (int i = 0; i < objectCount; ++i)
			m_objects.at(i).save(out, version);

		if (version >= 101)
			saveXForm(m_groupTransform, out);
	}

	void CrGroup::setGroupTransform(trimesh::xform gxform)
	{
		m_groupTransform = gxform;
	}

	void CrGroup::setGroupSceneObjectId(int64_t sceneObjId)
	{
		m_sceneObjectId = sceneObjId;
	}
}