#include "dllansycslicer52chengfei.h"
#include "slice/modelinput.h"

#include "crslice/crscene.h"
#include "crslice/crslice.h"

#include <QtCore/QDebug>
#include <QtCore/QUuid>
#include <QtCore/QFile>

#include "qtusercore/module/progressortracer.h"
#include "qtusercore/module/systemutil.h"
#include "interface/appsettinginterface.h"
#include "data/kernelmacro.h"
#include "msbase/utils/cut.h"

namespace creative_kernel
{
    QString generateTempGCodeFileNamecf()
    {
        QString fileName = QString("%1/temp.gcode").arg(SLICE_PATH);
        QFile::remove(fileName);

        return fileName;
    }

    QString generateTempPolygonFileNamecf()
    {
        QString fileName = QString("%1/poly").arg(SLICE_PATH);
        QFile::remove(fileName);

        return fileName;
    }

    DLLAnsycSlicer52CF::DLLAnsycSlicer52CF(const chengfeiSplit& chengfeiSplit,QObject* parent)
        :AnsycSlicer(parent)
        , m_chengfeiSplit(chengfeiSplit)
    {
    }

    DLLAnsycSlicer52CF::~DLLAnsycSlicer52CF()
    {
    }


    SliceResultPointer DLLAnsycSlicer52CF::doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, SliceAttain* _fDebugger)
    {
        float progressStep = 0.8f;
        tracer.resetProgressScope(0.0f, progressStep);

        bool failed = false;
        CrScenePtr scene(new crslice::CrScene());

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
        QString polyFileName = generateTempPolygonFileNamecf();
        scene->setPloygonFileName(polyFileName.toLocal8Bit().data());

        //save lines
        std::vector<std::vector<trimesh::vec2>> polys;
        for (const auto& poly : m_chengfeiSplit.lineList)
        {
            std::vector<trimesh::vec2> ps;
            for (const auto& p:poly)
            {
                ps.push_back(trimesh::vec2(p.first,p.second));
            }
            polys.push_back(ps);
        }
        scene->savePloygons(polys);

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

            //¶¨ÖÆÊÇ·ñÖÃµ×
            if (m_chengfeiSplit.type == chengfeiSplit::Type::HEIGHT_RESTRICTIONS)
            {
                settings->add("remove_empty_first_layers", m_chengfeiSplit.toPlate == true ? "true" : "false");
            }

            if (m_chengfeiSplit.type == chengfeiSplit::Type::CONTOUR_PARTITION)
            {
                settings->add("poly_order_user_specified", "true");
                if (!m_chengfeiSplit.orderPointList.empty())
                {
                    std::string str = "[";

                    auto it = m_chengfeiSplit.orderPointList.begin();
                    str += std::to_string(it->first);
                    str += ",";
                    str += std::to_string(it->second);
                    it++;
                    while (it != m_chengfeiSplit.orderPointList.end())
                    {
                        str += ",";
                        str += std::to_string(it->first);
                        str += ",";
                        str += std::to_string(it->second);
                        ++it;
                    }
                    str += "]";

                    settings->add("poly_order_user_specified_str", str);
                }
            }
            if (m_chengfeiSplit.type == chengfeiSplit::Type::HORIZONTAL_PARTITION)
            {
                settings->add("mesh_order_user_specified", "true");

                if (!m_chengfeiSplit.sliceOrder.empty())
                {
                    std::string str = "[";

                    auto it = m_chengfeiSplit.sliceOrder.begin();
                    str += std::to_string(*it);
                    it++;
                    while (it != m_chengfeiSplit.sliceOrder.end())
                    {
                        str += ",";
                        str += std::to_string(*it);
                        ++it;
                    }
                    str += "]";

                    settings->add("mesh_order_user_specified_str", str);
                }
            }
            else
            {
                settings->add("mesh_order_user_specified", "false");
            }

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

                auto add2group = [&MS, &scene, &groupID, &offsetXYZ](TriMeshPtr m)
                {
                    if (!m)
                    {
                        return;
                    }

                    if (offsetXYZ != trimesh::vec3())
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
                };

                if (m_chengfeiSplit.type == chengfeiSplit::Type::HEIGHT_RESTRICTIONS)
                {
                    //split
                    trimesh::TriMesh* out = nullptr;
                    msbase::splitRangeZ(m.get(), m_chengfeiSplit.topHeight, m_chengfeiSplit.bottomHeight, &out);
                    std::shared_ptr<trimesh::TriMesh> outptr(out);
                    m.swap(outptr);

                    add2group(m);
                }
                else if (m_chengfeiSplit.type == chengfeiSplit::Type::CONTOUR_PARTITION)
                {
                    modelInput->settings->add("poly_order_user_specified", "true");
                    add2group(m);
                }
                else if (m_chengfeiSplit.type == chengfeiSplit::Type::HORIZONTAL_PARTITION)
                {
                    m->need_bbox();
                    trimesh::vec box = m->bbox.min;
                    std::vector<trimesh::vec3> horizon;
                    for (auto h : m_chengfeiSplit.depthSplitPosList)
                    {
                        horizon.push_back(trimesh::vec3(box.x + h, 0.0, 0.0));
                    }
                    std::vector<trimesh::vec3> vertical;
                    for (auto v : m_chengfeiSplit.widthSplitPosList)
                    {
                        vertical.push_back(trimesh::vec3(0.0, box.y + v, 0.0));
                    }
                    std::vector < trimesh::TriMesh*> outMesh;
                    float interval = m_chengfeiSplit.interval > 0.0f ? m_chengfeiSplit.interval : 0.0f;
                    //mmesh::splitRangeXYZ(m.get(), horizon,vertical, interval, outMesh);

                    for (int i = 0; i < outMesh.size(); i++)
                    {
                        if (outMesh[i] != nullptr)
                        {
                            std::shared_ptr<trimesh::TriMesh> outptr(outMesh[i]);
                            add2group(outptr);
                        }
                    }
                }
                else
                {
                    add2group(m);
                }
            }
            scene->setGroupOffset(groupID, offsetXYZ);
        }

#if EXPORT_FDM52_DATA
        QString sceneFile = QString("%1/%2").arg(TEMPGCODE_PATH).arg(QUuid::createUuid().toString(QUuid::Id128));
        scene->save(sceneFile.toStdString());

        qDebug() << QString("DLLAnsycSlicer52 EXPORT_FDM52_DATA -> [%1]").arg(sceneFile);
#endif

        QString fileName = generateTempGCodeFileNamecf();
        scene->setOutputGCodeFileName(fileName.toLocal8Bit().data());

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
