#include "meshslice.h"
#include "slicepolygonbuilder.h"
#include "openpolygonprocessor.h"

#include "slicehelper.h"

#include "ccglobal/tracer.h"

namespace topomesh
{
	SlicedMeshLayer::SlicedMeshLayer()
		:z(0)
	{

	}

	SlicedMeshLayer::~SlicedMeshLayer()
	{

	}

	SlicedMesh::SlicedMesh()
	{

	}

	SlicedMesh::~SlicedMesh()
	{

	}

	SlicedResult::SlicedResult()
	{

	}

	SlicedResult::~SlicedResult()
	{

	}

	void SlicedResult::save(const std::string& prefix)
	{
	}

	void SlicedResult::connect()
	{
		for (SlicedMesh& slicedMesh : slicedMeshes)
		{
			SlicePolygonBuilder builder;
			int layerCount = (int)slicedMesh.m_layers.size();
#pragma omp parallel for
			for (int j = 0; j < (int)layerCount; ++j)
			{
				SlicedMeshLayer& layer = slicedMesh.m_layers.at(j);

				Polygons closedPolygons;
				if (layer.openPolylines.size() > 0)
					connectOpenPolygons(layer.openPolylines, closedPolygons);

				for (size_t k = 0; k < closedPolygons.size(); ++k)
				{
					layer.polygons.add(closedPolygons[k]);
				}

				{
					Polygons _openPolygons;
					for (auto poly : layer.openPolylines)
					{
						if (poly.size())
						{
							_openPolygons.add(poly);
						}
					}
					layer.openPolylines.clear();
					layer.openPolylines = _openPolygons;
					_openPolygons.clear();

					layer.openPolylines.removeDegenerateVerts(); // remove verts connected to overlapping line segments
					builder.removeSamePoint(layer.openPolylines);


					_openPolygons.clear();
					for (auto poly : layer.openPolylines)
					{
						if (poly.size())
						{
							_openPolygons.add(poly);
						}
					}
					layer.openPolylines.clear();
					layer.openPolylines = _openPolygons;
					_openPolygons.clear();

					layer.openPolylines.removeDegenerateVerts();

				}

				{
					Polygons stitchClosedPolygons;
					stitch(layer.openPolylines, stitchClosedPolygons);

					for (size_t k = 0; k < stitchClosedPolygons.size(); ++k)
					{
						layer.polygons.add(stitchClosedPolygons[k]);
					}
				}

				if (true)
				{
					stitchExtensive(layer.openPolylines, layer.polygons);
				}

				if (false)
				{
					for (PolygonRef polyline : layer.openPolylines)
					{
						if (polyline.size() > 0)
							layer.polygons.add(polyline);
					}
				}

				{
					//connect 1
					if (layer.openPolylines.size())
					{
						Polygons connected;
						builder.connectOpenPolylines(connected, layer.openPolylines);
						for (size_t k = 0; k < connected.size(); ++k)
						{
							layer.polygons.add(connected[k]);
						}
					}
				}

				Polygons resultPolygons;
				for (PolygonRef polyline : layer.openPolylines)
				{
					if (polyline.size() > 0)
					{
						resultPolygons.add(polyline);
					}
				}
				layer.openPolylines = resultPolygons;
			}
		}
	}

	void SlicedResult::simplify(coord_t line_segment_resolution, coord_t line_segment_deviation,
		float xy_offset, bool enable_xy_offset)
	{
		for (SlicedMesh& slicedMesh : slicedMeshes)
		{
			SlicePolygonBuilder builder;
			int layerCount = (int)slicedMesh.m_layers.size();
#pragma omp parallel for
			for (int j = 0; j < (int)layerCount; ++j)
			{
				SlicedMeshLayer& layer = slicedMesh.m_layers.at(j);
				//const coord_t snap_distance = std::max(param.minimum_polygon_circumference, static_cast<coord_t>(1));
				//auto it = std::remove_if(layer.polygons.begin(), layer.polygons.end(), [snap_distance](PolygonRef poly) {
				//	return poly.shorterThan(snap_distance);
				//	});
				//layer.polygons.erase(it, layer.polygons.end());

				//Finally optimize all the polygons. Every point removed saves time in the long run.
				layer.polygons.simplify(line_segment_resolution, line_segment_deviation);
				layer.polygons.removeDegenerateVerts(); // remove verts connected to overlapping line segments

				if (enable_xy_offset && abs(xy_offset) > 0.000001)
				{
					layer.polygons = layer.polygons.offset(DLP_MM2_S(xy_offset));
				}

				//合并闭合轮廓
				//layer.polygons = layer.polygons.unionPolygons();

				//ClipperLib::Path intersectPoints;
				//if (layer.openPolylines.paths.size())
				//{
				//	builder.connectOpenPolylines(layer.polygons, layer.openPolylines, intersectPoints);
				//}
			}
		}
	}

	int SlicedResult::meshCount()
	{
		return (int)slicedMeshes.size();
	}

	int SlicedResult::layerCount()
	{
		if (meshCount() > 0)
			return (int)slicedMeshes.at(0).m_layers.size();
		return 0;
	}

	void sliceMeshes(std::vector<MeshObject*>& meshes, std::vector<SlicedMesh>& slicedMeshes, std::vector<int>& z)
	{
		size_t meshCount = meshes.size();
		if (meshCount > 0)
		{
			if (slicedMeshes.size() != meshCount)
				slicedMeshes.resize(meshCount);

			for (size_t i = 0; i < meshCount; ++i)
				sliceMesh(meshes.at(i), slicedMeshes.at(i), z);
		}
	}

