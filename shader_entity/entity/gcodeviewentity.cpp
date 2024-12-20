#include "gcodeviewentity.h"
#include "gcodevieweffect.h"

namespace qtuser_3d
{
	GCodeViewEntity::GCodeViewEntity(Qt3DCore::QNode* parent)
		: XEntity(parent)
	{
		setEffect(new GCodeViewEffect(this));

		m_clipValue = setParameter("clipValue", QVector4D(0.0f, 20000.0f, 0.0f, 30000.0f));
		m_showType = setParameter("showType", 0);
		m_animation = setParameter("animation", 0);
		m_layerShow = setParameter("layershow", QVector2D(-1.0f, 9999999.0f));

		QVariantList values;
		/*values << QVector4D(0.5f, 0.5f, 0.5f, 1.0f)
			<< QVector4D(0.47f, 0.18f, 0.16f, 1.0f)
			<< QVector4D(0.01f, 0.55f, 0.02f, 1.0f)
			<< QVector4D(1.0f, 0.7f, 0.5f, 1.0f)
			<< QVector4D(0.02f, 0.55f, 0.55f, 1.0f)
			<< QVector4D(0.32f, 0.12f, 0.33f, 1.0f)
			<< QVector4D(0.9f, 0.86f, 0.2f, 1.0f)
			<< QVector4D(0.71f, 0.74f, 0.22f, 1.0f)
			<< QVector4D(1.0f, 0.42f, 0.27f, 1.0f)
			<< QVector4D(0.4f, 0.2f, 0.3f, 1.0f)
			<< QVector4D(0.84f, 0.23f, 0.07f, 1.0f)
			<< QVector4D(0.2f, 0.6f, 0.1f, 1.0f)
			<< QVector4D(0.9f, 0.3f, 0.2f, 1.0f)
			<< QVector4D(0.37f, 0.34f, 0.37f, 1.0f)
			<< QVector4D(1.0f, 1.0f, 1.0f, 1.0f)
			<< QVector4D(0.0f, 0.0f, 1.0f, 1.0f)
			<< QVector4D(1.0f, 0.0f, 0.0f, 1.0f);*/

		m_typeColors = setParameter("typecolors[0]", values);

		m_speedColorValues
			<< QVector4D(0.0431373f, 0.172549f, 0.470588f, 1.0f)
			<< QVector4D(0.0745098f, 0.34902f, 0.521569f, 1.0f)
			<< QVector4D(0.109804f, 0.533333f, 0.568627f, 1.0f)
			<< QVector4D(0.0588235f, 0.839216f, 0.0588235f, 1.0f)
			<< QVector4D(0.666667f, 0.94902f, 0.0f, 1.0f)
			<< QVector4D(0.988235f, 0.976471f, 0.117647f, 1.0f)
			<< QVector4D(0.960784f, 0.807843f, 0.0392157f, 1.0f)
			<< QVector4D(0.890196f, 0.533333f, 0.12549f, 1.0f)
			<< QVector4D(0.819608f, 0.407843f, 0.188235f, 1.0f)
			<< QVector4D(0.760784f, 0.321569f, 0.235294f, 1.0f)
			<< QVector4D(0.580392f, 0.14902f, 0.0862745f, 1.0f);

		m_speedColors = setParameter("speedcolors[0]", m_speedColorValues);
		setParameter("speedcolorSize", (float)(m_speedColorValues.size() - 1));

		values.clear();
		values
			<< QVector4D(1.0f, 0.5f, 0.5f, 1.0f)
			<< QVector4D(0.5f, 1.0f, 0.5f, 1.0f)
			<< QVector4D(0.5f, 0.5f, 1.0f, 1.0f)
			<< QVector4D(1.0f, 1.0f, 0.5f, 1.0f)
			<< QVector4D(1.0f, 0.5f, 1.0f, 1.0f)
			<< QVector4D(0.5f, 1.0f, 1.0f, 1.0f);

		m_nozzleColors = setParameter("nozzlecolors[0]", values);

		m_showTypeValues.clear();
		m_showTypeValues
			<< 1
			<< 1
			<< 1
			<< 1
			<< 1
			<< 1
			<< 1
			<< 0
			<< 0
			<< 1
			<< 1
			<< 0
			<< 0
			<< 0
			<< 0
			<< 1
			<< 0;
		int size = m_showTypeValues.size();
		for (size_t i = size; i < 53; i++)
		{
			//结构类型“擦拭”默认为不勾选
			if (i == 49)
			{
				m_showTypeValues << 0;
			}
			else {
				m_showTypeValues << 1;
			}
			
		}

		m_typecolorsshow = setParameter("typecolorsshow[0]", m_showTypeValues);

		m_layerHeight = setParameter("layerHeight", 0.1f);
		//m_lineWidth = setParameter("lineWidth", 0.4f);
	}

	GCodeViewEntity::~GCodeViewEntity()
	{

	}

	void GCodeViewEntity::setTypeShow(int typepos, bool needshow)
	{
		if (typepos >= 0 && typepos < m_showTypeValues.size())
		{
			m_showTypeValues[typepos] = needshow ? 1 : 0;
		}

		if (m_typecolorsshow)
		{
			m_typecolorsshow->setValue(m_showTypeValues);
		}
	}

	void GCodeViewEntity::setClipValue(const QVector4D& value)
	{
		m_clipValue->setValue(value);
	}

	void GCodeViewEntity::setGCodeVisualType(int type)
	{
		int _type = (int)type;
		if (_type >= 0  && m_showType)
		{
			m_showType->setValue(_type);
		}
	}

	void GCodeViewEntity::setAnimation(int animation)
	{
		m_animation->setValue(animation);
	}

	QList<QColor> GCodeViewEntity::getSpeedColorValues()
	{
		QList<QColor> colors;
		for (const QVariant& colorVar : m_speedColorValues)
		{
			QVector4D colorVector = colorVar.value<QVector4D>();
			QColor color;
			color.setRgbF(colorVector.x(), colorVector.y(), colorVector.z());
			colors.append(color);
		}

		return colors;
	}

	void GCodeViewEntity::setLayerShowScope(int startlayer, int endlayer)
	{
		m_layerShow->setValue(QVector2D(startlayer, endlayer));
	}

	void GCodeViewEntity::clearLayerShowScope()
	{
		m_layerShow->setValue(QVector2D(-1.0f, 9999999.0f));
	}

	void GCodeViewEntity::setLayerHeight(float height)
	{
		m_layerHeight->setValue(height);
	}

	/*void GCodeViewEntity::setLineWidth(float width)
	{
		m_lineWidth->setValue(width);
	}*/
}