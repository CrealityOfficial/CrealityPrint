#include "topomesh/interface/interactiveslice.h"
#include "internal/polygon/polygon.h"
#include "internal/polygon/meshslice.h"

namespace topomesh
{
	class InteractiveSliceImpl
	{
	public:
		InteractiveSliceImpl() {

		}

		~InteractiveSliceImpl() {

		}

        std::vector<Polygons> cache;
	};

	InteractiveSlice::InteractiveSlice()
		:impl(new InteractiveSliceImpl())
	{

	}

	InteractiveSlice::~InteractiveSlice()
	{

	}

	void InteractiveSlice::reset(const std::vector<trimesh::TriMesh*>& meshes)
	{
        impl->cache.clear();

        trimesh::box b;
        for (trimesh::TriMesh* mesh : meshes)
        {
            mesh->need_bbox();
            b += mesh->bbox;
        }

        std::vector<int> z;

        int height = b.max.z - b.min.z;
        const int layer = 10;
        float per = height * 1.0f / layer;
        for (size_t i = 1; i <= layer; i++)
        {
            int h = per * i;
            if (h > 0)
            {
                z.push_back(MM2INT(per * i));
            }
        }

        std::vector<SlicedMesh> slicedMeshes;
        sliceMeshes_src(meshes, slicedMeshes, z);

        //过滤掉有开区间的层
        bool haveClosedLayer = false;
        for (auto& smesh : slicedMeshes)
        {
            for (auto& poly : smesh.m_layers)
            {
                if (!poly.polygons.empty() && poly.openPolylines.empty())
                {
                    haveClosedLayer = true;
                    break;
                }
            }
        }


        for (auto& smesh : slicedMeshes)
        {
            Polygons polys;
            for (auto& poly : smesh.m_layers)
            {
                if (haveClosedLayer && !poly.openPolylines.empty())
                {
                    continue;
                }
                //polys = polys.unionPolygons(poly.polygons);
                polys.add(poly.polygons);
            }
            polys = polys.unionPolygons();
            if (!polys.paths.empty())
            {
                impl->cache.push_back(polys);
            }
        }
	}

	void InteractiveSlice::contourSplit(const std::vector<std::vector<trimesh::vec2>>& contours, float _interval)
	{
        int interval = MM2INT(_interval);
                
        Polygons ploygon;
        for (const std::vector<trimesh::vec2>& contour : contours)
        {
            ClipperLib::Path path;
            for (const trimesh::vec2& point : contour)
            {
                path.push_back(ClipperLib::IntPoint(MM2INT(point.x), MM2INT(point.y)));
            }
            if (!path.empty())
            {
                ploygon.add(path);
            }
        }
        
        auto getPloygons = [&interval](Polygons& ploygon)
        {
            ClipperLib::ClipperOffset clipper;
            clipper.AddPaths(ploygon.paths, ClipperLib::JoinType::jtSquare, ClipperLib::etOpenSquare);
            clipper.Execute(ploygon.paths, interval);
        };
        getPloygons(ploygon);
        
        
        std::vector<Polygons> _vctPolys = impl->cache;
        for (auto& poly : _vctPolys)
        {
            poly = poly.difference(ploygon);
        }
        
        impl->cache.clear();

        std::vector<PolygonsPart> splitIntoParts;
        for (auto& poly : _vctPolys)
        {
            std::vector<PolygonsPart> _splitIntoParts;
            _splitIntoParts = poly.splitIntoParts();
        
            impl->cache.insert(impl->cache.end(), _splitIntoParts.begin(), _splitIntoParts.end());
        }
	}

	void InteractiveSlice::getCurrentContours(std::vector<Contour>& coutours)
	{
        coutours.clear();

        auto f = [](const Polygons& poly, Contour& contour) {
            contour.clear();
            for (auto& path : poly.paths)
            {
                Path _path;
                _path.solid = ClipperLib::Orientation(path);
                for (auto& point : path)
                {
                    _path.line.push_back(trimesh::vec2(INT2MM(point.X), INT2MM(point.Y)));
                }
                contour.push_back(_path);
            }
        };

        for (const Polygons& poly : impl->cache)
        {
            Contour contour;
            f(poly, contour);
            coutours.push_back(contour);
        }
	}
}