#include "dllansycslicerorca.h"

#include "crslice2/crscene.h"
#include "crslice2/crslice.h"

#include "modelinput.h"
#include "ansycworker.h"

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

    //test data
    void testPlates(crslice2::CrScenePtr& scene)
    {
        scene->plates_custom_gcodes;
        crslice2::PlateInfo info;
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
            scene->setGroupTransform(groupID, modelGroupInput->groupTransform);
            scene->setGroupSceneObjectId(groupID, modelGroupInput->groupId);

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

                ModelNInput* modelNInput = dynamic_cast<ModelNInput*>(modelInput);
                std::vector<double> layer;
                if (modelNInput != nullptr)
                {
                    layer = modelNInput->layerHeightProfile();
                }

                // fix bug:[界面上错误显示信息乱码]
                std::string nameStr = cxkernel::qString2String(modelInput->name());
                scene->setOjbectMeshPaint(groupID, objectID, m, modelInput->getXform(), modelInput->getColors2Facets(), modelInput->getSeam2Facets(), modelInput->getSupport2Facets(), nameStr, layer, modelInput->modelType());
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
        const std::string _blname = blName.toLocal8Bit().constData();
        scene->setBLName(_blname);

        qDebug() << QString("Slice : DLLAnsycSlicer52 construct scene . [%1]").arg(currentProcessMemory());
        crslice2::CrSlice slice;
        slice.sliceFromScene(scene, &tracer);
        if (_fDebugger)
        {
            QMap<QString, QPair<QString, int64_t> > slice_messages;
            for (std::map<std::string, std::pair<std::string, int64_t>>::iterator it = slice.extraSliceWarningDetails.begin();
                it != slice.extraSliceWarningDetails.end(); ++it) {
                slice_messages.insert(QString::fromUtf8(it->first.c_str()), qMakePair(QString::fromUtf8(it->second.first.c_str()), it->second.second));
            }
            _fDebugger->setSliceWarningDetails(slice_messages);
        }

        qDebug() << QString("Slice : DLLAnsycSlicer52 slice over . [%1]").arg(currentProcessMemory());
        if (!failed && !tracer.error())
            return nullptr;
            //return generateResult(fileName, tracer);
        return nullptr;
    }
}
