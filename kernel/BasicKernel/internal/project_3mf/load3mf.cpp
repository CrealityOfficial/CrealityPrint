#include "load3mf.h"

#include "cxkernel/interface/iointerface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/spaceinterface.h"
#include "interface/printerinterface.h"
#include "interface/commandinterface.h"
#include "interface/machineinterface.h"
#include "interface/projectinterface.h"
#include "interface/shareddatainterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"

#include <QtWidgets/QFileDialog>
#include <QtCore/QCoreApplication>

#include "cxkernel/utils/utils.h"
#include "qtusercore/module/progressortracer.h"
#include "qtusercore/util/settings.h"
#include "qtusercore/string/resourcesfinder.h"

#include "kernel/kernelui.h"
#include "kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/multi_printer/printersettings.h"
#include "internal/project_cx3d/cx3dsubmenurecentproject.h"
#include "internal/multi_printer/printermanager.h"
#include "external/unittest/unittestflow.h"

#include "operation/fontmesh/fontmeshwrapper.h"

namespace creative_kernel
{
    Load3MFCommand::Load3MFCommand(QObject* parent)
        :ActionCommand(parent)
    {
        //m_shortcut = "Ctrl+S";
        m_actionname = tr("3MF ") + QCoreApplication::translate("MenuCmdObj", "Project File"); // tr("Load 3MF");
        m_actionNameWithout = "Load 3MF";
        m_insertKey = "load3MF";
        m_eParentMenu = eMenuType_File;
        addUIVisualTracer(this,this);
    }

    Load3MFCommand::~Load3MFCommand()
    {
    }

