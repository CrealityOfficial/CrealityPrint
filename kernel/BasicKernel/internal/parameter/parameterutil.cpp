#include "parameterutil.h"
#include "qtuser3d/camera/screencamera.h"
#include "interface/camerainterface.h"
#include "interface/reuseableinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "internal/render/printerentity.h"
#include "data/modeln.h"

#include "internal/parameter/printmachine.h"

namespace creative_kernel
{
	void applyPrintMachine(PrintMachine* machine)
	{
        if (!machine)
            return;

        us::USettings* settings = machine->settings()->copy();
        settings->merge(machine->userSettings());

        QList<ModelN*> ModelNs = modelns();
        QVector<QVector3D> vctdistance;
        for (size_t i = 0; i < ModelNs.size(); i++)
        {
            QVector3D oldboxcenter = baseBoundingBox().center();
            QVector3D qdistance = baseBoundingBox().center() - ModelNs.at(i)->globalSpaceBox().center();
            qdistance.setZ(0.0);
            vctdistance.push_back(qdistance);
        }

        QString machine_widthStr = settings->vvalue("machine_width").toString();
        float machine_width = settings->vvalue("machine_width").toFloat();
        float machine_height = settings->vvalue("machine_height").toFloat();
        float machine_depth = settings->vvalue("machine_depth").toFloat();
        int machine_extruder_count = settings->vvalue("machine_extruder_count").toInt();
        bool isZero = settings->vvalue("machine_center_is_zero").toBool();
        QString machineName = settings->vvalue("machine_name").toString();
        bool isBelt = settings->vvalue("machine_is_belt").toBool();
        settings->clear();
        delete settings;

        if (machine_extruder_count == 1)
        {
            QList<ModelN*> models = modelns();
            for (size_t i = 0; i < models.size(); i++)
            {
                ModelN* m = models.at(i);
                m->setNozzle(0);
            }
        }

        if (machine_width < 1)
        {
            machine_width = 1;
        }
        if (machine_depth < 1)
        {
            machine_depth = 1;
        }
        if (machine_height < 1)
        {
            machine_height = 1;
        }

        //热床分模块加热底图
        PrinterEntity* entity = getCachedPrinterEntity();
        if (machineName == "Creality CR-10 Inspire Pro")
        {
            entity->drawBedFaces(qtuser_3d::bedType::CR_10_Inspire_Pro);
        }
        else if (machineName == "Creality CR-GX")
        {
            entity->drawBedFaces(qtuser_3d::bedType::CR_GX);
        }
        else if (machineName == "Creality CR-10 H1")
        {
            entity->drawBedFaces(qtuser_3d::bedType::CR_10H1);
        }
        else
        {
            entity->drawBedFaces(qtuser_3d::bedType::None);
        }

        qtuser_3d::Box3D box;
        if (isZero)
        {
            float MAX_X = machine_width / 2.0;
            float MAX_Y = machine_depth / 2.0;
            box = qtuser_3d::Box3D(QVector3D(-MAX_X, -MAX_Y, 0), QVector3D(MAX_X, MAX_Y, machine_height));
            creative_kernel::setBaseBoundingBox(box);
        }
        else
        {
            box = qtuser_3d::Box3D(QVector3D(0, 0, 0), QVector3D(machine_width, machine_depth, machine_height));
            creative_kernel::setBaseBoundingBox(box);
        }

        qtuser_3d::ScreenCamera* aScreencamera = visCamera();
        if (isBelt)
        {
            Qt3DRender::QCamera* aCamera = aScreencamera->camera();
            //setFarPlane.set
            QVector3D size = box.size();
            qtuser_3d::Box3D b;
            b += box.min;
            QVector3D bmax = box.max;
            bmax.setY(1000.0f);
            b += bmax;
            aScreencamera->fittingBoundingBox(b);
            aCamera->lens()->setFarPlane(machine_depth);
            aScreencamera->setUpdateNearFarRuntime(false);
        }
        else
        {
            aScreencamera->setUpdateNearFarRuntime(true);
            aScreencamera->fittingBoundingBox(box);
        }

        creative_kernel::setModelEffectBox(box.min, box.max);


        for (size_t i = 0; i < ModelNs.size(); i++)
        {
            QVector3D NewCenter = baseBoundingBox().center() - vctdistance[i];
            NewCenter.setZ(0.0);
            QVector3D newLocal = ModelNs.at(i)->mapGlobal2Local(NewCenter);
            ModelNs.at(i)->setLocalPosition(newLocal);
        }

        for (size_t i = 0; i < ModelNs.size(); i++)
        {
            ModelN* modeln = ModelNs.at(i);
            if (modeln->selected())
            {
                selectedPickableInfoChanged(modeln);
            }
        }

        checkModelRange();
        checkBedRange();
	}
}