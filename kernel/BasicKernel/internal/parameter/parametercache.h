#ifndef CREATIVE_KERNEL_PARAMETERCACHE_1675854796608_H
#define CREATIVE_KERNEL_PARAMETERCACHE_1675854796608_H
#include <QtCore/QStringList>

namespace creative_kernel
{
	QStringList readCacheMachineUniqueNames();
	void removeCacheMachine(const QString& machineUniqueNames);

	int readCacheCurrentMachineIndex();
	void writeCacheCurrentMachineIndex(int index);

	//int readCacheCurrentProfile(const QString& machineName);
	QString readCacheCurrentProfile(const QString& machineName);
	void writeCacheCurrentProfile(const QString& machineName, int index);
	void writeCacheCurrentProfile(const QString& machineName, const QString& profileName);

	QStringList readCacheSelectMaterials(const QString& machineName, const int& index);
	void writeCacheSelectMaterials(const QString& machineName, const QStringList& materials, const int& index);
	bool removeMachineMaterials(const QString& machineName, const QStringList& materials, const int& index);
	bool reNameMachineMaterial(const QString& machineName, const QString& oldMaterialName, const QString& newMaterialName, const int& extruderIndex);

	QString readCacheExtruderMaterialIndex(const QString& machineName, int extruderIndex);
	void writeCacheExtruderMaterialIndex(const QString& machineName, int extruderIndex, const QString& materialName);
}

#endif // CREATIVE_KERNEL_PARAMETERCACHE_1675854796608_H