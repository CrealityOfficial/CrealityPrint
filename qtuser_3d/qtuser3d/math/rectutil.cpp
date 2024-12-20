#include "rectutil.h"

namespace qtuser_3d
{
	int adjustY(int y, const QSize& size)
	{
		int ay = size.height() - y;
		if (ay < 0) ay = 0;
		if (ay > size.height()) ay = size.height();
		return ay;
	}

	float viewportRatio(int x, int w)
	{
		return (float)x / (float)w - 0.5f;
	}

	QPointF viewportRatio(const QPoint p, const QSize& size)
	{
		QPointF pf;
		pf.setX((float)p.x() / (float)size.width() - 0.5f);
		pf.setY((float)(size.height() - p.y()) / (float)size.height() - 0.5f);
		return pf;
	}
}
