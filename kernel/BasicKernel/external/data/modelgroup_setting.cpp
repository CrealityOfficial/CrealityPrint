#include "modelgroup.h"
#include "internal/parameter/parametermanager.h"
#include "external/kernel/kernel.h"
#include "interface/displaywarninginterface.h"
#include "internal/project_cx3d/cx3dmanager.h"
namespace creative_kernel
{
	void ModelGroup::onGlobalSettingsChanged(QObject* parameter_base, const QString& key)
	{
		if (key == "enable_support")
		{
			emit supportEnabledChanged();
		}
		else if (key == "support_threshold_angle")
		{
			emit supportAngleChanged();
		}
		else if (key == "support_type")
		{
			emit supportStructureChanged();
		}
	}

	void ModelGroup::onSettingsChanged(const QString& key, us::USetting* setting)
	{
		if (key == "enable_support")
		{
			checkGroupHigherThanBottom();
		}

		if (!checkSettingDirty(key))
			return;

		QList<creative_kernel::ModelGroup *> grouplist;
		grouplist.append(this);
		getKernel()->cx3dManager()->triggerAutoSave(grouplist,AutoSaveJob::MODELSETTINGS);

		dirty();
		emit settingsChanged();

		if (key == "enable_support")
		{
			supportEnabled();
			emit supportEnabledChanged();
		}
		else if (key == "support_threshold_angle")
		{
			supportAngle();
			emit supportAngleChanged();
		}
		else if (key == "support_type")
		{
			supportStructure();
			emit supportStructureChanged();
		}
		else if (key == "support_filament")
		{
			supportFilament();
			emit supportSupportFilamentChanged();
		}
		else if (key == "support_interface_filament")
		{
			supportInterfaceFilament();
			emit supportInterfaceFilamentChanged();
		}
	}

	bool ModelGroup::checkSupportSettingState()
	{
		bool objectHasSupportSetting = false;

		objectHasSupportSetting = cSupportEnabled();
		if (!objectHasSupportSetting)
		{
			return getKernel()->parameterManager()->enableSupport();
		}

		return objectHasSupportSetting;

	}
}