#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/module/glcompatibility.h"
#include "qtuser3d/math/space3d.h"

namespace qtuser_3d
{
	Qt3DRender::QAttribute* BufferHelper::CreateVertexAttribute(const char* buffer, uint count, uint stride, const QString& name)
	{
		if (count == 0 || !buffer)
			return nullptr;

		Qt3DRender::QBuffer* qBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		qBuffer->setData(QByteArray(buffer, stride * sizeof(float) * count));
		Qt3DRender::QAttribute* attribute = new Qt3DRender::QAttribute(qBuffer, name, Qt3DRender::QAttribute::Float, stride, count);
		return attribute;
	}



	Qt3DRender::QAttribute* BufferHelper::CreateVertexAttribute(const char* buffer, AttribueSlot slot, uint count)
	{
		if (count == 0 || !buffer) return nullptr;

		uint vertexSize = 3;
		QString attributeName;
		switch (slot)
		{
		case AttribueSlot::Position:
			attributeName = Qt3DRender::QAttribute::defaultPositionAttributeName();
			break;
		case AttribueSlot::Normal:
			attributeName = Qt3DRender::QAttribute::defaultNormalAttributeName();
			break;
		case AttribueSlot::Color:
			attributeName = Qt3DRender::QAttribute::defaultColorAttributeName();
			vertexSize = 4;
			break;
		case AttribueSlot::Texcoord:
			attributeName = Qt3DRender::QAttribute::defaultTextureCoordinateAttributeName();  //"vertexTexCoord";
			vertexSize = 2;
			break;
		default:
			break;
		}

		Qt3DRender::QBuffer* qBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		qBuffer->setData(QByteArray(buffer, vertexSize * sizeof(float) * count));
		Qt3DRender::QAttribute* attribute = new Qt3DRender::QAttribute(qBuffer, attributeName, Qt3DRender::QAttribute::Float, vertexSize, count);
		return attribute;
	}

	Qt3DRender::QAttribute* BufferHelper::CreateVertexAttributeEx(const char* buffer, AttribueSlot slot, uint count)
	{
		if (count == 0 || !buffer) return nullptr;

		uint vertexSize = 3;
		QString attributeName;
		switch (slot)
		{
		case AttribueSlot::Position:
			attributeName = Qt3DRender::QAttribute::defaultPositionAttributeName();
			if (qtuser_3d::isGles())
			{
				vertexSize = 4;
			}
			break;
		case AttribueSlot::Normal:
			attributeName = Qt3DRender::QAttribute::defaultNormalAttributeName();
			break;
		case AttribueSlot::Color:
			attributeName = Qt3DRender::QAttribute::defaultColorAttributeName();
			vertexSize = 4;
			break;
		case AttribueSlot::Texcoord:
			attributeName = Qt3DRender::QAttribute::defaultTextureCoordinateAttributeName();  //"vertexTexCoord"; 
			vertexSize = 2;
			break;
		default:
			break;
		}

		Qt3DRender::QBuffer* qBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		qBuffer->setData(QByteArray(buffer, vertexSize * sizeof(float) * count));
		Qt3DRender::QAttribute* attribute = new Qt3DRender::QAttribute(qBuffer, attributeName, Qt3DRender::QAttribute::Float, vertexSize, count);
		return attribute;
	}

	Qt3DRender::QAttribute* BufferHelper::CreateVertexAttribute(const QString& name, const char* buffer, uint vertexSize, uint count)
	{
		Qt3DRender::QBuffer* qBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		qBuffer->setData(QByteArray(buffer, vertexSize * sizeof(float) * count));
		Qt3DRender::QAttribute* attribute = new Qt3DRender::QAttribute(qBuffer, name, Qt3DRender::QAttribute::Float, vertexSize, count);
		return attribute;
	}

	Qt3DRender::QAttribute* BufferHelper::CreateIndexAttribute(const char* buffer, uint count)
	{
		Qt3DRender::QBuffer* indexBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::IndexBuffer);
		indexBuffer->setData(QByteArray(buffer, sizeof(unsigned) * count));

