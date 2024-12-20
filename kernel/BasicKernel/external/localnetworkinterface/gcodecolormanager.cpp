#include "gcodecolormanager.h"
#include <qchar.h>
#include <qglobal.h>
#include <QDebug>
namespace creative_kernel
{
	GCodeColorManager::GCodeColorManager(QObject* parent)
		:QObject(parent)
	{
	}

	GCodeColorManager::~GCodeColorManager()
	{

	}

	QList<GCodeColorData*> GCodeColorManager::getColorDatas()
	{
		return m_colorDatas;
	}
	void GCodeColorManager::cleatDatas()
	{
		Q_FOREACH(auto colorData, m_colorDatas)
		{
			delete colorData;
		}
		m_colorDatas.clear();
	}
	void GCodeColorManager::addColorData(GCodeColorData* data)
	{
		m_colorDatas.push_back(data);
	}
	void GCodeColorManager::setDefaultGcodeInfo(QString types,QString colors,QString ids)
	{
		m_types = types;
		m_colors = colors;
		m_ids = ids;
		m_colorDatas.clear();
		addDefaultColorData();
		
	}
	std::string getColorIdByIndex(int index)
    {
        return "T" + std::to_string(index/4 +1) + std::string(1,'A'+index%4);
    }
	void GCodeColorManager::addDefaultColorData()
	{
		QStringList colorList = m_colors.split(";");
		for (int i=0;i<colorList.length();i++)
		{
			if(colorList[i].isEmpty()) continue;
			GCodeColorData *data = new GCodeColorData();
			data->type = m_types.split(";").at(i);
			data->color = colorList[i];
			data->colorId = QString::fromStdString(getColorIdByIndex(i));
			data->boxId = 1;
			m_colorDatas.push_back(data);
		}
		setCount(m_colorDatas.length());
	}
	QVariantList GCodeColorManager::dataMaps()
	{
		QVariantList list;

		for (int index = 0; index < m_colorDatas.size(); index++)
		{
			QVariantMap data;
			GCodeColorData *gdata = m_colorDatas.at(index);
			if (gdata->color.isEmpty()) {
				gdata->color = Qt::transparent; // ʹ�ð�ɫ��ΪĬ��ֵ
			}
			if (gdata->color.length() > 7) {
				gdata->color = "#" + gdata->color.remove(0,2);
			}
			data.insert("colorId", gdata->colorId);
			data.insert("gcodeColor", gdata->color);
			data.insert("gcodeType", gdata->type);
			data.insert("gcodeBoxId", gdata->boxId);
			data.insert("gcodeMaterialId", gdata->materialId);
			list.push_back(data);
		}
		return list;

	}
	QVariantMap GCodeColorManager::getColorData(int index)
	{
		QVariantMap data;
		data.clear();
		if (m_colorDatas.size() < index) return data;
		GCodeColorData *gdata = m_colorDatas.at(index);
		{
			data.insert("gcodeColor", gdata->color);
			data.insert("gcodeType", gdata->type);
			data.insert("gcodeBoxId", gdata->boxId);
			data.insert("gcodeMaterialId", gdata->materialId);
		}
		return data;
	}
}