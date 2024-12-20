#include "save3mf.h"

#include "data/modeln.h"
#include "kernel/kernelui.h"
#include "cxkernel/interface/jobsinterface.h"
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
#include <QFileInfo>
#include <QDateTime>
#include <external/interface/projectinterface.h>
#include "internal/project_cx3d/cx3dsubmenurecentproject.h"

#include "buildinfo.h"
#include "common_3mf.h"

#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/multi_printer/printer.h"
#include "cxkernel/utils/utils.h"
#include "cxkernel/interface/constinterface.h"
#include <boost/filesystem.hpp>
#include <QCryptographicHash>
#include "operation/fontmesh/fontmeshwrapper.h"
#include "internal/project_cx3d/cx3dmanager.h"
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
        savePostProcess(false);
    }

    void Save3MFJob::successed(qtuser_core::Progressor* progressor)
    {
        QString suffix = QFileInfo(m_fileName).suffix();
        m_strMessageText = Save3MFCommand::tr("Save %1 file Finished").arg(suffix);
        
        if (m_removeProjectCache)
        {
            creative_kernel::projectUI()->clearSecen(false);
            creative_kernel::projectUI()->deleteTempProject();
            projectUI()->setProjectPath(m_fileName);
        }
        if (m_showProgress)
        {
            Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_fileName);
            setDefaultPath(m_fileName);
            bool isNewBuild = getKernel()->cx3dManager()->isNewBuild();
            if(!isNewBuild)
            {
                projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_fileName));
            }
            savePostProcess(true, m_strMessageText);
            
            return;
        }

        savePostProcess(true);
    }

    void Save3MFJob::work(qtuser_core::Progressor* progressor)
    {
        qtuser_core::ProgressorTracer tracer(progressor);

        //save3MF(m_fileName.toStdString(), &tracer);
        save_current_scene_2_3mf(m_fileName, &tracer);
    }

    std::string getSubTypeByModelNType(creative_kernel::ModelNType mType)
    {
        if (ModelNType::normal_part == mType)
            return "normal_part";
        if (ModelNType::negative_part == mType)
            return "negative_part";
        if (ModelNType::modifier == mType)
            return "modifier_part";
        if (ModelNType::support_defender == mType)
            return "support_blocker";
        if (ModelNType::support_generator == mType)
            return "support_enforcer";

        return "normal_part";
    }
    void save_current_scene_2_3mf_without_group(const QString& file_name, ccglobal::Tracer* tracer )
    {
        save_scene_2_3mf(file_name,QList<ModelGroup*>(),tracer,false);
    }
	void save_current_scene_2_3mf_with_group(const QString& file_name, QList<ModelGroup*> mglist,ccglobal::Tracer* tracer )
    {
        save_scene_2_3mf(file_name,mglist,tracer,false);
    }
    void save_gcode_2_3mf(const QString& file_name, QList<int> plate_ids, bool save_scene)
    {
        QString tempProjectPath = projectUI()->getAutoProjectDirPath();
        QString file_path = file_name;
        common_3mf::Write3MF writer(file_path.toStdString());
        common_3mf::Scene3MF scene;

        scene.application = QString("%1 %2").arg(BUNDLE_NAME).arg(PROJECT_VERSION).toStdString();

        QList<ModelN*> models = creative_kernel::modelns();
        QList<ModelGroup*> mGroups = creative_kernel::modelGroups();
        if (mGroups.size() > 0 && save_scene)
        {
            scene.group_counts = mGroups.count();
            scene.model_counts = models.count();

            for (int i = 0; i < mGroups.size(); i++)
            {
                ModelGroup* mg = mGroups.at(i);
                if (!mg)
                    continue;

                common_3mf::Group3MF group3mf;
                group3mf.name = cxkernel::qString2String(mg->groupName());
                group3mf.id = mg->getObjectId();
                group3mf.extruder = mg->defaultColorIndex() + 1;
                group3mf.xf = mg->getMatrix();
                group3mf.settings = mg->setting()->toStdMaps();
                group3mf.skip = false;

                for (int k = 0; k < mg->models().size(); k++)
                {
                    ModelN* aModel = mg->models().at(k);
                    if (!aModel)
                        continue;

                    common_3mf::Model3MF model3mf;
                    model3mf.name = cxkernel::qString2String(aModel->name());
                    model3mf.id = aModel->getObjectId();
                    model3mf.xf = aModel->getMatrix();
                    model3mf.extruder = aModel->materialID() + 1;

                    model3mf.backup_meshid = aModel->sharedDataID().renderDataId.meshId;
                    model3mf.backup_colorid = aModel->sharedDataID().renderDataId.colorsId;
                    model3mf.backup_seamid = aModel->sharedDataID().seamsId;
                    model3mf.backup_supportid = aModel->sharedDataID().supportsId;
                    model3mf.subtype = getSubTypeByModelNType(aModel->modelNType());
                    model3mf.mesh = aModel->data()->mesh;
                    model3mf.colors = aModel->getColors2Facets();
                    model3mf.seams = aModel->getSeam2Facets();
                    model3mf.supports = aModel->getSupport2Facets();
                    model3mf.settings = aModel->setting()->toStdMaps();

                    group3mf.models.push_back(model3mf);
                }

                scene.groups.push_back(group3mf);
            }
        }

        PrinterMangager* pm = getPrinterManager();
        int count = pm->printersCount();
        scene.plates.resize(count);
        for (const auto& i : plate_ids)
        {
            Printer* ptr = pm->getPrinter(i);
            if (!ptr)
            {
                continue;
            }
            common_3mf::Plate3MF& plate = scene.plates.at(i);
            plate.plateId = ptr->index() + 1;
            plate.locked = ptr->lock();
            plate.name = cxkernel::qString2String(ptr->name());
            plate.settings = ptr->stdSettings();
            for (int k = 0; k < ptr->modelGroupsInside().size(); k++)
            {
                int groupId = ptr->modelGroupsInside().at(k)->getObjectId();
                plate.groupIds.push_back(groupId);
            }

            QString gcode_path = QString("%1/plate_%2.gcode").arg(tempProjectPath).arg(i + 1);
            QString gcode_md5_path = QString("%1/plate_%2.gcode.md5").arg(tempProjectPath).arg(i + 1);
            if (QFile::exists(gcode_path))
            {
                scene.plate_gcodes.emplace_back(gcode_path.toLocal8Bit().data());
                QFile gcode_file(gcode_path);
                QFile gcode_md5_file(gcode_md5_path);
                if (gcode_file.open(QIODevice::ReadWrite))
                {
                    bool ret = gcode_file.setFileTime(QDateTime::currentDateTime().addSecs(-60*(count-i)), QFileDevice::FileModificationTime);
                    if (gcode_md5_file.open(QIODevice::WriteOnly))
                    {
                        auto data = QCryptographicHash::hash(gcode_file.readAll(), QCryptographicHash::Md5).toHex().toUpper();
                        gcode_md5_file.write(data);
                        gcode_md5_file.close();
                        scene.plate_gcodes.emplace_back(gcode_md5_path.toLocal8Bit().data());
                    }
                    gcode_file.close();
                }
            }
            QString thumbnail_path = QString("%1/plate_%2.png").arg(tempProjectPath).arg(i + 1);
            ptr->picture().scaled(QSize(300, 300)).save(thumbnail_path);
            scene.plate_thumbnails.emplace_back(thumbnail_path.toLocal8Bit().data());
            QString thumbnail_small_path = QString("%1/plate_%2_small.png").arg(tempProjectPath).arg(i + 1);
            ptr->picture().scaled(QSize(96, 96)).save(thumbnail_small_path);
            scene.plate_thumbnails.emplace_back(thumbnail_small_path.toLocal8Bit().data());
        }

        scene.create_print_version = cxkernel::version().toStdString();
        QString projectConfigFile = tempProjectPath + "/project_settings.config";
        getKernel()->parameterManager()->saveCurrentProjectSettings(projectConfigFile);
        scene.project_settings = projectConfigFile.toLocal8Bit().data();
        writer.write_scene_2_3mf(scene, nullptr, false);
    }

     void save_current_scene_2_3mf(const QString& file_name, ccglobal::Tracer* tracer,bool show_progress)
     {
        save_scene_2_3mf(file_name,QList<ModelGroup*>(),tracer,true);
     }
    
    void save_scene_2_3mf(const QString& file_name, QList<ModelGroup*> mglist,ccglobal::Tracer* tracer,bool show_progress)
    {
        common_3mf::Write3MF writer(file_name.toStdString());
        common_3mf::Scene3MF scene;

        scene.model_metadata = creative_kernel::modelMetaData();

        scene.application = QString("%1 %2").arg(BUNDLE_NAME).arg(PROJECT_VERSION).toStdString();
        //scene.application = QString("%1 %2").arg(BUNDLE_NAME).arg("V01.09.01.66").toStdString();
        //scene.application = "BambuStudio-01.09.01.66";
        QList<ModelN*> models = creative_kernel::modelns();
        QList<ModelGroup*> mGroups = creative_kernel::modelGroups();
        bool bSkip = mglist.size()==0?false:true;
        if (mGroups.size() > 0)
        {
            scene.group_counts = mGroups.count();
            scene.model_counts = models.count();

            for (int i = 0; i < mGroups.size(); i++)
            {
                ModelGroup* mg = mGroups.at(i);
                if (!mg)
                    continue;

                common_3mf::Group3MF group3mf;
                group3mf.name = cxkernel::qString2String(mg->groupName());
                group3mf.id = mg->getObjectId();
                group3mf.extruder = mg->defaultColorIndex() + 1;
                group3mf.xf = mg->getMatrix();
                group3mf.settings = mg->setting()->toStdMaps();
                group3mf.skip = bSkip;
                group3mf.layerHeightProfile = mg->layerHeightProfile();
                if(mglist.contains(mg))
                {
                    group3mf.skip = false;
                }

                for (int k = 0; k < mg->models().size(); k++)
                {
                    ModelN* aModel = mg->models().at(k);
                    if (!aModel)
                        continue;

                    common_3mf::Model3MF model3mf;
                    model3mf.name = cxkernel::qString2String(aModel->name());
                    model3mf.id = aModel->getObjectId();
                    model3mf.xf = aModel->getMatrix();
                    model3mf.extruder = aModel->materialID() + 1;
                    
                    model3mf.backup_meshid = aModel->sharedDataID().renderDataId.meshId;
                    model3mf.backup_colorid = aModel->sharedDataID().renderDataId.colorsId;
                    model3mf.backup_seamid = aModel->sharedDataID().seamsId;
                    model3mf.backup_supportid = aModel->sharedDataID().supportsId;
                    model3mf.subtype = getSubTypeByModelNType(aModel->modelNType());
                    if(show_progress)
                    {
                        model3mf.mesh = aModel->data()->mesh;
                        model3mf.colors = aModel->getColors2Facets();
                        model3mf.seams = aModel->getSeam2Facets();
                        model3mf.supports = aModel->getSupport2Facets();
                    }
                    model3mf.settings = aModel->setting()->toStdMaps();
                    if (aModel->isRelief())
                        model3mf.relief_description = aModel->fontMesh()->serialize();
                    

                    group3mf.models.push_back(model3mf);
                }

                scene.groups.push_back(group3mf);
            }
        }
        
        
        QString tempProjectPath = projectUI()->getAutoProjectDirPath();
            PrinterMangager* pm = getPrinterManager();
            int count = pm->printersCount();
            scene.plates.resize(count);
            for (size_t i = 0; i < count; i++)
            {
                Printer* ptr = pm->getPrinter(i);
                if (!ptr)
                {
                    continue;
                }
                common_3mf::Plate3MF& plate = scene.plates.at(i);
                plate.plateId = ptr->index()+1;
                plate.locked = ptr->lock();
                plate.name = cxkernel::qString2String(ptr->name());
                plate.settings = ptr->stdSettings();

                crslice2::PlateInfo plateInfo = ptr->getPlateInfo();
                for (crslice2::Plate_Item item : plateInfo.gcodes)
                {
                    common_3mf::PlateCustomGCode gcode;
                    gcode.print_z = item.print_z;
                    gcode.type = (common_3mf::PlateType)item.type;
                    gcode.extruder = item.extruder;
                    gcode.color = item.color;
                    gcode.extra = item.extra;
                    plate.gcodes.push_back(gcode);
                }

                for (int k = 0; k < ptr->modelGroupsInside().size(); k++)
                {
                    int groupId = ptr->modelGroupsInside().at(k)->getObjectId();
                    plate.groupIds.push_back(groupId);
                }
                QString gcode_path = QString("%1/plate_%2.gcode").arg(tempProjectPath).arg(i + 1);
                QString gcode_md5_path = QString("%1/plate_%2.gcode.md5").arg(tempProjectPath).arg(i+1);
                if (QFile::exists(gcode_path))
                {
                    scene.plate_gcodes.emplace_back(gcode_path.toLocal8Bit().data());
                    QFile gcode_file(gcode_path);
                    QFile gcode_md5_file(gcode_md5_path);
                    if (gcode_file.open(QIODevice::ReadOnly))
                    {
                        if (gcode_md5_file.open(QIODevice::WriteOnly))
                        {
                            auto data = QCryptographicHash::hash(gcode_file.readAll(), QCryptographicHash::Md5).toHex().toUpper();
                            gcode_md5_file.write(data);
                            gcode_md5_file.close();
                            scene.plate_gcodes.emplace_back(gcode_md5_path.toLocal8Bit().data());
                        }
                        gcode_file.close();
                    }
                }
                QString thumbnail_path = QString("%1/plate_%2.png").arg(tempProjectPath).arg(i+1);                
                ptr->picture().scaled(QSize(300,300)).save(thumbnail_path);
                scene.plate_thumbnails.emplace_back(thumbnail_path.toLocal8Bit().data());
                QString thumbnail_small_path = QString("%1/plate_%2_small.png").arg(tempProjectPath).arg(i + 1);
                ptr->picture().scaled(QSize(96, 96)).save(thumbnail_small_path);
                scene.plate_thumbnails.emplace_back(thumbnail_small_path.toLocal8Bit().data());
            }
        
        scene.create_print_version = cxkernel::version().toStdString();
       
        QString projectConfigFile = tempProjectPath + "/project_settings.config";
        getKernel()->parameterManager()->saveCurrentProjectSettings(projectConfigFile);
        scene.project_settings = projectConfigFile.toLocal8Bit().data();

        writer.set_backup_path(tempProjectPath.toStdString());
        writer.write_scene_2_3mf(scene, tracer,!show_progress);
    }
}
