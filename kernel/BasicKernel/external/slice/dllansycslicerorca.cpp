#include "dllansycslicerorca.h"

#include "crslice2/crscene.h"
#include "crslice2/crslice.h"

#include "modelinput.h"

#include <QtCore/QDebug>
#include <QtCore/QUuid>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "qtusercore/module/progressortracer.h"
#include "qtusercore/module/systemutil.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "data/kernelmacro.h"
#include "interface/machineinterface.h"
#include "interface/printerinterface.h"
#include "slice/modelninput.h"
#include "cxkernel/utils/utils.h"
#include "interface/unittestinterface.h"
#include "interface/projectinterface.h"
namespace creative_kernel
{
    DLLAnsycSlicerOrca::DLLAnsycSlicerOrca(QObject* parent)
        :AnsycSlicer(parent)
    {
    }

    DLLAnsycSlicerOrca::~DLLAnsycSlicerOrca()
    {
    }

    void dealDataOverrides(QStringList& keys)
    {
        QString keys1 = "filament_deretraction_speed,filament_retract_before_wipe,filament_retract_lift_above,filament_retract_lift_below,filament_retract_lift_enforce,filament_retract_restart_extra,filament_retract_when_changing_layer,filament_retraction_length,filament_retraction_minimum_travel,filament_retraction_speed,filament_wipe,filament_wipe_distance,filament_z_hop,filament_z_hop_types";
        QString  _keys = keys1;
        keys = _keys.split(",");
    }

    void dealData(QStringList& keys)
    {
        QString keys1 = "material_flow_dependent_temperature,material_flow_temp_graph,cool_special_cds_fan_speed,cool_cds_fan_start_at_height,additional_cooling_fan_speed,bed_temperature_difference,close_fan_the_first_x_layers,cool_plate_temp,cool_plate_temp_initial_layer,enable_overhang_bridge_fan,enable_pressure_advance,eng_plate_temp,eng_plate_temp_initial_layer,fan_cooling_layer_time,fan_max_speed,fan_min_speed,filament_ramming_parameters";
        QString keys2 = "filament_start_gcode,filament_end_gcode,filament_colour,filament_cost,filament_density,filament_diameter,filament_flow_ratio,filament_ids,filament_is_support,filament_max_volumetric_speed,filament_minimal_purge_on_wipe_tower,filament_settings_id,filament_shrink,filament_soluble,filament_type,filament_vendor";
        QString keys3 = "full_fan_speed_layer,hot_plate_temp,hot_plate_temp_initial_layer,nozzle_temperature,nozzle_temperature_initial_layer,nozzle_temperature_range_high,nozzle_temperature_range_low,overhang_fan_speed,overhang_fan_threshold,pressure_advance,reduce_fan_stop_start_freq,required_nozzle_HRC,slow_down_for_layer_cooling,slow_down_layer_time,slow_down_min_speed,support_material_interface_fan_speed,temperature_vitrification,textured_plate_temp,textured_plate_temp_initial_layer,extruder_offset";
        
        //add V1.8.1
        QString keys4 = "complete_print_exhaust_fan_speed,activate_air_filtration,activate_chamber_temp_control,chamber_temperature,during_print_exhaust_fan_speed,filament_cooling_final_speed,filament_cooling_initial_speed,filament_cooling_moves,filament_load_time,filament_loading_speed,filament_loading_speed_start,filament_multitool_ramming,filament_multitool_ramming_flow,filament_multitool_ramming_volume,filament_toolchange_delay,filament_unload_time,filament_unloading_speed,filament_unloading_speed_start";

        QString  _keys = keys1 +  "," + keys2 + "," + keys3 + "," + keys4;
        keys =  _keys.split(",");
    }

    void flush_volumes(SettingsPointer& G, int colorSize)
    {
        QString flush_volumes_matrix = G->value("flush_volumes_matrix", "280");
        QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");
        if (_flush_volumes_matrixs.size() != colorSize* colorSize)
        {
            QString _flush_volumes_matrix = "";
            if (colorSize) {
                for (int i = 0; i < colorSize; i++) {
                    for (int j = 0; j < colorSize; j++) {
                        if (i == j)
                        {
                            if (i == colorSize -1)
                                _flush_volumes_matrix += "0";
                            else
                            _flush_volumes_matrix += "0,";
                        }
                        else
                            _flush_volumes_matrix += "280,";
                    }
                }
                G->add("flush_volumes_matrix", _flush_volumes_matrix, true);
                G->add("flush_volumes_multiplier", "0.5", true);
            }
        }
    }

