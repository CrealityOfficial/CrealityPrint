#include "plane.h"

namespace qtuser_3d
{
	Plane::Plane()
		:center(0.0f, 0.0f, 0.0f)
		, dir(0.0f, 0.0f, 1.0f)
	{

	}

	Plane::Plane(const QVector3D& center)
		:center(center)
		, dir(0.0f, 0.0f, 1.0f)
	{

	}

	Plane::Plane(const QVector3D& center, const QVector3D& ndir)
		:center(center)
		,dir(ndir)
	{

	}

	Plane::~Plane()
	{
	}
}
