#include "idgenerator.h"
#include "meshdatawrapper.h"
#include "spreaddatawrapper.h"

namespace creative_kernel
{
	IdGenerator::IdGenerator(QObject* parent) : QObject(parent)
	{

	}

	// int IdGenerator::generateNewId(const QHash<int, MeshDataWrapper*>& map)
	// {
	// 	int id = 0;
	// 	while (map.find(id) != map.cend())
	// 	{
	// 		id++;
	// 	}

	// 	return id;
	// }

	// int IdGenerator::generateNewId(const QHash<int, SpreadDataWrapper*>& map)
	// {
	// 	int id = 0;
	// 	while (map.find(id) != map.cend())
	// 	{
	// 		id++;
	// 	}

	// 	return id;
	// }
	
	int IdGenerator::generateNewId(const QString& key)
	{
		if (m_idMap.contains(key))
		{
			m_idMap[key] = m_idMap[key] + 1;
		}
		else 
		{
			m_idMap[key] = 0;
		}
		return m_idMap[key];
	}

}