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
		pfpt_default_base,
		pfpt_default_keys,
		pfpt_default_ui,
		pfpt_default_parampack,
		pfpt_machines,
		pfpt_extruders,
		pfpt_profiles,
		pfpt_materials,
		pfpt_user_root,
		pfpt_user_parampack,
		pfpt_max_num
	};

	QString pathFromPFPT(ParameterFilePathType type);
	
	void loadDefaultKeys();
	QStringList getMetaMachineKeys();
	QStringList getMetaExtruderKeys();
	QStringList getMetaProfileKeys();
	QStringList getMetaMaterialKeys();
	void loadMachineMeta(QList<MachineData>& metas, const bool& user = false);
	void saveMachineMetaUser(const MachineData& meta);
	void removeMachineMetaUser(const QString& machineName);
	void loadMaterialMeta(QList<MaterialData>& metas, const bool& user = false, const QString& machineName = QString());
	void removeUserMaterialFromJson(const QString& materialName, const QString& machineName = QString());
	void reNameUserMaterialFromJson(const QString& materialName, const QString& newMaterialName, const QString& machineName = QString());
	void saveMateriaMeta(const MaterialData& meta, const QString& machineName = QString());
	void loadOfficialMachineNames(QStringList& names);
	void loadUserMachineNames(QStringList& names);

	us::USettings* createDefaultGlobal();
	us::USettings* createSettings(const QString& fileName, const QStringList& defaultKeys = QStringList());
	us::USettings* createDefaultMachineSettings(const QString& baseMachineName);

	void makeSureUserParamPackDir(const QString& machineUniqueName);
	void removeSubDirectory(const QString& folder, const QString& subFolder);
	QStringList fetchFileNames(const QString& directory, bool noExt = false);

	void createMachineSettings(const QString& uniqueName, us::USettings* uSettings, QList<us::USettings*>* extruderSettings, bool isUser = false, MachineData* data = nullptr);
	us::USettings* createMachineCoverSettings(const QString& uniqueName);
	QString defaultMachineFile(const QString& baseMachineName, bool isUser = false);
	QString machineCoverFile(const QString& uniqueName);
	void removeUserMachineFile(const QString& uniqueName);

	QString userExtruderFile(const QString& machineName, int index);
	//�û��½����ߵ�����͵�ʱ�����
	void removeUserExtuderFile(const QString& machineName, int index);
	us::USettings* createUserExtruderSettings(const QString& machineName, int index);
	us::USettings* createUserExtruderSettingsFromUser(const QString& machineName, int index = 0, const QString & sourceMachineName = QString());

	void copyFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover = false);
	void copyPrinterFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover = false);
	void copyMaterialFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover = false);
	QString defaultProfileFile(const QString& machineName, const QString& profile);
	QString defaultProfileCoverFile(const QString& machineName, const QString& profile, const QString& materialName);
	QString userProfileFile(const QString& machineName, const QString& profile, const QString& materialName = "");
	QString userProfileCoverFile(const QString& machineName, const QString& profile, const QString& materialName = "");
	QStringList fetchOfficialProfileNames(const QString& machineName);
	QStringList fetchUserProfileNames(const QString& machineName);
	void removeUserProfileFile(const QString& machineName, const QString& fileName, const QString& materialName = "");
	us::USettings* createDefaultProfileSettings(const QString& machineName, const QString& profileName);
	us::USettings* createDefaultProfileCoverSettings(const QString& machineName, const QString& profileName, const QString& materialName = "");
	us::USettings* createUserProfileSettings(const QString& machineName, const QString& profileName);
	us::USettings* createUserProfileCoverSettings(const QString& machineName, const QString& profileName, const QString& materialName = "");

	QString defaultMaterialFile(const QString& uniqueName, const QString& materialName, bool user = false);
	QString materialCoverFile(const QString& uniqueName, const QString& materialName);
	QString userExtruderOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName);
	QString defaultExtruderOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName);
	QString userProcessOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName, const QString& process);
	QString defaultProcessOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName, const QString& process);

	us::USettings* createMaterialSettings(const QString& uniqueName, const MaterialData& data, bool user = false);
	us::USettings* createMaterialCoverSettings(const QString& uniqueName, const MaterialData& data);

	us::USettings* createProcessOverrideSettings(const QString& uniqueName, const MaterialData& data, const QString& quality);
	us::USettings* createExtruderOverrideSettings(const QString& machineUniqueName, const QString& materialUniqueName);
}

#define _pfpt_def(x) creative_kernel::pathFromPFPT(creative_kernel::ParameterFilePathType::x)
#define _pfpt_default_root _pfpt_def(pfpt_default_root)
#define _pfpt_default_base _pfpt_def(pfpt_default_base)
#define _pfpt_default_keys _pfpt_def(pfpt_default_keys)
#define _pfpt_default_ui _pfpt_def(pfpt_default_ui)
#define _pfpt_default_parampack _pfpt_def(pfpt_default_parampack)
#define _pfpt_user_root _pfpt_def(pfpt_user_root)
#define _pfpt_user_parampack _pfpt_def(pfpt_user_parampack)

#define _qr_default ":/default/sliceconfig/default"
#endif // CREATIVE_KERNEL_PARAMETERPATH_1675843805348_H