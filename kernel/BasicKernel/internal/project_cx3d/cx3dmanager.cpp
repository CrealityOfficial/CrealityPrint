#include "cx3dmanager.h"
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>
#include <QDesktopServices>

#include "cx3dimportercommand.h"
#include "menu/ccommandlist.h"
#include "cx3dautosaveproject.h"
#include "cx3dsubmenurecentproject.h"
#include "kernel/kernelui.h"

#include "cx3dcloseprojectcommand.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/uiinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "cx3dnewbuild.h"
#include <cxkernel/interface/iointerface.h>
#include <QCoreApplication>
#include <QFileInfo>
#include "cx3dexportjob.h"
#include "cx3dexportercommand.h"
#include "data/modelgroup.h"
#include "qtusercore/util/settings.h"
#include <external/interface/cloudinterface.h>
#include <internal/multi_printer/printermanager.h>
#include <external/interface/printerinterface.h>
#include <internal/project_3mf/save3mf.h>
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
    bool CX3DManager::isProjectOpened()
    {
        QString projectPath = creative_kernel::projectUI()->getProjectPath();
        if(projectPath.isEmpty()||projectPath==":/cx3d/cx3d_exporter/.cxprj")
        {
            return false;
        }
        return true;
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


    void CX3DManager::setProjectTypeIndex(int typeIndex)
    {
        qtuser_core::VersionSettings setting;
        setting.setValue("projectTypeIndex", typeIndex);
    }

    int CX3DManager::projectTypeIndex()
    {
        qtuser_core::VersionSettings setting;
        int typeIndex = setting.value("projectTypeIndex", "").toInt();

        return typeIndex;
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
        QString newPrejctPath = ":/cx3d/cx3d_exporter/.cxprj";
        QFileInfo file(newPrejctPath);
        if (file.exists())
        {
            qDebug() << "CX3DManager::createProject() file exists";
            creative_kernel::projectUI()->setProjectPath(newPrejctPath);
            creative_kernel::projectUI()->setProjectName("Untitled.cxprj");
            loadProject(newPrejctPath);
        }
    }

    void CX3DManager::accept()
    {
        QString filePath = defaultPath().toLocalFile();
        openFileLocation(filePath);
    }

    void CX3DManager::triggerAutoSave(QList<creative_kernel::ModelGroup*>  modelgroup,AutoSaveJob::SaveType type)
    {
        m_autoCommand->triggerAutoSave(modelgroup,type);
    }
    void CX3DManager::startAutoSave()
    {
        m_autoCommand->startAutoSaveJob();
    }
    void CX3DManager::stopAutoSave()
    {
        m_autoCommand->stopAutoSaveJob();
    }
    void CX3DManager::initNewProject()
    {
        creative_kernel::projectUI()->setInitProjectDirty(false);
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
    void CX3DManager::openProject(const QString& filepath, bool quiet)
    {
        if (m_importcommand)
        {
            m_importcommand->openCxprjAnd3MF(filepath, quiet);
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
        if (m_exportcommand)
        {
            m_exportcommand->saveAsDialog(getProjectName(), m_newBuild);
        }
    }

    void CX3DManager::saveAs3mf(const QString& filepath)
    {
        if (m_exportcommand)
        {
            m_quiet = true;
            m_exportcommand->saveAsDialog(filepath, m_newBuild, "Project File (*.3mf)");
        }
    }

    QString CX3DManager::getProjectName()
    {
        QString prjectName = creative_kernel::projectUI()->getProjectName();
        if (prjectName == "Untitled.cxprj")
        {
            prjectName = QFileInfo(mainModelName()).completeBaseName();
        }
        if (creative_kernel::projectUI()->availablePath() && prjectName.length() > 6)
        {
            prjectName = QFileInfo(prjectName).completeBaseName() + "_1";
        }
        return prjectName;
    }

    QUrl CX3DManager::defaultPath()
    {
        qtuser_core::VersionSettings setting;
        QString lastPath = setting.value("dialogLastPath", "").toString();

        QUrl fileUrl = QUrl::fromLocalFile(lastPath);
        return fileUrl;
    }

    void CX3DManager::setDefaultPath(const QString& filePath)
    {
        qtuser_core::VersionSettings setting;
        setting.setValue("dialogLastPath", filePath);
    }

    QUrl CX3DManager::defaultName()
    {
        int plate_id = getSelectedPrinter()->index();
        QString gcode3mfname = QString("%1_plate_%2_gcode").arg(QFileInfo(mainModelName()).completeBaseName()).arg(plate_id + 1);

        QUrl fileName = QUrl::fromLocalFile(gcode3mfname);
        return fileName;
    }

    void CX3DManager::exportSinglePlate()
    {
        int plate_id = getSelectedPrinter()->index();
        QString gcode3mfname = QString("%1_plate_%2_gcode.3mf").arg(getProjectName()).arg(plate_id + 1);
        if (m_exportcommand)
        {
            m_exportcommand->exportGcode3mf(gcode3mfname, QList<int>{ plate_id });
        }
    }

    void CX3DManager::exportSinglePlate(const QUrl& url)
    {
        qtuser_core::VersionSettings setting;
        
        int plate_id = getSelectedPrinter()->index();
        QString gcode3mfname = QString("%1_plate_%2_gcode.3mf").arg(getProjectName()).arg(plate_id + 1);
        if (m_exportcommand)
        {
            QString filePath = url.toLocalFile();
            if (QFileInfo(filePath).suffix() != "3mf")
            {
                filePath.append(".3mf");
            }
            m_exportcommand->exportGcode3mf(gcode3mfname, QList<int>{ plate_id }, filePath);

            QString lastPath = QFileInfo(filePath).path();
            qtuser_core::VersionSettings setting;
            setting.setValue("dialogLastPath", lastPath);
            cxkernel::setLastSaveFileName(lastPath);
        }
    }

    void CX3DManager::exportAllPlate()
    {
        QString gcode3mfname = QString("%1_gcode.3mf").arg(QFileInfo(mainModelName()).completeBaseName());
        m_exportcommand->exportGcode3mf(gcode3mfname, getSlicedPrinterIndice());
    }

    QString CX3DManager::saveTempGCode3mf()
    {
        QString tempProjectPath = projectUI()->getAutoProjectDirPath();
        QString gcode3mfname = QString("%1/%2.gcode.3mf").arg(tempProjectPath).arg(getProjectName());
        save_gcode_2_3mf(gcode3mfname, getSlicedPrinterIndice());
        return gcode3mfname;
    }

    QString CX3DManager::getMessageText()
    {
        return m_strMessageText;
    }

    void CX3DManager::savePostProcess(bool result, const QString& msg)
    {
        if (m_newBuild) {
            createProject();
            m_newBuild = false;
        }
        if (result && !m_quiet)
        {
            if (!msg.isEmpty())
            {
                m_strMessageText = msg;
                requestQmlDialog(this, "messageDlg");
            }
        }
        if (m_quiet)
        {
            QDesktopServices::openUrl(QUrl(QString("%1/create-model?source=100").arg(creative_kernel::GetCloudUrl()), QUrl::TolerantMode));
            m_quiet = false;
        }
    }
    void CX3DManager::cancelSave()
    {
        createProject();
    }

}
