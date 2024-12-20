#ifndef _CXGCODE_US_SETTINGITEMDEF_1589438667492_H
#define _CXGCODE_US_SETTINGITEMDEF_1589438667492_H
#include "cxgcode/interface.h"
#include <QtCore/QVariant>

namespace cxgcode
{
	CXGCODE_API bool checkBool(const QString& str);
	CXGCODE_API QStringList loadKeys(const QString& fileName);
	CXGCODE_API void saveKeys(const QStringList& keys, const QString& fileName);

	class MetaType;
	class CXGCODE_API SettingItemDef: public QObject
	{
	Q_OBJECT
	public:
		SettingItemDef(QObject* parent = nullptr);
		virtual ~SettingItemDef();

		void process();

		void addSettingItemDef(SettingItemDef* settingItemDef);

		QList<SettingItemDef*> items;

		QString category;
		QString name;
		QString label;
		QString unit;
		QString description;
		QString type;
		QString defaultStr;
		QString minimum;
		QString miniwarning;
		QString maximum;
		QString maxwarning;
		QString valueStr;
		QString showValueStr;
		QString enableStatus;
		bool isCustomSetting;

		int level;
		int order;

		QMap<QString, QString> options;

	protected:
		MetaType* m_metaType;
	};
}
#endif // _CXGCODE_US_SETTINGITEMDEF_1589438667492_H
