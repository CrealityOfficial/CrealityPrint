#include "projectinterface.h"
#include "kernel/kernel.h"

#include "internal/project_cx3d/cx3dmanager.h"
#include "internal/menu/submenurecentfiles.h"
#include <internal/project_3mf/save3mf.h>

namespace creative_kernel
{
	ProjectInfoUI* projectUI()
	{
		return ProjectInfoUI::instance();
	}

	bool projectSpaceDirty()
	{
		return ProjectInfoUI::instance()->spaceDirty();
	}
	void savePostProcess(bool result, const QString& msg)
	{
		getKernel()->cx3dManager()->savePostProcess(result, msg);
	}

	void setDefaultPath(const QString& filePath)
	{
		getKernel()->cx3dManager()->setDefaultPath(filePath);
	}

	void setMenuMostRecentFile(const QString& file_name)
	{
		SubMenuRecentFiles::getInstance()->setMostRecentFile(file_name);
	}

	void triggerAutoSave(QList<creative_kernel::ModelGroup*> modelgroups,int type)
	{
		getKernel()->cx3dManager()->triggerAutoSave(modelgroups,(AutoSaveJob::SaveType)type);
	}

	QString projectName()
	{
		return ProjectInfoUI::instance()->getProjectName();
	}
	QString projectNameNoSuffix()
	{
		return ProjectInfoUI::instance()->getProjectNameNoSuffix();
	}
	QString projectPath()
	{
		return ProjectInfoUI::instance()->getProjectPath();
	}

	void saveGcode3mf(const QString& file_name, QList<int> plate_ids)
	{
		save_gcode_2_3mf(file_name, plate_ids);
	}
}