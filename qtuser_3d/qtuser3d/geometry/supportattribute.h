#ifndef QTUSER_3D_SUPPORT_ATTRIBUTE_H
#define QTUSER_3D_SUPPORT_ATTRIBUTE_H
#include <QtCore/QByteArray>
#include <qtuser3d/geometry/attribute.h>
#include "qtuser3d/module/glcompatibility.h"

namespace qtuser_3d
{
	// used for support chunk entity
	struct SupportAttributeShade
	{
		SupportAttributeShade()
		{
			vertexSize = 3;
			if (qtuser_3d::isGles())
			{
				vertexSize = 4;
			}

		}

		int vertexSize;
		AttributeShade positionAttribute;
		AttributeShade normalAttribute;
		AttributeShade flagAttribute;

	};
}

#endif