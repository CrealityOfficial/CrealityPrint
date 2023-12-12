#include "ray.h"

namespace qtuser_3d
{
	Ray::Ray()
		:start(0.0f, 0.0f, 0.0f)
		,dir(0.0f, 0.0f, 1.0f)
	{

	}

	Ray::Ray(const QVector3D& vstart)
		:start(vstart)
		,dir(0.0f, 0.0f, 1.0f)
	{

	}

	Ray::Ray(const QVector3D& vstart, const QVector3D& ndir)
		:start(vstart)
		,dir(ndir)
	{

	}

	Ray::~Ray()
	{
	}
}