    void Load3MFCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void Load3MFCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("3MF ") + QCoreApplication::translate("MenuCmdObj", "Project File");//+ "    " + m_shortcut;
    }

    void Load3MFCommand::execute()
    {
        sensorAnlyticsTrace("File Menu", "Load 3MF");
        //cxkernel::saveFile(this, QString("%1.stl").arg(mainModelName()));

        QString fileName;
        auto f = [this](const QString& file) {
            cxkernel::openFileWithName(file);
        };

        QString title = QObject::tr("OpenFile");
        QString filter = "Mesh file(*.3mf)";
        qtuser_core::VersionSettings setting;
        QString lastPath = setting.value("dialogLastPath", "").toString();
#ifdef Q_OS_LINUX
        fileName = QFileDialog::getOpenFileName(
            nullptr, title,
            lastPath, filter, nullptr, QFileDialog::DontUseNativeDialog);
#else
        fileName = QFileDialog::getOpenFileName(
            nullptr, title,
            lastPath, filter);
#endif
        if (fileName.isEmpty())
            return;
        QFileInfo fileinfo = QFileInfo(fileName);
        lastPath = fileinfo.path();
        setting.setValue("dialogLastPath", lastPath);
        setKernelPhase(KernelPhaseType::kpt_prepare);

        Load3MFJob* loadJob = new Load3MFJob(this);
        loadJob->setFileName(fileName);
        cxkernel::executeJob(loadJob);
    }

    Load3MFJob::Load3MFJob(QObject* parent)
        : Job(parent)
        , m_open_project(true)
    {
    }

    Load3MFJob::~Load3MFJob()
    {
    }

    void Load3MFJob::setFileName(const QString& fileName)
    {
        m_fileName = fileName;
    }

    void Load3MFJob::setOpenProject(bool open_project)
    {
        m_open_project = open_project;
    }

    QString Load3MFJob::name()
    {
        return "Load3MFJob";
    }

    QString Load3MFJob::description()
    {
        return "Load3MFJob";
    }

    void Load3MFJob::failed()
    {
        qDebug() << "Load3MFJob failed";
    }

    void Load3MFJob::successed(qtuser_core::Progressor* progressor)
    {
        if (!m_read3mfState)
        {
            return;
        }

        if (m_open_project)
        {
            if (ProjectInfoUI::instance()->getNameFromPath(m_fileName)==".cxprj")
            {
                projectUI()->setProjectPath("");
                //Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_fileName);
                projectUI()->setProjectName("Untitled.cxprj");
            }else{
                QString tempdir = projectUI()->getAutoProjectDirPath();
                if(m_fileName.indexOf(tempdir)>=0)
                {
                    Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(projectUI()->getProjectPath());
                    projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_fileName));
                }else{
                    projectUI()->setProjectPath(m_fileName);
                    Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_fileName);
                    projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_fileName));
                }
            }
        }
        std::vector<float> wipe_tower_x;
        std::vector<float> wipe_tower_y;
        getKernel()->parameterManager()->loadProjectSettings(m_projectConfigFile,wipe_tower_x,wipe_tower_y);
        getKernel()->onModelMaterialUpdate();

        // fix : [ID1027907]: ��3mfʱ����ɫ��ʧ���ڵ�ǰ��Ŀ���ٴ������3mf����ɫ��Ϣ����;
        // "loadProjectSettings" need to be called before "registerModelData"

        //QMap<QString, QString> kvs;
        //for (std::map<std::string, std::string>::const_iterator it = model.settings.begin();
        //    it != model.settings.end(); ++it)
        //    kvs.insert(it->first.c_str(), it->second.c_str());
        //for (std::map<std::string, std::string>::const_iterator it = group.settings.begin();
        //    it != group.settings.end(); ++it)
        //{
        //    if (!kvs.contains(it->first.c_str()))
        //        kvs.insert(it->first.c_str(), it->second.c_str());
        //}
        //
        //settings.append(kvs);

        int printer_count = m_scene.plates.size();
        if (printer_count > 0)
        {
            getPrinterManager()->resizePrinters(printer_count);
        }

        std::vector<common_3mf::Plate3MF> &plates = m_scene.plates;

        for (int i = 0; i < printer_count; ++i)
        {
            Printer* ptr = getPrinterManager()->getPrinter(i);
            if (ptr == nullptr)
            {
                continue;
            }
            PrinterSettings* printer_settings = qobject_cast<PrinterSettings*>(ptr->settingsObject());

            QMap<float, crslice2::Plate_Item> customs;
            for (const common_3mf::PlateCustomGCode& custom_gcode : m_scene.plates.at(i).gcodes)
            {
                crslice2::Plate_Item item;
                item.print_z = custom_gcode.print_z;
                item.color = custom_gcode.color;
                item.extruder = custom_gcode.extruder;
                item.type = (crslice2::Plate_Type)custom_gcode.type;
                item.extra = custom_gcode.extra;
                customs.insert(custom_gcode.print_z, item);
            }


            printer_settings->setCustomGCode(customs);
            
            if (i < plates.size())
            {
                const common_3mf::Plate3MF& info = plates.at(i);
                ptr->setName(info.name.c_str());
                ptr->setLock(info.locked);
                const std::map<std::string, std::string>& settings = info.settings;

                for (auto iter = settings.begin(); iter != settings.end(); ++iter)
                {
                    ptr->setParameter(QString(iter->first.c_str()), QString(iter->second.c_str()));
                }

                ptr->applySettings();
            }
        }

        creative_kernel::setModelMetaData(m_scene.model_metadata);

        createSceneData();

        creative_kernel::projectUI()->clearSecen(false);
        getPrinterMangager()->updateWipeTowersPositions(wipe_tower_x, wipe_tower_y);
        unselectAll();

        //
        getPrinterMangager()->updatePrinterContent();
        //set flush matrix and wipe_tower_x, wipe_tower_y
        SettingsPointer settings = createCurrentGlobalSettings();
        //float wipe_tower_x = settings->vvalue("wipe_tower_x").toFloat();
        //float wipe_tower_y = settings->vvalue("wipe_tower_y").toFloat();
        
        QString materialColors = settings->vvalue("filament_colour").toString();
        QString flush_matrix = settings->vvalue("flush_volumes_matrix").toString();
        QString flush_vector = settings->vvalue("flush_volumes_vector").toString();
        float maltip = settings->vvalue("flush_multiplier").toFloat();
        calculate_flush_matrix_and_set(materialColors, flush_matrix, flush_vector, maltip);

        if (BLCompareTestFlow::enabled())
        {
            setKernelPhase(KernelPhaseType::kpt_preview);
            return;
        }
        requestVisUpdate(true);
    }

    void Load3MFJob::work(qtuser_core::Progressor* progressor)
    {
        qtuser_core::ProgressorTracer tracer(progressor);

        common_3mf::Read3MF reader(cxkernel::qString2String(m_fileName),projectUI()->getAutoProjectDirPath().toStdString());
        m_scene_create_data.add_groups.clear();
        m_scene_create_data.remove_groups.clear();

        tracer.progress(0.3f);
        if (reader.read_all_3mf(m_scene, &tracer))
        {
            m_read3mfState = true;

			m_projectConfigFile = projectUI()->getAutoProjectDirPath() + "/project_settings.config";
			reader.extract_file(m_scene.project_settings, cxkernel::qString2String(m_projectConfigFile));
        }
        else
        {
            qWarning() << "failed to read 3mf file : " << m_fileName;
            m_read3mfState = false;
        }

        tracer.progress(1.0f);

    }

    creative_kernel::ModelNType Load3MFJob::getModelNTypeBySubType(const std::string& stype)
    {
        if ("normal_part" == stype)
            return ModelNType::normal_part;
        if ("negative_part" == stype)
            return ModelNType::negative_part;
        if ("modifier_part" == stype)
            return ModelNType::modifier;
        if ("support_blocker" == stype)
            return ModelNType::support_defender;
        if ("support_enforcer" == stype)
            return ModelNType::support_generator;

        return ModelNType::normal_part;
    }

    void Load3MFJob::createSceneData()
    {
        //int model_count_load = 0;

        for (common_3mf::Group3MF& group : m_scene.groups)
        {
            GroupCreateData group_create_data;
            group_create_data.model_group_id = -1;
            group_create_data.xf = group.xf;
            group_create_data.name = QString("%1").arg(group.name.c_str());
            group_create_data.defaultColorIndex = group.extruder-1;
            group_create_data.layerHeightProfile = group.layerHeightProfile;
            if (!group.settings.empty())
            {
                group_create_data.usettings = new us::USettings();
                for (auto it = group.settings.cbegin(); it != group.settings.cend(); ++it)
                {
                    group_create_data.usettings->add(it->first.c_str(), it->second.c_str());
                }
            }
            if(group_create_data.defaultColorIndex<0)
            {
                group_create_data.defaultColorIndex=0;
            }
            for (common_3mf::Model3MF& model : group.models)
            {
                //float start = 0.3f + (float)model_count_load * 0.7f / (float)m_scene.model_counts;
                //float end = 0.3f + (float)(model_count_load + 1) * 0.7f / (float)m_scene.model_counts;

                //tracer.resetProgressScope(start, end);

                model.mesh->need_bbox();
                trimesh::vec3 size = model.mesh->bbox.size();
                if ((model.mesh->vertices.size() < 4)
                    || (model.mesh->faces.size() <= 3))
                    //|| (size.x * size.y * size.z < 1.0f))   // fix bug:[ID1027978]: [3mf文件]导入Orca创建的3mf文件到C3D软件后，组合体的部分零件数据丢失
                    continue;

                makeSureDataValid(model.mesh, model.colors, model.seams, model.supports);
                int extruder = model.extruder > 0  ? model.extruder - 1 : 0;
                ModelDataPtr model_data = createModelData(model.mesh, model.colors, model.seams, model.supports, extruder);
                if (!model_data)
                    continue;

         
                Printer* printer = currentPrinter();
                assert(printer);

                trimesh::dbox3 base_box = printer->printerBox();
                //model_data->mesh->adaptSmallBox(base_box);

                ModelCreateData model_create_data;
                model_create_data.name = QString("%1").arg(model.name.c_str());

                trimesh::xform toCenterOffsetXf = trimesh::xform::trans(-model_data->mesh->offset);
                model_create_data.xf = model.xf * toCenterOffsetXf;
                model_create_data.dataID = registerModelData(model_data);
                model_create_data.dataID(ModelType) = int(getModelNTypeBySubType(model.subtype));
                model_create_data.reliefDescription = model.relief_description;

                if(model_create_data.name=="")
                {
                    model_create_data.name = QString("Object_%1").arg(model_create_data.dataID.renderDataId.meshId);
                }
                if(group_create_data.name=="" && group_create_data.models.empty())
                {
                    group_create_data.name = model_create_data.name;
                }

                if (!model.settings.empty())
                {
                    model_create_data.usettings = new us::USettings();
                    for (auto it = model.settings.cbegin(); it != model.settings.cend(); ++it)
                    {
                        model_create_data.usettings->add(it->first.c_str(), it->second.c_str());
                    }
                }
                group_create_data.models.append(model_create_data);
                
                //++model_count_load;
            }

            if (group_create_data.models.size() > 0)
                m_scene_create_data.add_groups.append(group_create_data);
        }

        modifyScene(m_scene_create_data);
    }

    std::vector<MeshWithName> load_meshes_from_3mf(const QString& file_name, ccglobal::Tracer* tracer)
    {
        std::vector<MeshWithName> meshes;
        common_3mf::Read3MF reader(cxkernel::qString2String(file_name));
        common_3mf::Scene3MF scene;
        if (reader.read_all_3mf(scene, tracer))
        {
            for (const common_3mf::Group3MF& group : scene.groups)
            {
                for (const common_3mf::Model3MF& model : group.models)
                {
                    trimesh::apply_xform(model.mesh.get(), group.xf * model.xf);
                    MeshWithName ameshWithName;
                    ameshWithName.mesh = model.mesh;
                    ameshWithName.name = model.name.length() > 0 ? model.name : group.name;
                    meshes.push_back(ameshWithName);
                }
            }
        }
 
        return meshes;
    }
}
