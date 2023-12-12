#ifndef _qtuser_3d_BOX3D_1588062165691_H
#define _qtuser_3d_BOX3D_1588062165691_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QVector3D>
namespace qtuser_3d
{
	class QTUSER_3D_API Box3D
	{
	public:
		// Construct as empty
		Box3D();
		Box3D(const QVector3D& v);
		Box3D(const QVector3D& vmin, const QVector3D& vmax);

		// Mark invalid
		void clear();
		QVector3D center() const;
		QVector3D size() const;

		float radius() const;

		// Grow a bounding box to encompass a point or another Box
		Box3D& operator += (const QVector3D& point);
		Box3D& operator += (const Box3D& box);

		inline friend const Box3D operator + (const Box3D& box, const QVector3D& point)
		{
			return Box3D(box) += point;
		}
		inline friend const Box3D operator + (const QVector3D& point, const Box3D& box)
		{
			return Box3D(box) += point;
		}
		inline friend const Box3D operator + (const Box3D& box1, const Box3D& box2)
		{
			return Box3D(box1) += box2;
		}

		// Does a Box contain, or at least touch, a point?
		bool contains(const QVector3D& point) const;

		// Does a Box contain another Box?
		bool contains(const Box3D& b) const;

		// Do two Boxes intersect, or at least touch?
		bool intersects(const Box3D& box);

		void translate(const QVector3D& t);
	public:
		// Public (!) members
		QVector3D min;
		QVector3D max;
		bool valid;
	};
}

// Equality and inequality.  An invalid box compares unequal to anything.
static inline bool operator == (const qtuser_3d::Box3D& box1, const qtuser_3d::Box3D& box2)
{
	return box1.valid && box2.valid && box1.min == box2.min && box1.max == box2.max;
}

static inline bool operator != (const qtuser_3d::Box3D& box1, const qtuser_3d::Box3D& box2)
{
	return !(box1 == box2);
}

// (In-)validity testing
static inline bool operator ! (const qtuser_3d::Box3D& box)
{
	return !box.valid;
}

#endif // _qtuser_3d_BOX3D_1588062165691_H
