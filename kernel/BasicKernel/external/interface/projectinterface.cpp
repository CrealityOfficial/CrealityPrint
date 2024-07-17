#include "projectinterface.h"
#include "kernel/kernel.h"
#include "internal/project_cx3d/cx3dmanager.h"

namespace creative_kernel
{
	ProjectInfoUI* projectUI()
	{
		return ProjectInfoUI::instance();
	}
	void dirtyProjectSpace()
	{
		ProjectInfoUI::instance()->setSpaceDirty(true);
	}

	void clearProjectSpaceDrity()
	{
		ProjectInfoUI::instance()->setSpaceDirty(false);
	}

	bool projectSpaceDirty()
	{
		return ProjectInfoUI::instance()->spaceDirty();
	}
	void savePostProcess()
	{
		getKernel()->cx3dManager()->savePostProcess();
	}
	void triggerAutoSave()
	{
		getKernel()->cx3dManager()->triggerAutoSave();
	}

	QString projectName()
	{
		return ProjectInfoUI::instance()->getProjectName();
	}
	QString projectPath()
	{
		return ProjectInfoUI::instance()->getProjectPath();
	}
}