		Qt3DRender::QAttribute* indexAttribute = new Qt3DRender::QAttribute(indexBuffer, Qt3DRender::QAttribute::UnsignedInt, 1, count);
		indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
		return indexAttribute;
	}

	Qt3DRender::QAttribute* BufferHelper::createAttribute(const QString& name, Qt3DRender::QAttribute::VertexBaseType vertexBaseType, uint vertexSize)
	{
		Qt3DRender::QBuffer* buffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		return new Qt3DRender::QAttribute(buffer, name, vertexBaseType, vertexSize, 0);
	}

	Qt3DRender::QAttribute* BufferHelper::createDefaultVertexAttribute()
	{
		uint vertexSize = 3;
		if (qtuser_3d::isGles())
			vertexSize = 4;
		return createAttribute(Qt3DRender::QAttribute::defaultPositionAttributeName(), Qt3DRender::QAttribute::Float, vertexSize);
	}

	Qt3DRender::QAttribute* BufferHelper::createDefaultNormalAttribute()
	{
		return createAttribute(Qt3DRender::QAttribute::defaultNormalAttributeName(), Qt3DRender::QAttribute::Float, 3);
	}

	void BufferHelper::setAttributeCount(Qt3DRender::QAttribute* attribute, int count)
	{
		if (!attribute || count < 0)
			return;

		int bytesSizeNeed = attribute->vertexSize() * sizeof(float) * count;
		QByteArray bytes = attribute->buffer()->data();
		if (bytes.size() < bytesSizeNeed)
		{
			bytes.resize(bytesSizeNeed);
			bytes.fill(0);
		}

		attribute->buffer()->setData(bytes);
		attribute->setCount(count);
	}

	void BufferHelper::clearAttributeBuffer(Qt3DRender::QAttribute* attribute)
	{
		if (!attribute)
			return;

		QByteArray bytes = attribute->buffer()->data();
		bytes.fill(0);
		attribute->buffer()->updateData(0, bytes);
	}

	qtuser_3d::Box3D BufferHelper::calculateVertexAttributeBox(Qt3DRender::QAttribute* attribute, int start, int end, const QMatrix4x4& matrix)
	{
		qtuser_3d::Box3D box;
		if (attribute)
		{
			int count = attribute->count();
			if (start >= 0 && start < end && end < count)
			{
				QByteArray bytes = attribute->buffer()->data();
				int vertexSize = attribute->vertexSize();

				for (int i = start; i < end; ++i)
				{
					QVector3D* position = reinterpret_cast<QVector3D*>(bytes.data() + i * vertexSize * sizeof(float));
					box += matrix * *position;
				}
			}
		}
		return box;
	}

	void BufferHelper::updatePositionNormalAttributes(Qt3DRender::QAttribute* positionAttribute, QByteArray* positionBytes, Qt3DRender::QAttribute* normalAttribute, int start, int end)
	{
		if (!positionAttribute || !normalAttribute)
			return;

		int count = positionAttribute->count();
		int ncount = normalAttribute->count();

		if (count != ncount)
			return;

		if (start >= 0 && start < end && end < count)
		{
			int vertexCount = end - start;
			if (positionBytes)
			{
				int byteSizes = positionBytes->size();
				assert(byteSizes == vertexCount * positionAttribute->vertexSize() * sizeof(float));
				int positionOffset = start * positionAttribute->vertexSize() * sizeof(float);
				positionAttribute->buffer()->updateData(positionOffset, *positionBytes);

				int normalOffset = start * 3 * sizeof(float);
				QByteArray normalBytes(3 * vertexCount * sizeof(float), 0);
				QVector3D* normal = (QVector3D*)normalBytes.data();

				char* position = positionBytes->data();

				int n = vertexCount / 3;
				for (int i = 0; i < n; ++i)
				{
					QVector3D* v0 = (QVector3D*)(position + sizeof(float) * positionAttribute->vertexSize() * (3 * i));
					QVector3D* v1 = (QVector3D*)(position + sizeof(float) * positionAttribute->vertexSize() * (3 * i + 1));
					QVector3D* v2 = (QVector3D*)(position + sizeof(float) * positionAttribute->vertexSize() * (3 * i + 2));
					QVector3D v01 = *v1 - *v0;
					QVector3D v02 = *v2 - *v0;
					QVector3D norm = QVector3D::crossProduct(v01, v02);
					norm.normalize();
					*normal++ = norm;
					*normal++ = norm;
					*normal++ = norm;
				}
				normalAttribute->buffer()->updateData(normalOffset, normalBytes);
			}
		}
	}

	void BufferHelper::updateOneStrideAttributes(Qt3DRender::QAttribute* attribute, QByteArray* flagsBytes, int start, int end)
	{
		if (!attribute || !flagsBytes)
			return;

		int count = attribute->count();
		if (start >= 0 && start < end && end < count)
		{
			int vertexCount = end - start;
			int byteSizes = flagsBytes->size();
			assert(byteSizes == vertexCount * attribute->vertexSize() * sizeof(float));
			int offset = start * attribute->vertexSize() * sizeof(float);
			attribute->buffer()->updateData(offset, *flagsBytes);
		}
	}

	void BufferHelper::releaseAttributes(QList<Qt3DRender::QAttribute*>& attributes, int start, int end)
	{
		for (Qt3DRender::QAttribute* attribute : attributes)
			releaseAttribute(attribute, start, end);
	}

	void BufferHelper::releaseAttribute(Qt3DRender::QAttribute* attribute, int start, int end)
	{
		if (!attribute)
			return;

		int count = attribute->count();
		if (start >= 0 && start < end && end < count)
		{
			int vertexCount = end - start;
			QByteArray bytes;
			bytes.resize(vertexCount * attribute->vertexSize() * sizeof(float));
			bytes.fill(0);
			int offset = start * attribute->vertexSize() * sizeof(float);
			attribute->buffer()->updateData(offset, bytes);
		}
	}

	void BufferHelper::attributeRayCheck(Qt3DRender::QAttribute* positionAttribute, Qt3DRender::QAttribute* normalAttribute, int faceIndex,
		const Ray& ray, QVector3D& position, QVector3D& normal)
	{
		if (!positionAttribute || !normalAttribute)
			return;

		int count = positionAttribute->count();
		int ncount = normalAttribute->count();

		if (count != ncount)
			return;

		int vertexIndex = 3 * faceIndex;
		if (vertexIndex >= 0 && vertexIndex < count)
		{
			QVector3D* positionPtr = (QVector3D*)(positionAttribute->buffer()->data().data() + sizeof(float) * positionAttribute->vertexSize() * vertexIndex);
			QVector3D* normalPtr = (QVector3D*)(normalAttribute->buffer()->data().data() + sizeof(float) * positionAttribute->vertexSize() * vertexIndex);

			normal = *normalPtr;
			QVector3D v0 = *(positionPtr);
			lineCollidePlane(v0, normal, ray, position);
		}
	}

	void BufferHelper::copyAttribute(char* buffer, Qt3DRender::QAttribute* attribute, int start, int end)
	{
		if (!attribute)
			return;

		int count = attribute->count();
		if (start >= 0 && start < end && end < count)
		{
			int offset = start * attribute->vertexSize() * sizeof(float);
			int offset1 = end * attribute->vertexSize() * sizeof(float);
			memcpy(buffer, attribute->buffer()->data().data() + offset, offset1 - offset);
		}
	}
}