    void _dealExtruderOverrides(SettingsPointer& G, QList<SettingsPointer>& Es)
    {
        QStringList keys;
        dealDataOverrides(keys);
        std::vector<QString> values(keys.size(), "");
        int size = Es.size();
        for (int i = 0; i < keys.size(); i++)
        {
            for (int j = 0; j < size; j++)
            {
                auto& setting = Es.at(j);
                us::USetting* _setting = setting->findSetting(keys[i]);
                if (_setting != nullptr)
                {
                    values[i] += _setting->str();
                }
                else
                    values[i] +=  "nil";

                if (j != size-1)
                {
                    values[i] += ",";
                }
            }
        }

        for (int i = 0; i < keys.size(); i++)
        {
            G->add(keys[i], values[i], true);
        }
    }

    void _dealExtruder(SettingsPointer& G,QList<SettingsPointer>& Es)
    {
        QStringList keys;
        dealData(keys);
        flush_volumes(G, Es.size());
        bool first = false;
        std::vector<QString> values(keys.size(),"");
        for (auto& setting : Es)
        {
            for (int i = 0; i < keys.size(); i++)
            {
                if (first)
                {
                    if (keys[i] == "filament_colour" 
                        || keys[i] == "filament_ids"
                        || keys[i] == "filament_settings_id"
                        || keys[i] == "filament_type"
                        || keys[i] == "filament_vendor"
                        || keys[i] == "filament_end_gcode"
                        || keys[i] == "filament_start_gcode"
                        || keys[i] == "filament_ramming_parameters")
                    {
                        values[i] += ";";
                    }
                    else
                        values[i] += ",";

                }
                QString v = setting->value(keys[i], "");

                if (keys[i] == "filament_end_gcode"
                    || keys[i] == "filament_start_gcode")
                    v = QString("\"%1\"").arg(v);
                values[i] += v;
            }

            first = true;
        }

        for (int i=0;i< keys.size();i++)
        {
            G->add(keys[i], values[i], true);
        }

        //material_flow_dependent_temperature,material_flow_temp_graph,取实际用到的耗材的自动温度
        if (Es.size())
        {
            G->add("material_flow_dependent_temperature", Es[0]->value("material_flow_dependent_temperature", ""), true);
            G->add("material_flow_temp_graph", Es[0]->value("material_flow_temp_graph", ""), true);

            QString default_filament_colour = G->value("default_filament_colour","");
            QStringList values = default_filament_colour.split(";");
            for (int i = 0; i < values.size(); i++)
            {
                if (!values[i].isEmpty() && Es.size() > i)
                {
                    G->add("material_flow_dependent_temperature", Es[i]->value("material_flow_dependent_temperature", ""), true);
                    G->add("material_flow_temp_graph", Es[i]->value("material_flow_temp_graph", ""), true);
                    break;
                }
            }
        }
    }

    //test data
    void testPlates(crslice2::CrScenePtr& scene)
    {
        scene->plates_custom_gcodes;
        crslice2::plateInfo info;
        info.mode = crslice2::Plate_Mode::MultiAsSingle;

        crslice2::Plate_Item item1;
        item1.print_z = 0.23999999463558197;
        item1.type = crslice2::Plate_Type(2);
        item1.extruder = 1;
        item1.color = "#000000";
        item1.extra = "";

        crslice2::Plate_Item item2;
        item2.print_z = 0.56000000238418579;
        item2.type = crslice2::Plate_Type(2);
        item2.extruder = 2;
        item2.color = "#FF0000";
        item2.extra = "";

        crslice2::Plate_Item item3;
        item3.print_z = 0.95999997854232788;
        item3.type = crslice2::Plate_Type(2);
        item3.extruder = 3;
        item3.color = "#FFFF00";
        item3.extra = "";

        crslice2::Plate_Item item4;
        item4.print_z = 1.5199999809265137;
        item4.type = crslice2::Plate_Type(2);
        item4.extruder = 4;
        item4.color = "#FFFFFF";
        item4.extra = "";

        info.gcodes.push_back(item1);
        info.gcodes.push_back(item2);
        info.gcodes.push_back(item3);
        info.gcodes.push_back(item4);
        scene->plates_custom_gcodes.emplace(0,info);
    }

