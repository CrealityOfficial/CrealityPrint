#ifndef QTUSER_3D_GEOMETRYCREATEHELPER_1594863285229_H
#define QTUSER_3D_GEOMETRYCREATEHELPER_1594863285229_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/geometry/attribute.h"
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>

namespace qtuser_3d
{
	class QTUSER_3D_API GeometryCreateHelper : public QObject
	{
		Q_OBJECT
	public:
		GeometryCreateHelper(QObject* parent = nullptr);
		virtual ~GeometryCreateHelper();

		static Qt3DRender::QGeometry* create(Qt3DCore::QNode* parent = nullptr, Qt3DRender::QAttribute* attribute0 = nullptr, Qt3DRender::QAttribute* attribute1 = nullptr, Qt3DRender::QAttribute* attribute2 = nullptr, 
			Qt3DRender::QAttribute* attribute3 = nullptr);

		static Qt3DRender::QGeometry* createEx(Qt3DCore::QNode* parent = nullptr, Qt3DRender::QAttribute* attribute0 = nullptr, Qt3DRender::QAttribute* attribute1 = nullptr, Qt3DRender::QAttribute* attribute2 = nullptr,
			Qt3DRender::QAttribute* attribute3 = nullptr, Qt3DRender::QAttribute* attribute4 = nullptr, Qt3DRender::QAttribute* attribute5 = nullptr);

		static Qt3DRender::QGeometry* create(Qt3DCore::QNode* parent = nullptr, AttributeShade* attribute1 = nullptr,
			AttributeShade* attribute2 = nullptr, AttributeShade* attribute3 = nullptr,
			AttributeShade* attribute4 = nullptr, AttributeShade* attribute5 = nullptr);
		static Qt3DRender::QBuffer* createBuffer(AttributeShade* attribute = nullptr);

		static Qt3DRender::QGeometry* create(int vCount, const QByteArray& position, const QByteArray& normal,
			const QByteArray& texcoord, const QByteArray& color, Qt3DCore::QNode* parent = nullptr);
		
		static Qt3DRender::QGeometry* indexCreate(int vCount, const QByteArray& position,
			int iCount, const QByteArray& indices, Qt3DCore::QNode* parent = nullptr);
		static Qt3DRender::QGeometry* indexCreate(int vCount, const QByteArray& position, const QByteArray& color,
			int iCount, const QByteArray& indices, Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_GEOMETRYCREATEHELPER_1594863285229_H