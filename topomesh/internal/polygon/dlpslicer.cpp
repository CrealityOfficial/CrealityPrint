#include "dlpslicer.h"
#include "dlpinput.h"

#include "meshslice.h"
#include "slicepolygonbuilder.h"
#include "openpolygonprocessor.h"
#include <assert.h>

#ifdef _OPENMP
#include "omp.h"
#endif

namespace topomesh
{
	DLPSlicer::DLPSlicer()
	{

	}

	DLPSlicer::~DLPSlicer()
	{

	}

	bool DLPSlicer::compute(const DLPInput& input, DLPData& data, ccglobal::Tracer* tracer)
	{
#ifdef _OPENMP
		omp_set_num_threads(omp_get_num_procs());
#endif

		SlicedResult result;
		if (!sliceInput(input, result, tracer))
			return false;

		int layerCount = result.layerCount();
		if (layerCount <= 0)
			return false;

		data.impl->layersData.resize(layerCount);
		for (SlicedMesh& slicedMesh : result.slicedMeshes)
		{
#pragma omp parallel for
			for (int layer_nr = 0; layer_nr < static_cast<int>(layerCount); layer_nr++)
			{
				SlicedMeshLayer& layer = slicedMesh.m_layers.at(layer_nr);
				DLPLayer& dlpplayer = data.impl->layersData.at(layer_nr);
				dlpplayer.printZ = layer.z;
				dlpplayer.polygons = dlpplayer.polygons.unionPolygons(layer.polygons);
			}
		}

		return true;
	}

    OneLayerSlicer::OneLayerSlicer(MeshObjectPtr mesh)
        :m_mesh(mesh)
    {
        m_helper.prepare(mesh.get());
    }

    OneLayerSlicer::~OneLayerSlicer()
    {

    }

    bool OneLayerSlicer::compute(float z, DLPDebugger* debugger)
    {
        topomesh::coord_t iz = MM2INT(z);
        SlicePolygonBuilder builder;
        m_helper.sliceOneLayer(iz, builder.segments, builder.face_idx_to_segment_idx);

        Polygons polygons;
        Polygons openPolygons;
        builder.makePolygon(&polygons, &openPolygons);

        ClipperLib::Path intersectPoints;
        if (openPolygons.size())
        {
            builder.connectOpenPolylines(polygons,openPolygons, intersectPoints);
        }

        //connect 1
        Polygons closedPolygons;
        connectOpenPolygons(openPolygons, closedPolygons);
        for (size_t k = 0; k < closedPolygons.size(); ++k)
        {
            polygons.add(closedPolygons[k]);
        }

        //connect 2
        Polygons stitchClosedPolygons;
        stitch(openPolygons, stitchClosedPolygons);
        for (size_t k = 0; k < stitchClosedPolygons.size(); ++k)
        {
            polygons.add(stitchClosedPolygons[k]);
        }

        if (false)
        {
            stitchExtensive(openPolygons, polygons);
        }

        if (false)
        {
            for (PolygonRef polyline : openPolygons)
            {
                if (polyline.size() > 0)
                    polygons.add(polyline);
            }
        }

        Polygons resultPolygons;
        for (PolygonRef polyline : openPolygons)
        {
            if (polyline.size() > 0)
            {
                resultPolygons.add(polyline);
            }
        }
        openPolygons = resultPolygons;

        coord_t line_segment_resolution = 50;
        coord_t line_segment_deviation = 50;
        float xy_offset = 0.00f;
        bool enable_xy_offset = true;
        polygons.simplify(line_segment_resolution, line_segment_deviation);
        polygons.removeDegenerateVerts(); // remove verts connected to overlapping line segments
        if (enable_xy_offset && abs(xy_offset) > 0.000001)
        {
            polygons = polygons.offset((int)(xy_offset * 1000));
        }

        polygons = polygons.unionPolygons();

        if (debugger)
        {
            debugger->onConnected(polygons, openPolygons, intersectPoints);
        }

        return true;
    }
}