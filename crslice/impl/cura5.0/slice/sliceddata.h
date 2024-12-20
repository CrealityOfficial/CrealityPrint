//  Copyright (c) 2018-2022 Ultimaker B.V.
//  CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef SLICEDDATE_CONTEXT_H
#define SLICEDDATE_CONTEXT_H
#include "utils/polygon.h"

namespace cura52
{
	class Mesh;
	struct SlicedLayer
	{
		int z = -1;
		Polygons polygons;
		Polygons openPolylines;
	};

	class SlicedData
	{
	public:
		SlicedData();
		~SlicedData();

		std::vector<SlicedLayer> layers;

		Mesh* mesh = nullptr; // for convinient
	};
} //namespace cura52

#endif //SLICEDDATE_CONTEXT_H

