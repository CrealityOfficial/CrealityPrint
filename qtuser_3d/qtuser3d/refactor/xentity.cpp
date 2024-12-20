#include "xentity.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/refactor/xeffect.h"

#include <QtCore/QDebug>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>

namespace qtuser_3d
{
	XEntity::XEntity(Qt3DCore::QNode* parent)
		:Qt3DCore::QEntity(parent)
		, m_geometryRenderer(nullptr)
	{
		m_material = new Qt3DRender::QMaterial(this);
		m_transform = new Qt3DCore::QTransform(this);
		addComponent(m_transform);
		addComponent(m_material);

		m_geometryRenderer = new Qt3DRender::QGeometryRenderer(this);
		addComponent(m_geometryRenderer);
	}

	XEntity::~XEntity()
	{
	}

	Qt3DRender::QParameter* XEntity::setParameter(const QString& name, const QVariant& value)
	{
		Qt3DRender::QParameter* parameter = nullptr;
		QVector<Qt3DRender::QParameter*> parameters = m_material->parameters();

		for (Qt3DRender::QParameter* param : parameters)
		{
			if (name == param->name())
			{
				param->setValue(value);
				parameter = param;
				break;
			}
		}

		if (!parameter)
		{
			parameter = new Qt3DRender::QParameter(name, value, m_material);
			m_material->addParameter(parameter);
		}

		return parameter;
	}

	void XEntity::setEffect(XEffect* effect)
	{
		m_material->setEffect(effect);
	}

	XEffect* XEntity::xeffect()
	{
		return qobject_cast<XEffect*>(m_material->effect());
	}

	void XEntity::setPose(const QMatrix4x4& matrix)
	{
		m_transform->setMatrix(matrix);
		setParameter("model_matrix", matrix);
	}

	void XEntity::setModelMatrix(const QMatrix4x4& matrix)
	{
		setParameter("model_matrix", matrix);
	}

	void XEntity::setGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type,
		bool deleteOldGeometry)
	{
		Qt3DRender::QGeometry* old = m_geometryRenderer->geometry();
		if (geometry == old)
			return;

		m_geometryRenderer->setGeometry(geometry);
		m_geometryRenderer->setPrimitiveType(type);

		if (old && deleteOldGeometry)
			old->deleteLater();
	}

	void XEntity::replaceGeometryRenderer(Qt3DRender::QGeometryRenderer* geometryRenderer)
	{
		removeComponent(m_geometryRenderer);
		m_geometryRenderer->setParent((Qt3DCore::QNode*)nullptr);

		delete m_geometryRenderer;
		m_geometryRenderer = geometryRenderer;

		if (m_geometryRenderer)
		{
			addComponent(m_geometryRenderer);
		}
	}

	void XEntity::addRenderState(int index, Qt3DRender::QRenderState* state)
	{
		XEffect* effect = qobject_cast<XEffect*>(m_material->effect());
		if (!effect)
		{
			qDebug() << QString("XEffect is null.");
			return;
		}

		effect->addRenderState(index, state);
	}

	void XEntity::addPassFilter(int index, const QString& filter)
	{
		XEffect* effect = qobject_cast<XEffect*>(m_material->effect());
		if (!effect)
		{
			qDebug() << QString("XEffect is null.");
			return;
		}

		effect->addPassFilter(index, filter);
	}

	void XEntity::removePassFilter(int passIndex, const QString& filterName, const QVariant& filterValue)
	{
		XEffect* effect = qobject_cast<XEffect*>(m_material->effect());
		if (!effect)
		{
			qDebug() << Q_FUNC_INFO << QString("XEffect is null.");
			return;
		}

		effect->removePassFilter(passIndex, filterName, filterValue);
	}

	Qt3DRender::QGeometry* XEntity::geometry()
	{
		return m_geometryRenderer->geometry();
	}


	void XEntity::updateAttribute(int index, const QByteArray& bytes)
	{
		Qt3DRender::QGeometry* geom = geometry();
		if (geom)
		{
			QVector<Qt3DRender::QAttribute*> attris = geom->attributes();
			if(index >= 0 && index < attris.size())
			{
				Qt3DRender::QAttribute* attribute = attris.at(index);
				attribute->buffer()->setData(bytes);
			}
			else
			{
				Qt3DRender::QBuffer* colorBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
				colorBuffer->setData(bytes);
				int count = bytes.size() / 4;
				geom->addAttribute(new Qt3DRender::QAttribute(colorBuffer, Qt3DRender::QAttribute::defaultColorAttributeName(), Qt3DRender::QAttribute::Float, 3, count));
			}
		}
	}

	QMatrix4x4 XEntity::pose()
	{
		return m_transform->matrix();
	}
}