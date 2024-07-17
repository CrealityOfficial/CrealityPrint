#ifndef _POLYGONLIB_H
#define _POLYGONLIB_H
#include <vector>
#include "clipper3r/clipper.hpp"


namespace polygonLib
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
		static Clipper3r::Path polygonSimplyfy(const Clipper3r::Path& input, double epsilon = 1.0);
		static Clipper3r::Paths polygonSimplyfy(const Clipper3r::Paths& input, double epsilon = 1.0);
        static bool PointInPolygon(const Clipper3r::IntPoint& pt, const Clipper3r::Path& input);
        static Clipper3r::Path polygonConvexHull2(const Clipper3r::Path& input, bool clockwise = false);

		static Clipper3r::Path polygonConcaveHull(const Clipper3r::Path& input, double chi_factor = 0.1);
		static Clipper3r::Paths polygonConcaveHull(const Clipper3r::Paths& input, double chi_factor = 0.1);
		static std::vector<PointF> polygonConcaveHull(const std::vector<PointF> input, double chi_factor = 0.1);

		static Clipper3r::Path polygonsConcaveHull(const Clipper3r::Paths& input, double chi_factor = 0.1);

		static std::vector<PointF> polygonConvexHull(const std::vector<PointF> input);
		static Clipper3r::Path polygonConvexHull(const Clipper3r::Path& input);

		static Clipper3r::Path polygonsConvexHull(const Clipper3r::Paths& input);
	};


}



#endif