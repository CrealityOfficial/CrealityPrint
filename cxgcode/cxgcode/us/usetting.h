#ifndef _CXGCODE_US_USETTING_1589354685441_H
#define _CXGCODE_US_USETTING_1589354685441_H
#include "cxgcode/interface.h"
#include "cxgcode/us/settingitemdef.h"
#include "cxgcode/us/settinggroupdef.h"
#include <QVariant>

namespace cxgcode
{
	class CXGCODE_API USetting: public QObject
	{
		Q_OBJECT
	public:
		USetting(SettingItemDef* def, QObject* parent = nullptr);
		virtual ~USetting();

		USetting* clone();

		float toFloat();
		int toInt();

		Q_INVOKABLE void setValue(const QString& str);
		Q_INVOKABLE QString str();
		Q_INVOKABLE QString key();
		Q_INVOKABLE int enumIndex();
		Q_INVOKABLE bool enabled();
		Q_INVOKABLE QString description();
		Q_INVOKABLE QString label();
		Q_INVOKABLE QVariantList enums();
		Q_INVOKABLE QString type();
		Q_INVOKABLE int level();
		Q_INVOKABLE QString unit();
		Q_INVOKABLE QString minStr();
		Q_INVOKABLE QString maxStr();

		SettingItemDef* def();
	protected:
		SettingItemDef* m_itemDef;
		QString m_str;
	};
}
#endif // _CXGCODE_US_USETTING_1589354685441_H
