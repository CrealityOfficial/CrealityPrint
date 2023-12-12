#include "box3d.h"

namespace qtuser_3d
{
	Box3D::Box3D()
		: valid(false)
	{}

	// Construct from a single point
	Box3D::Box3D(const QVector3D& v)
		: min(v), max(v), valid(true)
	{}

	// Construct from two points
	Box3D::Box3D(const QVector3D& vmin, const QVector3D& vmax)
		: min(vmin), max(vmax), valid(true)
	{
		using namespace ::std;
		for (int i = 0; i < 3; i++) {
			if (min[i] > max[i])
				swap(min[i], max[i]);
		}
	}

	// Mark invalid
	void Box3D::clear()
	{
		valid = false;
	}

	// Return center point, (vector) diagonal, and (scalar) radius
	QVector3D Box3D::center() const
	{
		if (Q_UNLIKELY(!valid))
			return QVector3D();
		return 0.5f * (min + max);
	}

     QVector3D Box3D::size() const
	{
		if (Q_UNLIKELY(!valid))
			return QVector3D();
		return max - min;
	}

	float Box3D::radius() const
	{
		if (Q_UNLIKELY(!valid))
			return 0;
		return 0.5f * (min - max).length();
	}

	// Grow a bounding box to encompass a point or another Box
	Box3D& Box3D::operator += (const QVector3D& point)
	{
		if (Q_UNLIKELY(valid))
		{
			for (int i = 0; i < 3; i++)
			{
				if (point[i] < min[i])
					min[i] = point[i];
				else if (point[i] > max[i])
					max[i] = point[i];
			}
		}
		else
		{
			min = point;
			max = point;
			valid = true;
		}
		return *this;
	}

	Box3D& Box3D::operator += (const Box3D& box)
	{
		if (Q_UNLIKELY(valid))
		{
			for (int i = 0; i < 3; i++)
			{
				if (box.min[i] < min[i])
					min[i] = box.min[i];
				if (box.max[i] > max[i])
					max[i] = box.max[i];
			}
		}
		else
		{
			min = box.min;
			max = box.max;
			valid = true;
		}
		return *this;
	}

	// Does a Box contain, or at least touch, a point?
	bool Box3D::contains(const QVector3D& point) const
	{
		if (Q_UNLIKELY(!valid))
			return false;
		for (int i = 0; i < 3; i++)
		{
			if (point[i] < min[i] || point[i] > max[i])
				return false;
		}
		return true;
	}

	// Does a Box contain another Box?
	bool Box3D::contains(const Box3D& box) const
	{
		if (Q_UNLIKELY(!valid || !box.valid))
			return false;
		for (int i = 0; i < 3; i++)
		{
			if (box.min[i] < min[i] || box.max[i] > max[i])
				return false;
		}
		return true;
	}

	// Do two Boxes intersect, or at least touch?
	bool Box3D::intersects(const Box3D& box)
	{
		if (Q_UNLIKELY(!valid || !box.valid))
			return false;
		for (int i = 0; i < 3; i++)
		{
			if (box.max[i] < min[i] || box.min[i] > max[i])
				return false;
		}
		return true;
	}

	void Box3D::translate(const QVector3D& t)
	{
		if (Q_UNLIKELY(!valid))
			return;

		min += t;
		max += t;
	}
}
