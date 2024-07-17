#include "save3mf.h"

#include "data/modeln.h"
#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/modelinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "interface/uiinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "cxkernel/data/trimeshutils.h"
#include "qtuser3d/trimesh2/conv.h"
#include "msbase/mesh/merge.h"

#include "interface/spaceinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qtusercore/module/progressortracer.h"

#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "qtusercore/string/resourcesfinder.h"
#include <QCoreApplication>
#include <external/interface/projectinterface.h>
#include "internal/project_cx3d/cx3dsubmenurecentproject.h"

#include "buildinfo.h"
#include "common_3mf.h"

#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
    Save3MFCommand::Save3MFCommand(QObject* parent)
        :ActionCommand(parent)
    {
        //m_shortcut = "Ctrl+S";
        m_actionname = tr("3MF ") + QCoreApplication::translate("MenuCmdObj", "Project File");  //tr("Save 3MF");
        m_actionNameWithout = "Save 3MF";
        m_insertKey = "save3MF";
        m_eParentMenu = eMenuType_File;

        addUIVisualTracer(this,this);
        cxkernel::registerSaveHandler(this);
    }

    Save3MFCommand::~Save3MFCommand()
    {
    }

    void Save3MFCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Save as 3MF");
        //QList<ModelN*> selections = selectionms();
        //if (selections.size() == 0)
        //{
        //    requestQmlDialog(this, "messageNoModelDlg");
        //    return;
        //}

        cxkernel::saveFile(this, QString("%1.3mf").arg(mainModelName()));
    }

    void Save3MFCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void Save3MFCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("3MF ") + QCoreApplication::translate("MenuCmdObj", "Project File");
    }

    QString Save3MFCommand::filter()
    {
        QString _filter = QString("Project File (*.3mf)");
        return _filter;
    }
    void Save3MFCommand::handle(const QString& fileName)
    {
        QList<qtuser_core::JobPtr> jobs;
        Save3MFJob* loadJob = new Save3MFJob(this);
        loadJob->setFileName(fileName);
        jobs.push_back(qtuser_core::JobPtr(loadJob));
        cxkernel::executeJobs(jobs);
    }

    void Save3MFCommand::accept()
    {
        openFileLocation(m_fileName);
    }

    Save3MFJob::Save3MFJob(QObject* parent)
        : Job(parent)
        , m_showProgress(true)
        , m_removeProjectCache(true)
    {
    }

    Save3MFJob::~Save3MFJob()
    {
    }
    void Save3MFJob::setRemoveProjectCache(bool cache)
    {
        m_removeProjectCache = cache;
    }
    void Save3MFJob::setShowProgress(bool show)
    {
        m_showProgress = show;
    }
    void Save3MFJob::setFileName(const QString& fileName)
    {
        m_fileName = fileName;
    }

    QString Save3MFJob::name()
    {
        return "";
    }

    QString Save3MFJob::description()
    {
        return "";
    }

    QString Save3MFJob::getMessageText()
    {
        return m_strMessageText;
    }
    bool Save3MFJob::showProgress()
    {
        return m_showProgress;
    }
    void Save3MFJob::failed()
    {
        qDebug() << "";
        savePostProcess();
    }

    void Save3MFJob::successed(qtuser_core::Progressor* progressor)
    {
        m_strMessageText = Save3MFCommand::tr("Save 3mf file Finished");
        
        if (m_removeProjectCache)
        {
            creative_kernel::projectUI()->clearSecen(false);
            projectUI()->setProjectPath(m_fileName);
        }
        if (!(m_fileName.indexOf("/tmpProject/") > 0 || m_fileName.indexOf("cx3d_exporter/NewProject.cxprj") > 0))
        {
            Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_fileName);
            requestQmlDialog(this, "messageDlg");
        }

        projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_fileName));
        savePostProcess();
    }

    void Save3MFJob::work(qtuser_core::Progressor* progressor)
    {
        qtuser_core::ProgressorTracer tracer(progressor);

        //save3MF(m_fileName.toStdString(), &tracer);
        save_current_scene_2_3mf(m_fileName, &tracer);
    }

    void save_current_scene_2_3mf(const QString& file_name, ccglobal::Tracer* tracer)
    {
        common_3mf::Write3MF writer(file_name.toStdString());
        common_3mf::Scene3MF scene;

        scene.application = QString("%1 %2").arg(BUNDLE_NAME).arg(PROJECT_VERSION).toStdString();
        QList<ModelN*> models = creative_kernel::modelns();
        scene.groups.resize(1);
        scene.group_counts = 1;
        scene.model_counts = (int)models.size();
        common_3mf::Group3MF& group = scene.groups.at(0);
        group.extruder = 1;
        group.name = "default";
        group.id = 1;
        if (scene.model_counts > 0)
        {
            group.models.resize(scene.model_counts);
            for (int i = 0; i < scene.model_counts; ++i)
            {
                common_3mf::Model3MF& model = group.models.at(i);
                ModelN* _model = models.at(i);
                model.id = i + 2;
                model.xf = trimesh::xform(qtuser_3d::qMatrix2Xform(_model->globalMatrix()));
                model.extruder = _model->data()->defaultColor + 1;
                model.mesh = _model->data()->mesh;
                model.name = _model->objectName().toStdString();
                model.subtype = "normal_part";
                model.colors = _model->data()->colors;
                model.seams = _model->data()->seams;
                model.supports = _model->data()->supports;
                model.settings = _model->setting()->toStdMaps();
            }
        }

        
        {
            PrinterMangager* pm = getPrinterManager();
            int count = pm->printersCount();
            scene.plates.resize(count);
            for (size_t i = 0; i < count; i++)
            {
                Printer* ptr = pm->getPrinter(i);
                common_3mf::Plate3MF& plate = scene.plates.at(i);
                plate.plateId = ptr->index();
                plate.locked = ptr->lock();
                plate.name = ptr->name().toLocal8Bit().data();
                plate.settings = ptr->stdSettings();
            }
        }
       
        qtuser_core::getOrCreateAppDataLocation("tmpProject");
        QString projectConfigFile = qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/project_settings.config";
        getKernel()->parameterManager()->saveCurrentProjectSettings(projectConfigFile);
        scene.project_settings = projectConfigFile.toLocal8Bit().data();

        writer.write_scene_2_3mf(scene, tracer);
    }
}
