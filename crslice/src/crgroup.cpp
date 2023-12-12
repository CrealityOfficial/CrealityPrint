#include "crgroup.h"

#include "jsonhelper.h"
#include "ccglobal/log.h"
#include "ccglobal/serial.h"

namespace crslice
{
	CrGroup::CrGroup()
	{
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
	}

	void CrGroup::save(std::fstream& out, int version)
	{
		m_settings->save(out);
		int objectCount = (int)m_objects.size();
		ccglobal::cxndSaveT(out, objectCount);
		for (int i = 0; i < objectCount; ++i)
			m_objects.at(i).save(out, version);
	}
}