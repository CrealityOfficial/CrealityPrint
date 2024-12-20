#include "crslice/crscene.h"

#include "crgroup.h"
#include "crobject.h"
#include "jsonhelper.h"
#include "jsonloader.h"
#include "crslice/base/parametermeta.h"
#include "ccglobal/log.h"

#include "ccglobal/serial.h"

namespace crslice
{
	class CrSceneSerial : public ccglobal::Serializeable
	{
	public:
		CrSceneSerial(CrScene* _scene) 
			: scene(_scene)
		{

		}

		virtual ~CrSceneSerial() {

		}

		int version() override { // 100 - 200
			return 100;
		}

		bool save(std::fstream& out, ccglobal::Tracer* tracer) override {
			scene->m_settings->save(out);
			int extruderCount = (int)scene->m_extruders.size();
			ccglobal::cxndSaveT(out, extruderCount);
			for (int i = 0; i < extruderCount; ++i)
				scene->m_extruders.at(i)->save(out);

			int groupCount = (int)scene->m_groups.size();
			ccglobal::cxndSaveT(out, groupCount);
			for (int i = 0; i < groupCount; ++i)
				scene->m_groups.at(i)->save(out, version());

			return true;
		}

		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer){
			if (ver < 100 || ver > 200)
				return false;

			scene->m_settings->load(in);
			int extruderCount = 0;
			ccglobal::cxndLoadT(in, extruderCount);
			for (int i = 0; i < extruderCount; ++i)
			{
				SettingsPtr setting(new Settings());
				setting->load(in);
				scene->m_extruders.push_back(setting);
			}

			int groupCount = 0;
			ccglobal::cxndLoadT(in, groupCount);
			for (int i = 0; i < groupCount; ++i)
			{
				scene->addOneGroup();
				scene->m_groups.at(i)->load(in, ver);
			}

			return true;
		}

