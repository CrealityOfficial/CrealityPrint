#ifndef QTUSER_CORE_METACREATOR_1598969090730_H
#define QTUSER_CORE_METACREATOR_1598969090730_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/singleton.h"
#include <QtCore/QHash>

namespace qtuser_core
{
	class QTUSER_CORE_API MetaCreator : public QObject
	{
		Q_OBJECT
		SINGLETON_DECLARE(MetaCreator)
	public:
		virtual ~MetaCreator();

		void registerType(const QMetaObject* metaObject);
		QObject* create(const QString& name);
	protected:
		QHash<QString, const QMetaObject*> m_metaObjects;
	};
	
	SINGLETON_EXPORT(QTUSER_CORE_API, MetaCreator)
}

#define TYPE_REGISTER(x) \
	{\
		const QMetaObject* metaObject = &x::staticMetaObject; \
		qtuser_core::getMetaCreator()->registerType(metaObject); \
	}

#define AUTO_META_TYPE(x) namespace meta_type {\
						class x##mt \
						{                    \
						public:              \
						    x##mt()    \
						    {                 \
						        TYPE_REGISTER(x);   \
						    };                 \
						    ~x##mt() {};  \
						}; }                        \
						meta_type::x##mt x##metainit; 
#endif // QTUSER_CORE_METACREATOR_1598969090730_H