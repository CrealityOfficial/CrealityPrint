#include "lineexentity.h"
#include "qmath.h"

namespace qtuser_3d
{

	LineExEntity::LineExEntity(Qt3DCore::QNode* parent)
		:qtuser_3d::LineEntity(parent)
	{
		m_startPoint = QVector3D(0, 0, 0);
		m_endPoint = QVector3D(0, 0, 0);
		m_normal = QVector3D(0, 0, 1);

		m_lineType = NORMAL_LINE;
		m_dot_line_len = 2;
		m_dot_line_space = 0.8;

		m_arrow_len = 1.5;
		m_arrow_angle = 30 * M_PI / 180;

		m_colorParameter->setValue(QVector4D(0.2f, 0.2f, 0.8f, 1.0));
	}

	LineExEntity::~LineExEntity()
	{
		//
	}

	int LineExEntity::setPoint(const QVector3D& start_pt, const QVector3D& end_pt, const QVector3D& normal)
	{
		m_startPoint = start_pt;
		m_endPoint = end_pt;
		m_normal = normal.normalized();
		return genStorageData();
	}

	int LineExEntity::genGeometry()
	{
		if (m_storageData.size() == 0)
		{
			return -1;
		}
		updateGeometry(m_storageData);
		return 0;
	}

	int LineExEntity::changeLineStype(LINE_TYPE line_type)
	{
		if (m_lineType == line_type)
		{
			return 0;
		}
		m_lineType = line_type;
		return genStorageData();
	}

	void LineExEntity::setDotLineLen(int dot_line_len)
	{
		m_dot_line_len = dot_line_len;
	}

	void LineExEntity::setDotLineSpace(int dot_line_space)
	{
		m_dot_line_space = dot_line_space;
	}

	void LineExEntity::setArrowLen(int arrow_len)
	{
		m_arrow_len = arrow_len;
	}

	void LineExEntity::setArrowAngle(float arrow_angle)
	{
		m_arrow_angle = arrow_angle * M_PI / 180;
	}

	int LineExEntity::genStorageData()
	{
		m_storageData.clear();
		if (m_lineType & NORMAL_LINE)
		{
			m_storageData.push_back(m_startPoint);
			m_storageData.push_back(m_endPoint);
		}
		else if (m_lineType & DOT_LINE)
		{
			QVector3D dir = m_endPoint - m_startPoint;
			float total_len = dir.length();
			dir.normalize();
			QVector3D dot_line_dir = dir * m_dot_line_len;
			QVector3D dot_space_dir = dir * m_dot_line_space;

			bool is_space = false;
			float x = 0;
			QVector3D v = m_startPoint;
			while (true)
			{
				if (!is_space)
				{
					if (x + m_dot_line_len > total_len)
					{
						m_storageData.push_back(v);
						m_storageData.push_back(m_endPoint);
						break;
					}
					// 一段线段的起点和重点
					m_storageData.push_back(v);
					v += dot_line_dir;
					m_storageData.push_back(v);
					is_space = true;
				}
				else
				{
					if (x + m_dot_line_space > total_len)
					{
						break;
					}
					v += dot_space_dir;
					is_space = false;
				}
			}
		}
		// 有两个箭头
		if (m_lineType & 0x10)
		{
			QVector3D dir = m_endPoint - m_startPoint;
			dir.normalize();

			QVector3D offset = QVector3D::crossProduct(dir, m_normal);

			QVector3D dir_theta = qCos(m_arrow_angle) * m_arrow_len * dir;
			QVector3D offset_theta = qSin(m_arrow_angle) * m_arrow_len * offset;

			m_storageData.push_back(m_startPoint);
			m_storageData.push_back(m_startPoint + dir_theta + offset_theta);
			m_storageData.push_back(m_startPoint);
			m_storageData.push_back(m_startPoint + dir_theta - offset_theta);

			m_storageData.push_back(m_endPoint);
			m_storageData.push_back(m_endPoint - dir_theta + offset_theta);
			m_storageData.push_back(m_endPoint);
			m_storageData.push_back(m_endPoint - dir_theta - offset_theta);
		}
		return m_storageData.size();
	}
}

