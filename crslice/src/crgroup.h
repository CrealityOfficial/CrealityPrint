#ifndef CRSLICE_CRGROUP_1669515380929_H
#define CRSLICE_CRGROUP_1669515380929_H
#include "crobject.h"
#include <vector>
#include <fstream>

namespace crslice
{
	class CrGroup
	{
	public:
		CrGroup();
		~CrGroup();

		int addObject();
		void setObjectMesh(int objectID, TriMeshPtr mesh);
		void setObjectSettings(int objectID, SettingsPtr settings);
		void setSettings(SettingsPtr settings);
		void setOffset(trimesh::vec3 offset);

		void load(std::ifstream& in);
		void save(std::ofstream& out);

		void load(std::fstream& in, int version);   //for version
		void save(std::fstream& out, int version);  //for version

		std::vector<CrObject> m_objects;
		SettingsPtr m_settings;
		trimesh::vec3 m_offset;
	};
}

#endif // CRSLICE_CRGROUP_1669515380929_H