    SliceResultPointer DLLAnsycSlicerOrca::doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, SliceAttain* _fDebugger)
    {
        bool failed = false;
        crslice2::CrScenePtr scene(new crslice2::CrScene());
        scene->m_unittest_type = slicerUnitType();
        _dealExtruder(input.G, input.Es);
        _dealExtruderOverrides(input.G, input.Es);

        input.G->add("wipe_tower_x", getAllWipeTowerPositionX(), true);
        input.G->add("wipe_tower_y", getAllWipeTowerPositionY(), true);

        scene->m_isBBLPrinter = currentMachineIsBbl() || currentPrinterHasMultiColors();

        //todo �߶Ȳ���
        //testPlates(scene);

        scene->m_plate_index = currentPlateIndex();

        scene->plates_custom_gcodes.emplace(0, input.plate);

        //testPlates(scene);

        //todo
        scene->thumbnailDatas = input.pictures;

        //Global Settings
        const QHash<QString, us::USetting*>& G = input.G->settings();
        for (QHash<QString, us::USetting*>::const_iterator it = G.constBegin(); it != G.constEnd(); ++it)
        {
            scene->m_settings->add(it.key().toStdString(), it.value()->str().toStdString());
        }

        //extruder
        for (SettingsPointer setting : input.Es)
        {
            crslice2::SettingsPtr ptr(new crslice2::Settings());
            const QHash<QString, us::USetting*>& ES = setting->settings();
            for (QHash<QString, us::USetting*>::const_iterator it = ES.constBegin(); it != ES.constEnd(); ++it)
            {
                ptr->add(it.key().toStdString(), it.value()->str().toStdString());
            }

            scene->m_extruders.push_back(ptr);
        }

        //model groups
        for (ModelGroupInput* modelGroupInput : input.Groups)
        {
            QList<ModelInput*>& modelInputs = modelGroupInput->modelInputs;

            if (modelInputs.size() <= 0)
            {
                continue; //Don't slice empty mesh groups.
            }

            int groupID = scene->addOneGroup();

            crslice2::SettingsPtr settings(new crslice2::Settings());
            const QHash<QString, us::USetting*>& MG = modelGroupInput->settings->settings();

            //Load the settings in the mesh group.
            for (QHash<QString, us::USetting*>::const_iterator it = MG.constBegin(); it != MG.constEnd(); ++it)
            {
                settings->add(it.key().toStdString(), it.value()->str().toStdString());
            }
            scene->setGroupSettings(groupID, settings);

            for (ModelInput* modelInput : modelInputs)
            {
                const QHash<QString, us::USetting*>& MS = modelInput->settings->settings();
                TriMeshPtr m = modelInput->mesh();

                if (!m)
                {
                    failed = true;
                    break;
                }

               crslice2::SettingsPtr meshSettings(new crslice2::Settings());
                //Load the settings for the mesh.
                for (QHash<QString, us::USetting*>::const_iterator it = MS.constBegin(); it != MS.constEnd(); ++it)
                {
                    meshSettings->add(it.key().toStdString(), it.value()->str().toStdString());
                }

                int objectID = scene->addObject2Group(groupID);
                scene->setObjectSettings(groupID, objectID, meshSettings);

                if (m->flags.size() != m->faces.size())
                    m->flags.resize(m->faces.size(), 2);
                ModelNInput* modelNInput = dynamic_cast<ModelNInput*>(modelInput);
                std::vector<double> layer;
                if (modelNInput != nullptr)
                {
                    layer = modelNInput->layerHeightProfile();
                }
                scene->setOjbectMeshPaint(groupID, objectID, m, modelInput->getXform(), modelInput->getColors2Facets(), modelInput->getSeam2Facets(), modelInput->getSupport2Facets(), modelInput->name().toStdString(), layer,0);
            }
        }

        QString sceneFile = sceneTempFile();
        scene->save(sceneFile.toLocal8Bit().data());

        QString fileName = _fDebugger->tempGCodeFileName();
        QString sPath = fileName.left(fileName.lastIndexOf('/'));
        QDir dir(sPath);
        if (!dir.exists(sPath))
        {
            if (!dir.mkdir(sPath))
            {
                qDebug() << QString("Slice :  path [%1] is not there .").arg(sPath);
                failed = true;
                return nullptr;
            }
        }
        scene->setOutputGCodeFileName(cxkernel::qString2String(fileName));
        
        scene->setTempDirectory(generateTempFileName().toLocal8Bit().data());

        QString slicerBLPath = slicerBaselinePath();
        scene->setSliceBLDirectory((slicerBLPath).toLocal8Bit().data());
        QString slicerComparePath = slicerCompErrorPath();
        scene->setBLCompareErrorDirectory((slicerComparePath).toLocal8Bit().data());

        QFileInfo info(fileName);
        QString blName =  info.baseName();

        QString _projectName =  projectPath();
        QFileInfo projectInfo(_projectName);
        blName = projectInfo.suffix() + "_" + projectInfo.baseName() + "_" + blName;
        scene->setBLName(blName.toLocal8Bit().data());

        qDebug() << QString("Slice : DLLAnsycSlicer52 construct scene . [%1]").arg(currentProcessMemory());
        crslice2::CrSlice slice;
        slice.sliceFromScene(scene, &tracer);

        qDebug() << QString("Slice : DLLAnsycSlicer52 slice over . [%1]").arg(currentProcessMemory());
        if (!failed && !tracer.error())
            return nullptr;
            //return generateResult(fileName, tracer);
        return nullptr;
    }
}
