#include "parameterutil.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/trimesh2/conv.h"
#include "interface/camerainterface.h"
#include "interface/reuseableinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "internal/render/printerentity.h"
#include "data/modeln.h"

#include "internal/parameter/printmachine.h"
#include "internal/multi_printer/printermanager.h"
#include "us/usettingwrapper.h"

namespace creative_kernel
{
	void applyPrintMachine(PrintMachine* machine)
	{
        if (!machine)
            return;

        us::USettings* settings = machine->settings()->copy();
        settings->merge(machine->userSettings());

        QList<ModelN*> ModelNs = modelns();

        float machine_width = get_machine_width(settings);
        float machine_height = get_machine_height(settings);
        float machine_depth = get_machine_depth(settings);
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

        trimesh::dbox3 box;
        if (isZero)
        {
            float MAX_X = machine_width / 2.0;
            float MAX_Y = machine_depth / 2.0;
            box = trimesh::dbox3(trimesh::dvec3(-MAX_X, -MAX_Y, 0), trimesh::dvec3(MAX_X, MAX_Y, machine_height));
            creative_kernel::setBaseBoundingBox(box);
        }
        else
        {
            box = trimesh::dbox3(trimesh::dvec3(0.0, 0.0, 0.0), trimesh::dvec3(machine_width, machine_depth, machine_height));
            creative_kernel::setBaseBoundingBox(box);
        }



        qtuser_3d::ScreenCamera* aScreencamera = visCamera();
        if (isBelt)
        {
        }
        else
        {
            aScreencamera->setUpdateNearFarRuntime(true);
            //aScreencamera->fittingBoundingBox(box);
        }


        creative_kernel::setModelEffectBox(qtuser_3d::vec2qvector(box.min),
            qtuser_3d::vec2qvector(box.max));

        for (size_t i = 0; i < ModelNs.size(); i++)
        {
            ModelN* modeln = ModelNs.at(i);
            if (modeln->selected())
            {
                notifyPickableChanged(modeln);
            }
        }
	}
}