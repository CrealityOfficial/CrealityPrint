#ifndef CXKERNEL_CORE_ATTRIBUTE_1658112808251_H
#define CXKERNEL_CORE_ATTRIBUTE_1658112808251_H
#include "cxkernel/cxkernelinterface.h"
#include <QtCore/QByteArray>

namespace cxkernel
{
	class CXKERNEL_API GeometryData
	{
	public:
		GeometryData();
		virtual ~GeometryData();

	public:
		QByteArray position;
		QByteArray normal;
		QByteArray color;
		QByteArray texcoord;
		QByteArray indices;

		int fcount;
		int vcount;
		int indiceCount;
	};
}

#endif // CXKERNEL_CORE_ATTRIBUTE_1658112808251_H