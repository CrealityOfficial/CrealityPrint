#include "geometrycreatehelper.h"
#include <Qt3DRender/QBuffer>
#include <qtuser3d/geometry/bufferhelper.h>

namespace qtuser_3d
{
	GeometryCreateHelper::GeometryCreateHelper(QObject* parent)
		:QObject(parent)
	{
	}
	
	GeometryCreateHelper::~GeometryCreateHelper()
	{
	}

	Qt3DRender::QGeometry* GeometryCreateHelper::create(Qt3DCore::QNode* parent, Qt3DRender::QAttribute* attribute0, Qt3DRender::QAttribute* attribute1, Qt3DRender::QAttribute* attribute2,
		Qt3DRender::QAttribute* attribute3)
	{
		Qt3DRender::QGeometry* geometry = new Qt3DRender::QGeometry(parent);

		if (attribute0) geometry->addAttribute(attribute0);
		if (attribute1) geometry->addAttribute(attribute1);
		if (attribute2) geometry->addAttribute(attribute2);
		if (attribute3) geometry->addAttribute(attribute3);

		return geometry;
	}


	Qt3DRender::QGeometry* GeometryCreateHelper::createEx(Qt3DCore::QNode* parent, Qt3DRender::QAttribute* attribute0, Qt3DRender::QAttribute* attribute1, Qt3DRender::QAttribute* attribute2,
		Qt3DRender::QAttribute* attribute3, Qt3DRender::QAttribute* attribute4, Qt3DRender::QAttribute* attribute5)
	{
		Qt3DRender::QGeometry* geometry = new Qt3DRender::QGeometry(parent);

		if (attribute0) geometry->addAttribute(attribute0);
		if (attribute1) geometry->addAttribute(attribute1);
		if (attribute2) geometry->addAttribute(attribute2);
		if (attribute3) geometry->addAttribute(attribute3);
		if (attribute4) geometry->addAttribute(attribute4);
		if (attribute5) geometry->addAttribute(attribute5);

		return geometry;
	}

	Qt3DRender::QGeometry* GeometryCreateHelper::create(Qt3DCore::QNode* parent, AttributeShade* attribute1,
		AttributeShade* attribute2, AttributeShade* attribute3,
		AttributeShade* attribute4, AttributeShade* attribute5)
	{
		auto f = [](AttributeShade* attribute)-> Qt3DRender::QAttribute* {
			Qt3DRender::QAttribute* qAttribute = nullptr;
			if (attribute && attribute->count > 0 && !attribute->name.isEmpty() && attribute->type == 0)
			{
				Qt3DRender::QBuffer* buffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
				buffer->setData(attribute->bytes);
				qAttribute = new Qt3DRender::QAttribute(buffer, attribute->name, Qt3DRender::QAttribute::Float, attribute->stride, attribute->count);
			}
			if (attribute && attribute->count > 0 && attribute->type == 1)
			{
				Qt3DRender::QBuffer* indexBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer);
				indexBuffer->setData(attribute->bytes);

				qAttribute = new Qt3DRender::QAttribute(indexBuffer, Qt3DRender::QAttribute::UnsignedInt,
					1, attribute->stride * attribute->count);
				qAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
			}
			return qAttribute;
		};
		Qt3DRender::QAttribute* qAttribute1 = f(attribute1);
		Qt3DRender::QAttribute* qAttribute2 = f(attribute2);
		Qt3DRender::QAttribute* qAttribute3 = f(attribute3);
		Qt3DRender::QAttribute* qAttribute4 = f(attribute4);
		Qt3DRender::QAttribute* qAttribute5 = f(attribute5);
		return createEx(parent, qAttribute1, qAttribute2, qAttribute3, qAttribute4, qAttribute5);
	}

	Qt3DRender::QBuffer* GeometryCreateHelper::createBuffer(AttributeShade* attribute)
	{
		if (attribute && attribute->count > 0 && !attribute->name.isEmpty() && attribute->type == 0)
		{
			Qt3DRender::QBuffer* buffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
			buffer->setData(attribute->bytes);
			return buffer;
		}
		return nullptr;
	}

	Qt3DRender::QGeometry* GeometryCreateHelper::create(int vCount, const QByteArray& position, const QByteArray& normal,
		const QByteArray& texcoord, const QByteArray& color, Qt3DCore::QNode* parent)
	{
		if (position.size() == 0)
			return nullptr;
	
		Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		Qt3DRender::QBuffer* normalBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		positionBuffer->setData(position);
		normalBuffer->setData(normal);
	
		Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(positionBuffer, Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, 3, vCount);
		Qt3DRender::QAttribute* normalAttribute = new Qt3DRender::QAttribute(normalBuffer, Qt3DRender::QAttribute::defaultNormalAttributeName(), Qt3DRender::QAttribute::Float, 3, vCount);
	
		Qt3DRender::QAttribute* texcoordAttribute = nullptr;
		Qt3DRender::QAttribute* colorAttribute = nullptr;
	
		if (texcoord.size() > 0)
		{
			Qt3DRender::QBuffer* texcoordBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
			texcoordBuffer->setData(texcoord);
			texcoordAttribute = new Qt3DRender::QAttribute(texcoordBuffer, Qt3DRender::QAttribute::defaultTextureCoordinateAttributeName(), Qt3DRender::QAttribute::Float, 2, vCount);
		}
		if (color.size() > 0)
		{
			Qt3DRender::QBuffer* colorBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
			colorBuffer->setData(color);
			colorAttribute = new Qt3DRender::QAttribute(colorBuffer, Qt3DRender::QAttribute::defaultColorAttributeName(), Qt3DRender::QAttribute::Float, 3, vCount);
		}
	
		return qtuser_3d::GeometryCreateHelper::create(parent, positionAttribute, normalAttribute, colorAttribute, texcoordAttribute);
	}
	
	Qt3DRender::QGeometry* GeometryCreateHelper::indexCreate(int vCount, const QByteArray& position,
		int iCount, const QByteArray& indices, Qt3DCore::QNode* parent)
	{
		if (position.size() == 0)
			return nullptr;
	
		Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		positionBuffer->setData(position);
		Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(positionBuffer, Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, 3, vCount);
	
		Qt3DRender::QAttribute* indexAttribute = nullptr;
		if (indices.size() > 0)
		{
			indexAttribute = qtuser_3d::BufferHelper::CreateIndexAttribute((const char*)indices, iCount);
		}
	
		return qtuser_3d::GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}
	
	Qt3DRender::QGeometry* GeometryCreateHelper::indexCreate(int vCount, const QByteArray& position, const QByteArray& color,
		int iCount, const QByteArray& indices, Qt3DCore::QNode* parent)
	{
		if (position.size() == 0)
			return nullptr;

		Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		positionBuffer->setData(position);
		Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(positionBuffer, Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, 3, vCount);

		Qt3DRender::QBuffer* colorBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		colorBuffer->setData(color);
		Qt3DRender::QAttribute* colorAttribute = new Qt3DRender::QAttribute(colorBuffer, Qt3DRender::QAttribute::defaultColorAttributeName(), Qt3DRender::QAttribute::Float, 3, vCount);


		Qt3DRender::QAttribute* indexAttribute = nullptr;
		if (indices.size() > 0)
		{
			indexAttribute = qtuser_3d::BufferHelper::CreateIndexAttribute((const char*)indices, iCount);
		}

		return qtuser_3d::GeometryCreateHelper::create(parent, positionAttribute, indexAttribute, colorAttribute);
	}
}