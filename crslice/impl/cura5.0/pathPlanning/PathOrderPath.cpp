//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "PathOrderPath.h" //The definitions we're implementing here.
#include "communication/sliceDataStorage.h"

namespace cura52
{

    template<>
    ConstPolygonRef PathOrderPath<ConstPolygonPointer>::getVertexData()
    {
        return *vertices;
    }

    template<>
    ConstPolygonRef PathOrderPath<PolygonPointer>::getVertexData()
    {
        return *vertices;
    }

    template<>
    ConstPolygonRef PathOrderPath<const SkinPart*>::getVertexData()
    {
        return vertices->outline.outerPolygon();
    }

    template<>
    ConstPolygonRef PathOrderPath<const SliceLayerPart*>::getVertexData()
    {
        return vertices->outline.outerPolygon();
    }

    template<>
    ConstPolygonRef PathOrderPath<const SupportInfillPart*>::getVertexData()
    {
        return vertices->outline.outerPolygon();
    }
    template<>
    ConstPolygonRef PathOrderPath<const ExtrusionLine*>::getVertexData()
    {
        if ( ! cached_vertices)
        {
            cached_vertices = vertices->toPolygon();
        }
        return ConstPolygonRef(*cached_vertices);
    }

    template<>
    int PathOrderPath<ConstPolygonPointer>::startIdx()
    {
        return -1;
    }

    template<>
    int PathOrderPath<PolygonPointer>::startIdx()
    {
        return -1;
    }

    template<>
    int PathOrderPath<const SkinPart*>::startIdx()
    {
        return -1;
    }

    template<>
    int PathOrderPath<const SliceLayerPart*>::startIdx()
    {
        return -1;
    }

    template<>
    int PathOrderPath<const SupportInfillPart*>::startIdx()
    {
        return -1;
    }
    template<>
    int PathOrderPath<const ExtrusionLine*>::startIdx()
    {
        return vertices->start_idx;
    }


	template<>
	bool PathOrderPath<ConstPolygonPointer>::hasZseamPait()
	{
		return false;
	}

	template<>
    bool PathOrderPath<PolygonPointer>::hasZseamPait()
	{
		return false;
	}

	template<>
    bool PathOrderPath<const SkinPart*>::hasZseamPait()
	{
		return false;
	}

	template<>
    bool PathOrderPath<const SliceLayerPart*>::hasZseamPait()
	{
		return false;
	}

	template<>
    bool PathOrderPath<const SupportInfillPart*>::hasZseamPait()
	{
		return false;
	}

	template<>
	bool PathOrderPath<const ExtrusionLine*>::hasZseamPait()
	{
        for (const ExtrusionJunction& J: vertices->junctions)
        {
            if (J.flag == ExtrusionJunction::paintFlag::PAINT)
            {
                return true;
            }
        }
		return false;
	}

}