	protected:
		CrScene* scene;
	};

	CrScene::CrScene()
		:m_debugger(nullptr)
	{
		m_settings.reset(new Settings());
		machine_center_is_zero = false;
	}

	CrScene::~CrScene()
	{
		release();
	}

	int CrScene::addOneGroup()
	{
		int groupID = (int)m_groups.size();
		CrGroup* group = new CrGroup();
		m_groups.push_back(group);
		return groupID;
	}

	int CrScene::addObject2Group(int groupID)
	{
		int objectID = -1;
		if (groupID >= 0 && groupID < (int)m_groups.size())
		{
			CrGroup* group = m_groups.at(groupID);
			objectID = group->addObject();
		}
		return objectID;
	}

	void CrScene::setOjbectMesh(int groupID, int objectID, TriMeshPtr mesh)
	{
		if (groupID < 0 || groupID >= (int)m_groups.size())
		{
			LOGE("CrScene::setOjbectMesh [%d] not exist.", groupID);
			return;
		}

		CrGroup* group = m_groups.at(groupID);
		group->setObjectMesh(objectID, mesh);
	}

	void CrScene::setOjbectExclude(int groupID, int objectID, const std::string& fileName, std::vector<trimesh::vec3>& outline_ObjectExclude)
	{
		m_Object_Exclude_FileName.push_back(fileName);
		//save lines
		std::vector<std::vector<trimesh::vec2>> polys;

		std::vector<trimesh::vec2> ps;
		for (const auto& p : outline_ObjectExclude)
		{
			ps.push_back(trimesh::vec2(p.x * 1000, p.y * 1000));
		}
		polys.push_back(ps);

		savePloygons(polys, fileName);
	}

	void CrScene::setGroupOffset(int groupID, trimesh::vec3 offset)
	{
		if (groupID < 0 || groupID >= (int)m_groups.size())
		{
			LOGE("CrScene::setOjbectMesh [%d] not exist.", groupID);
			return;
		}

		CrGroup* group = m_groups.at(groupID);
		group->setOffset(offset);
	}

	void CrScene::setObjectSettings(int groupID, int objectID, SettingsPtr settings)
	{
		if (groupID < 0 || groupID >= (int)m_groups.size())
		{
			LOGE("CrScene::setOjbectMesh [%d] not exist.", groupID);
			return;
		}

		CrGroup* group = m_groups.at(groupID);
		group->setObjectSettings(objectID, settings);
	}

	void CrScene::setGroupSettings(int groupID, SettingsPtr settings)
	{
		if (groupID < 0 || groupID >= (int)m_groups.size())
		{
			LOGE("CrScene::setOjbectMesh [%d] not exist.", groupID); 
			return;
		}

		CrGroup* group = m_groups.at(groupID);
		group->setSettings(settings);
	}

	void CrScene::setSceneSettings(SettingsPtr settings)
	{
		m_settings = settings;
	}

	void CrScene::setSceneJsonFile(const std::string& fileName)
	{
		std::vector<KValues> extruders;
		if (loadJSON(fileName, m_settings->settings, extruders) != 0)
		{
			LOGE("setSceneJsonFile invalid json file: %s", fileName.c_str());
			return;
		}

		for (KValues& kvs : extruders)
		{
			SettingsPtr settings(new Settings());
			settings->settings.swap(kvs);
			m_extruders.push_back(settings);
		}
	}

	void CrScene::addSceneParameter(const std::string& key, const std::string& value)
	{
		m_settings->add(key, value);
	}

	void CrScene::setTempDirectory(const std::string& directory)
	{
		m_tempDirectory = directory;
	}

	void CrScene::release()
	{
		for (CrGroup* group : m_groups)
			delete group;
		m_groups.clear();
	}

	void CrScene::setOutputGCodeFileName(const std::string& fileName)
	{
		m_gcodeFileName = fileName;
	}

	void CrScene::setPloygonFileName(const std::string& fileName)
	{
		m_ploygonFileName = fileName;
	}

	void CrScene::setSupportFileName(const std::string& fileName)
	{
		m_supportFile = fileName;
	}

	void CrScene::setAntiSupportFileName(const std::string& fileName)
	{
		m_antiSupportFile = fileName;
	}
	void CrScene::setSeamFileName(const std::string& fileName)
	{
		m_seamFile = fileName;
	}
	void CrScene::setAntiSeamFileName(const std::string& fileName)
	{
		m_antiSeamFile = fileName;
	}

	bool CrScene::valid()
	{
		return true;
	}

	void CrScene::save(const std::string& fileName)
	{
		CrSceneSerial serial(this);
		ccglobal::cxndSave(serial, fileName);  //version 

#if 0
		std::ofstream out;
		out.open(fileName, std::ios_base::binary);
		if (out.is_open())
		{
			m_settings->save(out);
			int extruderCount = (int)m_extruders.size();
			templateSave<int>(extruderCount, out);
			for (int i = 0; i < extruderCount; ++i)
				m_extruders.at(i)->save(out);

			int groupCount = (int)m_groups.size();
			templateSave<int>(groupCount, out);
			for (int i = 0; i < groupCount; ++i)
				m_groups.at(i)->save(out);
		}
		out.close();
#endif
	}

	void CrScene::savePloygons(const std::vector<std::vector<trimesh::vec2>>& polys, const std::string filename)
	{
		int pNum = polys.size();
		std::fstream in(filename, std::ios::out | std::ios::binary);
		if (in.is_open() && pNum > 0)
		{
			in.write((char*)&pNum, sizeof(int));
			if (pNum > 0)
			{
				for (int i = 0; i < pNum; ++i)
				{
					int num = polys.at(i).size();
					in.write((char*)&num, sizeof(int));
					for (int j = 0; j < num; j++)
					{
						in.write((char*)&polys[i][j].x, sizeof(float));
						in.write((char*)&polys[i][j].y, sizeof(float));
					}
				}
			}
		}
	}

	void CrScene::savePloygons(const std::vector<std::vector<trimesh::vec2>>& polys)
	{
		int pNum = polys.size();
		std::fstream in(m_ploygonFileName, std::ios::out | std::ios::binary);
		if (in.is_open() && pNum> 0)
		{
			in.write((char*)&pNum, sizeof(int));
			if (pNum > 0)
			{
				for (int i = 0; i < pNum; ++i)
				{
					int num = polys.at(i).size();
					in.write((char*)&num, sizeof(int));
					for (int j = 0; j < num; j++)
					{
						in.write((char*)&polys[i][j].x, sizeof(float));
						in.write((char*)&polys[i][j].y, sizeof(float));
					}
				}
			}
		}
	}

	void CrScene::makeSureParameters()
	{
		MetasMap datas;
		parseMetasMap(datas);

		for (MetasMapIter it = datas.begin(); it != datas.end(); ++it)
		{
			if (!m_settings->has(it->first))
			{
				LOGI("parameter %s not in scene.", it->first.c_str());
				m_settings->add(it->first, it->second->default_value);
			}
		}
	}

	void CrScene::load(const std::string& fileName)
	{
		CrSceneSerial serial(this);
		if (!ccglobal::cxndLoad(serial, fileName))  //version 
		{
			std::ifstream in;
			in.open(fileName, std::ios_base::binary);
			if (in.is_open())
			{
				m_settings->load(in);
				int extruderCount = templateLoad<int>(in);
				for (int i = 0; i < extruderCount; ++i)
				{
					SettingsPtr setting(new Settings());
					setting->load(in);
					m_extruders.push_back(setting);
				}

				int groupCount = templateLoad<int>(in);
				for (int i = 0; i < groupCount; ++i)
				{
					addOneGroup();
					m_groups.at(i)->load(in);
				}
			}
			in.close();
		}
		makeSureParameters();
	}

	CrGroup* CrScene::getGroupsIndex(int groupID)
	{
		if (groupID < m_groups.size())
			return  m_groups.at(groupID);
		else
			return nullptr;
	}
}