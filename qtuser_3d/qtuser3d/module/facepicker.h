#ifndef FACEPICKER_1595464314037_H
#define FACEPICKER_1595464314037_H
#include <QtCore/QPoint>

namespace qtuser_3d
{
	class FacePicker
	{
	public:
		virtual ~FacePicker() {}

		virtual bool pick(int x, int y, int* faceID) = 0;
		virtual bool pick(const QPoint& point, int* faceID) = 0;
		virtual void sourceMayChanged() = 0;
	};
}
#endif // FACEPICKER_1595464314037_H