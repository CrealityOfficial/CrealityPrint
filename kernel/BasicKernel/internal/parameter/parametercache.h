#ifndef CREATIVE_KERNEL_PARAMETERCACHE_1675854796608_H
#define CREATIVE_KERNEL_PARAMETERCACHE_1675854796608_H
#include <QtCore/QStringList>
#include <QColor>

namespace creative_kernel
{
	QStringList readCacheMachineUniqueNames();
	int readCacheMachineExtruders(const QString& machineUniqueNames, std::vector<QColor>& extruderColors, std::vector<bool>& extruderPhysicals);
	void writeCacheMachineExtruder(const QString& machineUniqueNames, const QColor& extruderColor, const bool& extruderPhysical);
	void modifyCacheMachineExtruder(const QString& machineUniqueNames, const int& index, const QColor& extruderColor);
	void removeCacheMachineExtruder(const QString& machineUniqueNames);
	void removeCacheMachine(const QString& machineUniqueNames);

	int readCacheCurrentMachineIndex();
	void writeCacheCurrentMachineIndex(int index);
	QStringList readCacheSelectMaterials(const QString& machineName);
	void writeCacheSelectMaterials(const QString& machineName, const QStringList& materialNames);

	//int readCacheCurrentProfile(const QString& machineName);
	QString readCacheCurrentProfile(const QString& machineName);
	void writeCacheCurrentProfile(const QString& machineName, int index);
	void writeCacheCurrentProfile(const QString& machineName, const QString& profileName);

	QString readCacheExtruderMaterialIndex(const QString& machineName, int extruderIndex);
	void writeCacheExtruderMaterialIndex(const QString& machineName, int extruderIndex, const QString& materialName);

	QString readCacheCurrentBedType(const QString& machineName);
	void writeCacheCurrentBedType(const QString& machineName, const QString& bedType);
}

#endif // CREATIVE_KERNEL_PARAMETERCACHE_1675854796608_H