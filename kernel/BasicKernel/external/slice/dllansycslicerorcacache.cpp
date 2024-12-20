#include "dllansycslicerorcacache.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>

#include "interface/spaceinterface.h"
#include "data/modelspace.h"
#include "qtuser3d/trimesh2/conv.h"

#include "ansycslicer.h"
#include "cxkernel/utils/utils.h"
#include "crslice2/cacheslice.h"
#include "interface/unittestinterface.h"
#include "interface/projectinterface.h"
#include <QFileInfo>
#include <qtusercore/string/resourcesfinder.h>
namespace creative_kernel
{
    OrcaCacheSlice::OrcaCacheSlice(QObject* parent)
        :QObject(parent)
    {

    }

    OrcaCacheSlice::~OrcaCacheSlice()
    {
        
    }

	void OrcaCacheSlice::cacheSlice(Printer* printer, SettingsPointer G, const QList<SettingsPointer>& Es,
        const std::vector<crslice2::ThumbnailData>& thumbnails, const crslice2::PlateInfo& plate_infos,
        qtuser_core::ProgressorTracer* tracer, SliceAttain* attain)
	{
        if (!attain)
        {
            if(tracer)
                tracer->failed("no attain.");
            return;
        }

        ModelSpace* space = getModelSpace();
        
        //update datas
        space->update_crslice2_parameter();
        space->m_crslice_model.setPlateInfo(plate_infos);

        //build volume
        trimesh::box3 box = qtuser_3d::qBox32box3(printer->globalBox());
        space->update_crslice2_matrix(trimesh::xform::identity());
        std::vector<trimesh::dvec2> shapes;
        double height = box.max.z;
        shapes.push_back(trimesh::dvec2(box.min.x, box.min.y));
        shapes.push_back(trimesh::dvec2(box.max.x, box.min.y));
        shapes.push_back(trimesh::dvec2(box.max.x, box.max.y));
        shapes.push_back(trimesh::dvec2(box.min.x, box.max.y));
        crslice2::update_print_volume_state(space->m_crslice_model, shapes, height);

        //update parameters
        space->update_crslice2_matrix(trimesh::xform::trans(-box.min));
        const QHash<QString, us::USetting*>& GItems = G->settings();
        for (QHash<QString, us::USetting*>::const_iterator it = GItems.constBegin(); it != GItems.constEnd(); ++it)
        {
            if (it.key() == QString("wipe_tower_x"))
                qDebug() << QString("wipe_tower_x %1").arg(it.value()->str());
            if (it.key() == QString("wipe_tower_y"))
                qDebug() << QString("wipe_tower_y %1").arg(it.value()->str());

            if (it.key() == QString("epoxy_resin_plate_temp"))
                qDebug() << QString("epoxy_resin_plate_temp %1").arg(it.value()->str());
            if (it.key() == QString("epoxy_resin_plate_temp_initial_layer"))
                qDebug() << QString("epoxy_resin_plate_temp_initial_layer %1").arg(it.value()->str());
            //if (it.key() == QString("filament_colour"))
            //    qDebug() << QString("filament_colour %1").arg(it.value()->str());
            space->m_crslice_model.setParameter(it.key().toStdString(), it.value()->str().toStdString());
        }

        QString out_file = attain->tempGCodeFileName();
        crslice2::CacheSliceParam param;
        param.outName = cxkernel::qString2String(out_file);
        param.tempDirectory = generateTempFileName().toLocal8Bit().data();
#if  0
        param.fileName = sceneTempFile().toLocal8Bit().data();
#endif

        QString gcodeFileName = "";
        if (onlySliceCacheEnabled())
        {
            QFileInfo info(out_file);
            QString blName =  info.completeBaseName();

            QString _projectName =  projectPath();
            QFileInfo projectInfo(_projectName);
            blName = projectInfo.suffix() + "_" + projectInfo.completeBaseName();

            QString cachefile = slicerCacheDir() + "/" + blName + ".cxcache";

            param.fileName = cachefile.toLocal8Bit().data();
            gcodeFileName = slicerGCodeDir() + "/" + blName + ".gcode";
            param.tempDirectory = slicerCacheDir().toLocal8Bit().data();
        }


        if(slicerUnitType() > 0){
            //BaseLine Test
            param.baselineType = slicerUnitType();
            QString slicerBLPath = slicerBaselinePath();
            param.blDir = (slicerBLPath).toLocal8Bit().data();

            QString slicerComparePath = slicerCompErrorPath();
            param.resultDir = ((slicerComparePath).toLocal8Bit().data());

            QFileInfo info(out_file);
            QString blName =  info.completeBaseName();

            QString _projectName =  projectPath();
            QFileInfo projectInfo(_projectName);
            blName = projectInfo.suffix() + "_" + projectInfo.completeBaseName();
            const std::string _blname = blName.toLocal8Bit().constData();
            param.blName = (_blname);
        }
        
        crslice2::CrSliceResult result = crslice2::slice(printer->m_crslice_print, space->m_crslice_model, thumbnails,
            param, tracer);
        if (onlySliceCacheEnabled())
        {
             qtuser_core::copyFile(out_file, gcodeFileName, true);
        }
        if (attain)
        {
            QMap<QString, QPair<QString, int64_t> > slice_messages;
            for (std::map<std::string, std::pair<std::string, int64_t>>::iterator it = result.warnings.begin();
                it != result.warnings.end(); ++it) {
                slice_messages.insert(QString::fromUtf8(it->first.c_str()), qMakePair(QString::fromUtf8(it->second.first.c_str()), it->second.second));
            }
            attain->setSliceWarningDetails(slice_messages);
        }
	}
}
