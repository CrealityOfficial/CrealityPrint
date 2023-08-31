#ifndef CREATIVE_KERNEL_PARAMETERPATH_1675843805348_H
#define CREATIVE_KERNEL_PARAMETERPATH_1675843805348_H
#include <QtCore/QString>
#include <QtCore/QDebug>
#include "us/usettings.h"
#include "data/header.h"

namespace creative_kernel
{
	enum class ParameterFilePathType
	{
		pfpt_default_root,
		pfpt_default_machines,
		pfpt_default_extruders,
		pfpt_default_profiles,
		pfpt_default_materials,
		pfpt_default_machine_support_materials,
		pfpt_machines,
		pfpt_extruders,
		pfpt_profiles,
		pfpt_materials,
		pfpt_machine_support_materials,
		pfpt_max_num
	};

	QString pathFromPFPT(ParameterFilePathType type);
	
	void loadDefaultKeys();
	QStringList loadCategoryKeys();
	QStringList loadSingleCategoryKeys();
	QStringList loadMaterialKeys();
	QStringList loadMaterialKeys(const QString& category);
	QStringList loadExtruderKeys(const QString& category);
	QStringList loadMachineKeys(const QString& category);
	void loadMachineMeta(QList<MachineMeta>& metas, const bool& user = false);
	void saveMachineMeta(const MachineMeta& meta);
	void saveMachineMetaUser(const MachineMeta& meta);
	void removeMachineMetaUser(const QString& machineName);
	void loadMaterialMeta(QList<MaterialMeta>& metas, const bool& user = false, const QString& machineName = QString());
	void removeUserMaterialFromJson(const QString& materialName, const QString& machineName = QString());
	void reNameUserMaterialFromJson(const QString& materialName, const QString& newMaterialName, const QString& machineName = QString());
	void saveMateriaMeta(const MaterialMeta& meta, const QString& machineName = QString());

	us::USettings* createDefaultGlobal();
	us::USettings* createSettings(const QString& fileName, const QStringList& defaultKeys = QStringList());
	us::USettings* createDefaultMachineSettings(const QString& baseMachineName);

	void makeSureDirectory(const QString& folder, const QString& subFolder);
	void removeSubDirectory(const QString& folder, const QString& subFolder);
	QStringList fetchFileNames(const QString& directory);

	us::USettings* createMachineSettings(const QString& uniqueName);
	us::USettings* createMachineSettingsImport(const QString& uniqueName);
	us::USettings* createUserMachineSettings(const QString& uniqueName);
	QString defaultMachineFile(const QString& baseMachineName);
	QString userMachineFile(const QString& uniqueName, bool cover = false);
	QString uniqueName2BaseMachineName(const QString& uniqueName);
	QString uniqueName2ExtruderSize(const QString& uniqueName, int index=0);
	void makeSureUserMachineFile(const QString& uniqueName, const QString& dstName = QString(), bool isSourceMachineUser = false);
	void removeUserMachineFile(const QString& uniqueName);
	void coverUserMachineFile(const QString& uniqueName);

	QString defaultExtruderFile(const QString& machineName,int index);
	QString userExtruderFile(const QString& machineName, int index, bool cover = false);
	void makeSureUserExtruderFile(const QString& machineName, int index, const QString& sourceMachineName = QString(),bool isSourceMachineUser = false);
	//用户新建或者导入机型的时候调用
	void makeSureUserExtruderFileFromUser(const QString& sourceMachineName, const QString& dstMachineName, int index);
	void removeUserExtuderFile(const QString& machineName, int index);
	void coverUserExtuderFile(const QString& machineName, int index);
	us::USettings* createExtruderSettings(const QString& machineName, int index, bool defaultPath = false);
	us::USettings* createExtruderSettingsFromUser(const QString& machineName, int index, const QString& sourceMachineName = QString(), bool defaultPath = false);
	us::USettings* createUserExtruderSettings(const QString& machineName, int index);
	us::USettings* createUserExtruderSettingsFromUser(const QString& machineName, int index = 0, const QString & sourceMachineName = QString());

	void copyFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover = false);
	void copyPrinterFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover = false);
	void copyMaterialFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover = false);
	QString defaultProfileFile(const QString& machineName, const QString& profile);
	QString userProfileFile(const QString& machineName, const QString& profile, bool cover = false, const QString& materialName = "");	
	void makeSureUserProfileFiles(const QString& machineName);
	void makeSureUserProfileFiles_(const QString& machineName, const QString& dstMachineName = QString());
	void makeUserDefProfileFile(const QString& machineName, const QString& profileName, const QString& copyProfile);
	QStringList fetchUserProfileNames(const QString& machineName, const QString& dstMachineName = QString());
	void removeUserProfileFile(const QString& machineName, const QString& fileName, const QString& materialName = "");
	us::USettings* createUserProfileSettings(const QString& machineName, const QString& profileName);
	us::USettings* createUserProfileCoverSettings(const QString& machineName, const QString& profileName, const QString& materialName = "");

	QString defaultMachineSupportMaterialFile(const QString& machineName);
	QString userMachineSupportMaterialFile(const QString& machineName);
	void makeSureUserMachineSupportMaterialFile(const QString& machineName);
	void removeUserMachineSupportMaterialFile(const QString& machineName);
	void coverUserMachineSupportMaterialFile(const QString& machineName);
	QStringList fetchUserMachineSupportMaterialNames(const QString& machineName);

	QString userMaterialFile(const QString& uniqueName, const QString& materialName, bool cover = false);
	QString defaultMaterialFile(const QString& materialName);
	void makeSureUserMaterialFiles(const QString& uniqueName);
	QStringList fetchUserMaterialNames(const QString& uniqueName);
	void makeSureUserMaterialFile(const QString& uniqueName, const MaterialData& data);
	us::USettings* createMaterialSettings(const QString& uniqueName, const MaterialData& data, bool cover = false);
	us::USettings* createUserMaterialSettings(const QString& uniqueName, const MaterialData& data);
	us::USettings* createMaterialOverrideSettings(const QString& uniqueName, const MaterialData& data, const QString& quality);
}

#define _pfpt_def(x) creative_kernel::pathFromPFPT(creative_kernel::ParameterFilePathType::x)
#define _pfpt_default_root _pfpt_def(pfpt_default_root)
#define _pfpt_default_machines _pfpt_def(pfpt_default_machines)
#define _pfpt_default_extruders _pfpt_def(pfpt_default_extruders)
#define _pfpt_default_profiles _pfpt_def(pfpt_default_profiles)
#define _pfpt_default_materials _pfpt_def(pfpt_default_materials)
#define _pfpt_default_machine_support_materials _pfpt_def(pfpt_default_machine_support_materials)
#define _pfpt_machines _pfpt_def(pfpt_machines)
#define _pfpt_extruders _pfpt_def(pfpt_extruders)
#define _pfpt_profiles _pfpt_def(pfpt_profiles)
#define _pfpt_materials _pfpt_def(pfpt_materials)
#define _pfpt_machine_support_materials _pfpt_def(pfpt_machine_support_materials)

#define _qr_default ":/default/sliceconfig/default"
#endif // CREATIVE_KERNEL_PARAMETERPATH_1675843805348_H