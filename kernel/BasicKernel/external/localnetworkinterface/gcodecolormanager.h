#ifndef _GCODECOLORMANAGE_H
#define _GCODECOLORMANAGE_H
#include <QtCore/QObject>
#include <QVariant>
#include <qchar.h>
#include "basickernelexport.h"
namespace creative_kernel
{
	struct GCodeColorData
	{
		QString colorId;
		QString color;
		int boxId;
		QString type;
		int materialId;
		GCodeColorData()
		{
			color = "#000000";
			boxId = 0;
			type = "ABS";
			materialId = 0;
		}
	};
	class BASIC_KERNEL_API GCodeColorManager : public QObject
	{
		Q_OBJECT

		Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
		Q_PROPERTY(QVariantList dataList READ dataMaps  NOTIFY countChanged)
	public:
		GCodeColorManager(QObject* parent = nullptr);
		virtual ~GCodeColorManager();

		int count() { return m_count; }
		void setCount(int count) {
			m_count = count; 
			emit countChanged();
			emit colorDataChanged();
		}
		void cleatDatas();
		void addColorData(GCodeColorData* data);
		void addDefaultColorData();
		QVariantList dataMaps();
		QVariantMap getColorData(int index);
		QList<GCodeColorData*> getColorDatas();
		Q_INVOKABLE void setDefaultGcodeInfo(QString types,QString colors,QString ids);

	signals:
		void countChanged();
		void colorDataChanged();
	private:
		int m_count = 0;
		QList<GCodeColorData *> m_colorDatas;
		QString m_types;
		QString m_colors;
		QString m_ids;
	};

}

#endif // _GCODECOLORMANAGE_H