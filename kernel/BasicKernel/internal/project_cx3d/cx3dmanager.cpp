#include "cx3dmanager.h"
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>

#include "cx3dimportercommand.h"
#include "menu/ccommandlist.h"
#include "cx3dautosaveproject.h"
#include "cx3dsubmenurecentproject.h"
#include "kernel/kernelui.h"

#include "cx3dcloseprojectcommand.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "cx3dnewbuild.h"
#include <cxkernel/interface/iointerface.h>
#include <QCoreApplication>
#include <QFileInfo>
#include "cx3dexportjob.h"
#include "cx3dexportercommand.h"
namespace creative_kernel
{
    CX3DManager::CX3DManager(QObject* parent)
        :QObject(parent)
        , m_exportcommand(nullptr)
        , m_importcommand(nullptr)
        , m_projectNeedSave(true)
    {
    }

    CX3DManager::~CX3DManager()
    {
    }

    void CX3DManager::initialize()
    {
        Cx3dSubmenuRecentProject* rectfile = Cx3dSubmenuRecentProject::createInstance(this);
        rectfile->setParent(this);
        CCommandList::getInstance()->addActionCommad(rectfile, "subMenuFile");
        //
        rectfile->setNumOfRecentFiles(10);
        registerContextObject("projectSubmodel", rectfile);
        m_autoCommand = new Cx3dAutoSaveProject(this); 
        registerContextObject("autoSaveProject", m_autoCommand);


        m_exportcommand = new CX3DExporterCommand(this);
        m_exportcommand->setParent(this);
        CCommandList::getInstance()->addActionCommad(m_exportcommand);
        m_importcommand = new CX3DImporterCommand();
        m_importcommand->setParent(this);
        CCommandList::getInstance()->addActionCommad(m_importcommand);

        Cx3dCloseProjectCommand* closePro = new Cx3dCloseProjectCommand();
        closePro->setParent(this);
        CCommandList::getInstance()->addActionCommad(closePro);

        creative_kernel::setCloseHook(closePro);
    }

    void CX3DManager::uninitialize()
    {
        CCommandList::getInstance()->removeActionCommand(m_exportcommand);
        CCommandList::getInstance()->removeActionCommand(m_importcommand);
    }

    void CX3DManager::setOpenedProjectPath(const QUrl& url)
    {
        m_openedUrl = url;
    }

    QUrl CX3DManager::openedPath()
    {
        return m_openedUrl;
    }
    void CX3DManager::createProject()
    {
        //m_strTempFilePath = qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/default.cxprj";
        QString newPrejctPath = ":/cx3d/cx3d_exporter/NewProject.cxprj";
        QFileInfo file(newPrejctPath);
        if (file.exists())
        {
            qDebug() << "CX3DManager::createProject() file exists";
            creative_kernel::projectUI()->setProjectPath(newPrejctPath);
            loadProject(newPrejctPath);
        }
    }

    void CX3DManager::triggerAutoSave()
    {
        m_autoCommand->triggerAutoSave();
    }

    void CX3DManager::initNewProject()
    {
        if (!QFile::exists(projectUI()->getAutoProjectPath()))
        {
            createProject();
        }
    }

    void CX3DManager::newCX3D()
    {
        //qDebug() << "CX3DManager::newCX3D()";
        //CX3DNewBuild newBuild;
        //cxkernel::saveFile(&newBuild, "NewProject.cxprj", QCoreApplication::translate("CustomObject", "New Project"));

    }
    void CX3DManager::openProject(const QString& filepath)
    {
        if (m_importcommand)
        {
            m_importcommand->openCxprjAnd3MF(filepath);
        }
    }
    void CX3DManager::saveCX3D(bool newBuild)
    {
        m_newBuild = newBuild;
        m_projectNeedSave = true;
        if (creative_kernel::projectUI()->availablePath())
        {
            QString projectPath = creative_kernel::projectUI()->getProjectPath();
            saveProject(projectPath, true, this, true, m_newBuild);
        }
        else
        {
            saveAsCX3D(m_newBuild);
        }
        
    }
    void CX3DManager::saveAsCX3D(bool newBuild)
    {
        m_newBuild = newBuild;
        m_projectNeedSave = true;
        QString saveName = "NewProject.cxprj";
        QString prjectName = creative_kernel::projectUI()->getProjectName();
        if (creative_kernel::projectUI()->availablePath() && prjectName.length() > 6)
        {
            saveName = prjectName.left(prjectName.length() - 6) + "_1";
        }
        if (m_exportcommand)
        {
            m_exportcommand->saveAsDialog(saveName, m_newBuild);
        }
    }
    void CX3DManager::savePostProcess()
    {
        if (m_newBuild)
            createProject();
        m_newBuild = false;
    }
    void CX3DManager::cancelSave()
    {
        createProject();
    }

}
