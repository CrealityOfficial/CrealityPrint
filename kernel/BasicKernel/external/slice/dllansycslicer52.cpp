#include "dllansycslicer52.h"
#include "modelinput.h"

#include "crslice/crscene.h"
#include "crslice/crslice.h"

#include <QtCore/QDebug>
#include <QtCore/QUuid>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "qtusercore/module/progressortracer.h"
#include "qtusercore/module/systemutil.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "data/kernelmacro.h"

namespace creative_kernel
{
    QString generateTempGCodeFileName()
    {
        QString fileName = QString("%1/temp.gcode").arg(SLICE_PATH);
        QFile::remove(fileName);

        return fileName;
    }

    DLLAnsycSlicer52::DLLAnsycSlicer52(QObject* parent)
        :AnsycSlicer(parent)
    {
    }

    DLLAnsycSlicer52::~DLLAnsycSlicer52()
    {
    }

    SliceResultPointer DLLAnsycSlicer52::doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, crslice::PathData* _fDebugger)
    {
        float progressStep = 0.8f;
        tracer.resetProgressScope(0.0f, progressStep);

        bool failed = false;
        CrScenePtr scene(new crslice::CrScene());

        scene->m_debugger = _fDebugger;

        //Global Settings
        const QHash<QString, us::USetting*>& G = input.G->settings();
        for (QHash<QString, us::USetting*>::const_iterator it = G.constBegin(); it != G.constEnd(); ++it)
        {
#if 0  // debug
            if (it.key() == "machine_start_gcode")
            {
                qDebug() << it.value()->str();
            }
            QString v = it.value()->str();
            int len = v.size();
            std::string _v = v.toLocal8Bit();
            len = _v.size();
#endif
            scene->m_settings->add(it.key().toStdString(), it.value()->str().toStdString());
        }

        //Extruder Settings
        size_t extruder_count = std::stoul(scene->m_settings->getString("machine_extruder_count"));
        if (extruder_count == 0)
            extruder_count = 1;

        for (size_t extruder_nr = 0; extruder_nr < extruder_count; extruder_nr++)
            scene->m_extruders.emplace_back(new crslice::Settings());

        for (size_t i = 0; i < extruder_count; ++i)
        {
            int32_t extruder_nr = i;
            if (extruder_nr < 0 || extruder_nr >= static_cast<int32_t>(input.Es.size()))
            {
                failed = true;
                break;
            }

            SettingsPtr& extruder = scene->m_extruders[i];

            const QHash<QString, us::USetting*>& E = input.Es.at(i)->settings();
            for (QHash<QString, us::USetting*>::const_iterator it = E.constBegin(); it != E.constEnd(); ++it)
            {
                extruder->add(it.key().toStdString(), it.value()->str().toStdString());
            }
        }

        scene->machine_center_is_zero = G.value("machine_center_is_zero")->enabled();
        //model groups
        for (ModelGroupInput* modelGroupInput : input.Groups)
        {
            QList<ModelInput*>& modelInputs = modelGroupInput->modelInputs;

            if (modelInputs.size() <= 0)
            {
                continue; //Don't slice empty mesh groups.
            }

            int groupID = scene->addOneGroup();

            SettingsPtr settings(new crslice::Settings());
            const QHash<QString, us::USetting*>& MG = modelGroupInput->settings->settings();

            //Load the settings in the mesh group.
            for (QHash<QString, us::USetting*>::const_iterator it = MG.constBegin(); it != MG.constEnd(); ++it)
            {
                settings->add(it.key().toStdString(), it.value()->str().toStdString());
            }
            settings->add("machine_center_is_zero", "true");   //our system is always build in printer corner(origin), so set machine_center_is_zero == true, skip cura engine machine offset

            scene->setGroupSettings(groupID, settings);

            trimesh::vec3 offsetXYZ = trimesh::vec3();
            if (G.value("special_slope_slice_angle")->toFloat() != 0)
            {
                trimesh::TriMesh::BBox offset_box;
                for (ModelInput* modelInput : modelInputs)
                {
                    TriMeshPtr m = modelInput->mesh();
                    m->clear_bbox();
                    m->need_bbox();
                    offset_box += m->bbox;
                }
                offsetXYZ = trimesh::vec3(-offset_box.min.x, -offset_box.min.y, -offset_box.min.z);
            }
            for (ModelInput* modelInput : modelInputs)
            {
                const QHash<QString, us::USetting*>& MS = modelInput->settings->settings();
                TriMeshPtr m = modelInput->mesh();

                if (!m)
                {
                    failed = true;
                    break;
                }

               if(offsetXYZ != trimesh::vec3())
                    trimesh::trans(m.get(), offsetXYZ);

                SettingsPtr meshSettings(new crslice::Settings());
                //Load the settings for the mesh.
                for (QHash<QString, us::USetting*>::const_iterator it = MS.constBegin(); it != MS.constEnd(); ++it)
                {
                    meshSettings->add(it.key().toStdString(), it.value()->str().toStdString());
                }

                int objectID = scene->addObject2Group(groupID);
                scene->setObjectSettings(groupID, objectID, meshSettings);
                scene->setOjbectMesh(groupID, objectID, m);
                QString fileName = QString("%1/poly").arg(SLICE_PATH);
                std::string s = fileName.toLocal8Bit().data();
                scene->setOjbectExclude(groupID, objectID, s + std::to_string(objectID), modelInput->outline_ObjectExclude);
            }
            scene->setGroupOffset(groupID, offsetXYZ);
        }

#if EXPORT_FDM52_DATA
		QDir dir(TEMPGCODE_PATH);
		dir.setFilter(QDir::Files);
		int fileCount = dir.count();
		for (int i = 0; i < fileCount; i++)
			dir.remove(dir[i]);

        QString sceneFile = QString("%1/%2-%3").arg(TEMPGCODE_PATH).arg(generateSceneName())
                                                                    .arg(QUuid::createUuid().toString(QUuid::Id128));
        scene->save(sceneFile.toLocal8Bit().data());

        qDebug() << QString("DLLAnsycSlicer52 EXPORT_FDM52_DATA -> [%1]").arg(sceneFile);
#endif

        QString fileName = generateTempGCodeFileName();
        scene->setOutputGCodeFileName(fileName.toLocal8Bit().data());

        QString supportFile = QString("%1/paint_support").arg(SLICE_PATH);
        scene->setSupportFileName(supportFile.toLocal8Bit().data());
        QString paint_anti_support = QString("%1/paint_anti_support").arg(SLICE_PATH);
        scene->setAntiSupportFileName(paint_anti_support.toLocal8Bit().data());
        QString paint_seam = QString("%1/paint_seam").arg(SLICE_PATH);
        scene->setSeamFileName(paint_seam.toLocal8Bit().data());
        QString paint_anti_seam = QString("%1/paint_anti_seam").arg(SLICE_PATH);
        scene->setAntiSeamFileName(paint_anti_seam.toLocal8Bit().data());


        qDebug() << QString("Slice : DLLAnsycSlicer52 construct scene . [%1]").arg(currentProcessMemory());
        crslice::CrSlice slice;
        slice.sliceFromScene(scene, &tracer);

        qDebug() << QString("Slice : DLLAnsycSlicer52 slice over . [%1]").arg(currentProcessMemory());
        if (!failed && !tracer.error())
        {
            tracer.resetProgressScope(progressStep, 1.0f);
            SliceResultPointer result(new gcode::SliceResult());
            result->load(fileName.toLocal8Bit().data(), &tracer);

            qDebug() << QString("Slice : DLLAnsycSlicer52 load over . [%1]").arg(currentProcessMemory());
            if (result->layerCode().size() == 0)
                result.reset();
            return result;
        }
        return nullptr;
    }
}
