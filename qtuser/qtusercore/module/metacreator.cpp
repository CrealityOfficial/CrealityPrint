#include "metacreator.h"
#include <QtCore/QDebug>

namespace qtuser_core
{
	SINGLETON_IMPL(MetaCreator)
	MetaCreator::MetaCreator()
		:QObject()
	{
	}
	
	MetaCreator::~MetaCreator()
	{
	}

	void MetaCreator::registerType(const QMetaObject* metaObject)
	{
		QHash<QString, const QMetaObject*>::iterator it = m_metaObjects.insert(metaObject->className(), metaObject);
		if (it == m_metaObjects.end())
		{
			qDebug() << metaObject->className() << " MetaType Register Error.";
		}
	}

	QObject* MetaCreator::create(const QString& name)
	{
		QHash<QString, const QMetaObject*>::iterator it = m_metaObjects.find(name);
		if (it != m_metaObjects.end())
		{
			QObject* object = (*it)->newInstance();
			return object;
		}
		return nullptr;
	}
}