#include "MESHTOOL/UnitConverter.hpp"

namespace MESHTOOL {

	UnitConverter::UnitConverter() {}

	float UnitConverter::getFactor() const
	{
		return factor;
	}

	float UnitConverter::toOutside(const GEOM::GeomNumType& insideValue) const
	{
		return static_cast<float>(insideValue) / factor;
	}

	std::array<float, 3> UnitConverter::toOutside(const GEOM::Point& point) const
	{
		std::array<float, 3> pFloat;
		pFloat.at(0) = toOutside(point.x);
		pFloat.at(1) = toOutside(point.y);
		pFloat.at(2) = toOutside(point.z);
		return pFloat;
	}

	GEOM::GeomNumType UnitConverter::toInside(const float& outsideValue) const
	{
		return static_cast<GEOM::GeomNumType>(std::floor(outsideValue * factor));
	}

	GEOM::Point UnitConverter::toInside(const trimesh::point& point) const
	{
		GEOM::Point p(toInside(point.at(0)), toInside(point.at(1)), toInside(point.at(2)));
		return p;
	}

}  // namespace MESHTOOL