	void sliceMesh(MeshObject* mesh, SlicedMesh& slicedMesh, std::vector<int>& z)
	{
		size_t size = z.size();
		if (size > 0)
		{
			slicedMesh.m_layers.resize(size);

			SliceHelper helper;
			helper.prepare(mesh);

#pragma omp parallel for
			for (int i = 0; i < size; ++i)
				sliceMesh(mesh, slicedMesh.m_layers.at(i), z.at(i), &helper);
		}
	}

	void sliceMesh(MeshObject* mesh, SlicedMeshLayer& slicedMeshLayer, int z, SliceHelper* helper)
	{
		SlicePolygonBuilder builder;
		helper->sliceOneLayer(z, builder.segments, builder.face_idx_to_segment_idx);

		builder.makePolygon(&slicedMeshLayer.polygons, &slicedMeshLayer.openPolylines);

		//{//连接开多边形
		//    ClipperLib::Path intersectPoints;
		//    if (slicedMeshLayer.openPolylines.size())
		//    {
		//        builder.connectOpenPolylines(slicedMeshLayer.polygons, slicedMeshLayer.openPolylines, intersectPoints);
		//    }
		//}

		slicedMeshLayer.z = z;
	}

	void sliceMeshes_src(const std::vector<trimesh::TriMesh*>& meshes, std::vector<SlicedMesh>& slicedMeshes, std::vector<int>& z)
	{
		size_t meshCount = meshes.size();
		if (meshCount > 0)
		{
			if (slicedMeshes.size() != meshCount)
				slicedMeshes.resize(meshCount);

			for (size_t i = 0; i < meshCount; ++i)
				sliceMesh_src(meshes.at(i), slicedMeshes.at(i), z);
		}
	}


	void sliceMesh_src(trimesh::TriMesh* mesh, SlicedMesh& slicedMesh, std::vector<int>& z)
	{
		size_t size = z.size();
		if (size > 0)
		{
			slicedMesh.m_layers.resize(size);

			SliceHelper helper;
			helper.prepare(mesh);

#pragma omp parallel for
			for (int i = 0; i < size; ++i)
				sliceMesh_src(slicedMesh.m_layers.at(i), z.at(i), &helper);
		}
	}


	void sliceMesh_src(SlicedMeshLayer& slicedMeshLayer, int z, SliceHelper* helper)
	{
		SlicePolygonBuilder builder;
		builder.sliceOneLayer_dst(helper, z, &slicedMeshLayer.polygons, &slicedMeshLayer.openPolylines);
	}

	void buildSliceInfos(const DLPInput& input, std::vector<int>& z)
	{
		const DLPParam& param = input.Param;

		int slice_layer_count = 0;
		const coord_t initial_layer_thickness = param.initial_layer_thickness;
	    coord_t initial_layer_count = param.initial_layer_count;
		const coord_t layer_thickness = param.layer_thickness;

		AABB3D box = input.box();
		slice_layer_count = ceil((box.max.z - initial_layer_thickness) / (layer_thickness * 1.0)) + 1;

		if (slice_layer_count > 0)
		{
			z.resize(slice_layer_count, 0);
			if (slice_layer_count < initial_layer_count)
			{
				initial_layer_count = slice_layer_count;
			}
			z[0] = std::max(0LL, initial_layer_thickness - layer_thickness);
			coord_t adjusted_layer_offset = initial_layer_thickness;

			//inital_layer z positions 
			for (int initLayer = 0; initLayer < initial_layer_count; initLayer++)
			{
				z[initLayer] = adjusted_layer_offset + (layer_thickness * (initLayer));
			}
			adjusted_layer_offset = initial_layer_thickness * initial_layer_count + (layer_thickness / 2);

			// define all layer z positions other than inital_layer (depending on slicing mode, see above)
			for (int layer_nr = initial_layer_count; layer_nr < slice_layer_count; layer_nr++)
			{
				z[layer_nr] = adjusted_layer_offset + (layer_thickness * (layer_nr - initial_layer_count));
			}
		}
	}

	bool sliceInput(const DLPInput& input, SlicedResult& result, ccglobal::Tracer* tracer)
	{
		int meshCount = input.meshCount();
		if (meshCount == 0)
			return false;

		std::vector<int> z;
		buildSliceInfos(input, z);

		size_t layerCount = z.size();
		if (layerCount == 0)
			return false;

		if (tracer)
		{
			tracer->progress(0.3f);
			if (tracer->interrupt())
			{
				tracer->progress(1.0f);
				return false;
			}
		}

		const std::vector<MeshObjectPtr>& meshptres = input.Meshes;
		std::vector<MeshObject*> meshes;
		for (size_t i = 0; i < meshCount; ++i)
		{
			meshes.push_back(&*meshptres.at(i));
		}

		sliceMeshes(meshes, result.slicedMeshes, z);
		result.save("sliced");

		if (tracer)
		{
			tracer->progress(0.6f);
			if (tracer->interrupt())
			{
				tracer->progress(1.0f);
				return false;
			}
		}

		const DLPParam& param = input.Param;
		//////////
		result.connect();
		result.save("connected");

		if (tracer)
		{
			tracer->progress(0.7f);
			if (tracer->interrupt())
			{
				tracer->progress(1.0f);
				return false;
			}
		}

		const coord_t line_segment_resolution = param.line_segment_resolution;
		const coord_t line_segment_deviation = param.line_segment_deviation;
		float xy_offset = param.xy_offset;
		bool enable_xy_offset = param.enable_xy_offset;
		result.simplify(line_segment_resolution, line_segment_deviation, xy_offset, enable_xy_offset);

		if (tracer)
		{
			tracer->progress(0.9f);
			if (tracer->interrupt())
			{
				tracer->progress(1.0f);
				return false;
			}
		}

		result.save("result");
		return true;
	}
}

