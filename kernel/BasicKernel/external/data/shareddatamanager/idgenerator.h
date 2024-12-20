#ifndef CREATIVE_KERNEL_IDGENERATOR_1681019989200_H
#define CREATIVE_KERNEL_IDGENERATOR_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include <unordered_map>
#include <QMap>

namespace creative_kernel
{
	class MeshDataWrapper;
	class SpreadDataWrapper;
	class BASIC_KERNEL_API IdGenerator : public QObject
	{
	public:
		IdGenerator(QObject* parent);

		// int generateNewId(const QHash<int, MeshDataWrapper*>& map);
		// int generateNewId(const QHash<int, SpreadDataWrapper*>& map);
		int generateNewId(const QString& key);

	private:
		QMap<QString, int> m_idMap;

	};

}

#endif // CREATIVE_KERNEL_IDGENERATOR_1681019989200_H