#ifndef CREATIVE_KERNEL_APP_SETTING_1603955508504_H
#define CREATIVE_KERNEL_APP_SETTING_1603955508504_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/util/configbundle.h"

#include <QtGui/QColor>
#include <QtGui/QVector4D>

namespace qtuser_core
{
	class QTUSER_CORE_API ApplicationConfiguration : public QObject
	{
	public:
		ApplicationConfiguration(QObject* parent = nullptr);
		virtual ~ApplicationConfiguration();

		void registerBundle(const QString& name, QSettings::Format format = QSettings::IniFormat);
		void unRegisterBundle(const QString& name);

		QVariant value(const QString& name, const QString& key, const QString& group);
		void setValue(const QString& name, const QString& key, const QVariant& value, const QString& group);

		QColor color(const QString& name, const QString& key, const QString& group);
		QVector4D vector4(const QString& name, const QString& key, const QString& group);
		QVariantList varlist(const QString& name, const QString& key, const QString& group);
		QString string(const QString& name, const QString& key, const QString& group);
	protected:
		qtuser_core::ConfigBundle* findBundle(const QString& bundle);
		QStringList parseKey(const QString& key);
	private:
		QMap<QString, qtuser_core::ConfigBundle*> m_bundles;
	};
	
}

#endif // CREATIVE_KERNEL_APP_SETTING_1603955508504_H