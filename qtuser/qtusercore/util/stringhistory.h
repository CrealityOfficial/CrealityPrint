#ifndef QTUSER_CORE_STRINGHISTORY_1651996382739_H
#define QTUSER_CORE_STRINGHISTORY_1651996382739_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QStringList>

namespace qtuser_core
{
	class QTUSER_CORE_API StringHistory : public QObject
	{
		Q_OBJECT
	public:
		StringHistory(const QString& countID, const QString& stringID, QObject* parent = nullptr);
		virtual ~StringHistory();

		QStringList strings();
		QString lastOne();
		void setNum(int count);
		void setRecentString(const QString& str);
		void clear();
		//remove isnot exist file string
		void removeString(const QString& str);
	signals:
		void sigDataChange(const QStringList& data);
	protected:
		QString m_countID;
		QString m_stringsID;
	};
}

#endif // QTUSER_CORE_STRINGHISTORY_1651996382739_H