#ifndef _US_SETTINGITEMDEF_1589438667492_H
#define _US_SETTINGITEMDEF_1589438667492_H
#include "basickernelexport.h"
#include <QtCore/QVariant>

namespace us
{
	bool checkBool(const QString& str);
	QStringList loadKeys(const QString& fileName);
	void saveKeys(const QStringList& keys, const QString& fileName);

	class MetaType;
	class BASIC_KERNEL_API SettingItemDef: public QObject
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

		QString settableGlobally;
		QString settablePerExtruder;
		QString settablePerMesh;
		QString settablePerMeshGroup;

		int level;
		int order;

		QList<std::pair<QString, QString> > options;

	protected:
		MetaType* m_metaType;
	};
}
#endif // _US_SETTINGITEMDEF_1589438667492_H
