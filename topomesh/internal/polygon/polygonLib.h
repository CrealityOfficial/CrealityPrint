#ifndef _POLYGONLIB_H
#define _POLYGONLIB_H
#include <vector>
#include "clipper/clipper.hpp"

namespace topomesh
{
	struct PointF
	{
		float x;
		float y;

		PointF()
		{
			x = 0;
			y = 0;
		}

		PointF(float x_, float y_)
		{
			x = x_;
			y = y_;
		}
	};

	class PolygonPro
	{
	public:
		PolygonPro();
		~PolygonPro();

	public:
		static ClipperLib::Path polygonSimplyfy(const ClipperLib::Path& input, double epsilon = 1.0);
		static ClipperLib::Paths polygonSimplyfy(const ClipperLib::Paths& input, double epsilon = 1.0);

		static ClipperLib::Path polygonConcaveHull(const ClipperLib::Path& input, double chi_factor = 0.1);
		static ClipperLib::Paths polygonConcaveHull(const ClipperLib::Paths& input, double chi_factor = 0.1);
		static std::vector<PointF> polygonConcaveHull(const std::vector<PointF> input, double chi_factor = 0.1);

		static ClipperLib::Path polygonsConcaveHull(const ClipperLib::Paths& input, double chi_factor = 0.1);

		static std::vector<PointF> polygonConvexHull(const std::vector<PointF> input);
		static ClipperLib::Path polygonConvexHull(const ClipperLib::Path& input);

		static ClipperLib::Path polygonsConvexHull(const ClipperLib::Paths& input);
	};


}



#endif