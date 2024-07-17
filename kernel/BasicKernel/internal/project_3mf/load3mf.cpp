#include "load3mf.h"

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
#include <QtWidgets/QFileDialog>
#include <QtMath>
#include "interface/spaceinterface.h"

#include "cxkernel/utils/utils.h"
#include "common_3mf.h"

#include "cxkernel/interface/modelninterface.h"
#include "cxkernel/data/trimeshutils.h"

#include "cxkernel/utils/modelfrommeshjob.h"
#include "cxkernel/interface/jobsinterface.h"

#include "interface/machineinterface.h"
#include "msbase/mesh/deserializecolor.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qtusercore/module/progressortracer.h"
#include <qtusercore/util/settings.h>
#include "external/kernel/kernel.h"
#include "internal/parameter/parametermanager.h"
#include "qtusercore/string/resourcesfinder.h"
#include <QCoreApplication>
#include <external/interface/projectinterface.h>
#include "internal/project_cx3d/cx3dsubmenurecentproject.h"
#include "interface/printerinterface.h"
#include "internal/multi_printer/printermanager.h"
#include "external/kernel/kernel.h"
#include "external/unittest/unittestflow.h"
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

    QString Load3MFCommand::filter()
    {
        QString _filter = QString("Mesh File (*.3mf)");
        return _filter;
    }

    void updateColor(trimesh::TriMesh* mesh,std::vector<std::string>& colors)
    {
        /*if (!colors.empty())
        {
            TriMeshPtr _mesh(msbase::mergeColorMeshes(mesh, colors));
            creative_kernel::applyMaterialColorsToMesh(_mesh.get());

            mesh->colors = _mesh->colors;
            mesh->flags = _mesh->flags;
        }*/
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
        fileNames = QFileDialog::getOpenFileName(
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
        QList<qtuser_core::JobPtr> jobs;

        Load3MFJob* loadJob = new Load3MFJob(this);
        loadJob->setOpenJob(false);
        loadJob->setFileName(fileName);
        jobs.push_back(qtuser_core::JobPtr(loadJob));
        cxkernel::executeJobs(jobs);
    }

    void Load3MFCommand::handle(const QString& fileName)
    {
        std::vector<trimesh::TriMesh*> meshs;
        QList<ModelN*> models = selectionms();
        size_t size = models.size();
    }

    QString Load3MFCommand::getMessageText()
    {
        return m_strMessageText;
    }

    void Load3MFCommand::accept()
    {
        openFileLocation(m_fileName);
    }

    Load3MFJob::Load3MFJob(QObject* parent)
        : Job(parent)
        , m_isOpenJob(true)
    {
    }

    Load3MFJob::~Load3MFJob()
    {
    }

    void Load3MFJob::setFileName(const QString& fileName)
    {
        m_fileName = fileName;
    }
    void Load3MFJob::setOpenJob(bool jobType)
    {
        m_isOpenJob = jobType;
    }
    QString Load3MFJob::name()
    {
        return "";
    }

    QString Load3MFJob::description()
    {
        return "";
    }

    void Load3MFJob::failed()
    {
        qDebug() << "";
    }

    void Load3MFJob::successed(qtuser_core::Progressor* progressor)
    {
        if (m_isOpenJob)
        {
            creative_kernel::projectUI()->clearSecen(true);
            projectUI()->setProjectPath(m_fileName);
            Cx3dSubmenuRecentProject::getInstance()->setMostRecentFile(m_fileName);
            projectUI()->setProjectName(ProjectInfoUI::instance()->getNameFromPath(m_fileName));
        }

        getKernel()->parameterManager()->loadProjectSettings(m_projectConfigFile);
        getKernel()->onModelMaterialUpdate();

        QList<ModelN*> adds;
        QList<ModelN*> removes;
        for (QMap<int, QList<cxkernel::ModelNDataPtr>>::iterator iter = m_datas.begin();
            iter != m_datas.end(); ++iter)
        {
            QMap<int, QList<QMap<QString, QString>>>::iterator siter = m_settings.find(iter.key());

            QList<QMap<QString, QString>> kvlists;
            if (siter != m_settings.end())
                kvlists = siter.value();
            int index = 0;
            for (cxkernel::ModelNDataPtr data : iter.value())
            {
                ModelN* model = createModelFromData(data);
                model->setObjectId(data->input.objectId3mf);
                model->setGroupId(iter.key());

                model->buildLocalMatrix(qtuser_3d::xform2QMatrix(trimesh::fxform(data->input.mxform)));

                if (index < kvlists.size())
                {
                    for (QMap<QString, QString>::const_iterator it = kvlists[index].begin(); it != kvlists[index].end();
                        ++it)
                    {
                        model->setting()->add(it.key(), it.value());
                    }
                }
 
                adds.append(model);
                ++index;
            }
        }

        if (adds.size() > 0)
        {
            modifySpace(removes, adds, true);
        }

        int printer_count = m_plates.size();
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
            printer_settings->setCustomGCode(m_plates.at(i));
            
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
        //
        getPrinterMangager()->updatePrinterContent();
        //set flush matrix and wipe_tower_x, wipe_tower_y
        SettingsPointer settings = createCurrentGlobalSettings();
        float wipe_tower_x = settings->vvalue("wipe_tower_x").toFloat();
        float wipe_tower_y = settings->vvalue("wipe_tower_y").toFloat();
        getPrinterMangager()->updateWipeTowersPositions(wipe_tower_x, wipe_tower_y);
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
    }

    void Load3MFJob::work(qtuser_core::Progressor* progressor)
    {
        qtuser_core::ProgressorTracer tracer(progressor);

        common_3mf::Read3MF reader(cxkernel::qString2String(m_fileName));
        common_3mf::Scene3MF& scene = m_scene;

        if (reader.read_all_3mf(scene, &tracer))
        {
            tracer.progress(0.3f);
            int model_count_load = 0;

            for (const common_3mf::Group3MF& group : scene.groups)
            {
                QList<cxkernel::ModelNDataPtr> models;
                models.clear();
                QList<QMap<QString, QString>> settings;

                for (const common_3mf::Model3MF& model : group.models)
                {
                    float start = 0.3f + (float)model_count_load * 0.7f / (float)scene.model_counts;
                    float end = 0.3f + (float)(model_count_load + 1) * 0.7f / (float)scene.model_counts;

                    tracer.resetProgressScope(start, end);

                    model.mesh->need_bbox();
                    trimesh::vec3 size = model.mesh->bbox.size();
                    if ((model.mesh->vertices.size() < 4)
                        || (model.mesh->faces.size() <= 3)
                        || (size.x * size.y * size.z < 1.0f))
                        continue;

                    trimesh::apply_xform(model.mesh.get(), group.xf * model.xf);

                    cxkernel::ModelCreateInput input;
                    input.mesh = model.mesh;
                    input.mxform = model.xf;
                    input.objectId3mf = model.id;
                    input.fileName = m_fileName;
                    input.name = QString("%1").arg(model.name.c_str());
                    input.type = cxkernel::ModelNDataType::mdt_file;
                    input.colors = model.colors;
                    input.seams = model.seams;
                    input.supports = model.supports;
                    input.defaultColor = model.extruder - 1;

                    cxkernel::ModelNDataCreateParam param;
                    cxkernel::ModelNDataPtr data = cxkernel::createModelNData(input, &tracer, param);
                    if (data)
                    {
                        data->input.mxform = trimesh::xform::trans(-data->offset);
                        models.append(data);
                        ++model_count_load;
                    }

                    QMap<QString, QString> kvs;
                    for (std::map<std::string, std::string>::const_iterator it = model.settings.begin();
                        it != model.settings.end(); ++it)
                        kvs.insert(it->first.c_str(), it->second.c_str());
                    for (std::map<std::string, std::string>::const_iterator it = group.settings.begin();
                        it != group.settings.end(); ++it)
                    {
                        if(!kvs.contains(it->first.c_str()))
                            kvs.insert(it->first.c_str(), it->second.c_str());
                    }

                    settings.append(kvs);
                }
                m_datas.insert(group.id, models); 
                m_settings.insert(group.id, settings);
            }
        }

        qtuser_core::getOrCreateAppDataLocation("tmpProject");
        m_projectConfigFile = qtuser_core::getOrCreateAppDataLocation("tmpProject") + "/project_settings.config";
        reader.extract_file(scene.project_settings, cxkernel::qString2String(m_projectConfigFile));

        if (scene.plates.size() > 0)
        {
            int size = (int)scene.plates.size();
            for (int i = 0; i < size; ++i)
            {
                QMap<float, crslice2::Plate_Item> customs;
                for (const common_3mf::PlateCustomGCode& custom_gcode : scene.plates.at(i).gcodes)
                {
                    crslice2::Plate_Item item;
                    item.print_z = custom_gcode.print_z;
                    item.color = custom_gcode.color;
                    item.extruder = custom_gcode.extruder;
                    item.type = (crslice2::Plate_Type)custom_gcode.type;
                    item.extra = custom_gcode.extra;
                    customs.insert(custom_gcode.print_z, item);
                }
                m_plates.append(customs);
            }
        }
        tracer.progress(1.0f);
    }

    std::vector<MeshWithName> load_meshes_from_3mf(const QString& file_name, ccglobal::Tracer* tracer)
    {
        std::vector<MeshWithName> meshes;
        common_3mf::Read3MF reader(file_name.toLocal8Bit().data());
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
