#include "cx3dautosaveproject.h"
#include "cx3dexportjob.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/uiinterface.h"
#include "interface/modelinterface.h"
#include "interface/spaceinterface.h"
#include "interface/projectinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/visualsceneinterface.h"


using namespace creative_kernel;

Cx3dAutoSaveProject::Cx3dAutoSaveProject(QObject* parent)
    :QObject(parent)
{    
    tmp_timer = new QTimer(this);
}

Cx3dAutoSaveProject::~Cx3dAutoSaveProject()
{
}

void Cx3dAutoSaveProject::startAutoSave()
{
    tmp_timer->start(60000);
    connect(tmp_timer, SIGNAL(timeout()), this, SLOT(updateTmpTime()));

    if (QFile::exists(projectUI()->getAutoProjectPath()))
    {
        requestQmlDialog(this, "openDefaultCx3d");
    }
}

void Cx3dAutoSaveProject::reject()
{
}

void Cx3dAutoSaveProject::accept()
{
    loadProject(projectUI()->getAutoProjectPath());
}

void Cx3dAutoSaveProject::updateTmpTime()
{
    saveProject(projectUI()->getAutoProjectPath(), false, NULL, false);
}

void Cx3dAutoSaveProject::onModelAdded(creative_kernel::ModelN* model)
{
    saveProject(projectUI()->getAutoProjectPath(), false, NULL, true);
}

void Cx3dAutoSaveProject::onModelRemoved(creative_kernel::ModelN* model)
{
    saveProject(projectUI()->getAutoProjectPath(), false, NULL, true);
}
