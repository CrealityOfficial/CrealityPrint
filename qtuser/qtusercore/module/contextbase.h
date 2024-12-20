#ifndef QTUSER_QUICK_CONTEXTBASE_1652425898903_H
#define QTUSER_QUICK_CONTEXTBASE_1652425898903_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QMap>

namespace qtuser_core
{
	class QTUSER_CORE_API ContextBase : public QObject
	{
		Q_OBJECT
	public:
		ContextBase(QObject* parent = nullptr);
		virtual ~ContextBase();

		void setContextName(const QString& name);
		Q_INVOKABLE QObject* obj(const QString& name);
		Q_INVOKABLE bool registerObj(const QString& name, QObject* object);
		Q_INVOKABLE void unRegisterObj(const QString& name);
	protected:
		QObject* obj(ContextBase* parent, const QString& childName);
		bool registerObj(ContextBase* parent, QObject* object, const QString& childName);
		virtual QString contextName();
	protected:
		QMap<QString, QObject*> m_objects;
		QString m_contextName;
	};
}

#endif // QTUSER_QUICK_CONTEXTBASE_1652425898903_H