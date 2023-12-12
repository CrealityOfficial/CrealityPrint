#ifndef _QTUSER_CORE_DETAILDESCRIPTION_1590380110110_H
#define _QTUSER_CORE_DETAILDESCRIPTION_1590380110110_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QHash>

namespace qtuser_core
{
	class QTUSER_CORE_API DetailDescription: public QObject
	{
		Q_OBJECT
	public:
		DetailDescription(QObject* parent = nullptr);
		virtual ~DetailDescription();

		Q_INVOKABLE QString get(const QString& key);
		void set(const QString& key, const QString& text);
		QStringList enumKeys();

		void parse(const QString& str);
		float value(const QString& key);

		void clear();
	protected:
		QHash<QString, QString> m_details;
	};
}
#endif // _QTUSER_CORE_DETAILDESCRIPTION_1590380110110_H
