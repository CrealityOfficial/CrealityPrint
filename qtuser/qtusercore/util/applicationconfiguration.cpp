#include "applicationconfiguration.h"
#include "qtusercore/util/colorutil.h"

#include <QtCore/QSettings>
#include <QtCore/QtCore>
#include <QtCore/QDebug>

namespace qtuser_core
{
    ApplicationConfiguration::ApplicationConfiguration(QObject* parent)
        : QObject(parent)
	{
	}
	
    ApplicationConfiguration::~ApplicationConfiguration()
	{
	}

    void ApplicationConfiguration::registerBundle(const QString& name, QSettings::Format format)
    {
        qtuser_core::ConfigBundle* bundle = findBundle(name);
        if (bundle)
        {
            qDebug() << QString("ApplicationConfiguration::registerBundle %1 exist.").arg(name);
            return;
        }

        if (name.isEmpty())
        {
            qDebug() << QString("ApplicationConfiguration::registerBundle name empty.");
            return;
        }

        bundle = new qtuser_core::ConfigBundle(this, name);
        m_bundles.insert(name, bundle);
    }

    void ApplicationConfiguration::unRegisterBundle(const QString& name)
    {
        QMap<QString, qtuser_core::ConfigBundle*>::iterator it = m_bundles.find(name);
        if (it != m_bundles.end())
        {
            delete it.value();
            m_bundles.erase(it);
        }
    }

    QVariant ApplicationConfiguration::value(const QString& name, const QString& key, const QString& group)
    {
        qtuser_core::ConfigBundle* bundle = findBundle(name);
        if (!bundle)
        {
            qDebug() << QString("ApplicationConfiguration::registerBundle %1 not exist.").arg(name);
            return QVariant();
        }

        return bundle->value(key, group);
    }

    void ApplicationConfiguration::setValue(const QString& name, const QString& key, const QVariant& value, const QString& group)
    {
        qtuser_core::ConfigBundle* bundle = findBundle(name);
        if (!bundle)
        {
            qDebug() << QString("ApplicationConfiguration::registerBundle %1 not exist.").arg(name);
            return;
        }

        bundle->setValue(key, value, group);
    }

    qtuser_core::ConfigBundle* ApplicationConfiguration::findBundle(const QString& bundle)
    {
        return m_bundles.value(bundle);
    }

    QStringList ApplicationConfiguration::parseKey(const QString& key)
    {
        QStringList names = key.split("_");
        while (names.size() < 3)
            names.push_back(QString(""));
        return names;
    }

    QColor ApplicationConfiguration::color(const QString& name, const QString& key, const QString& group)
    {
        QVariant var = value(name, key, group);
        return QColor(var.toString());
    }

    QVector4D ApplicationConfiguration::vector4(const QString& name, const QString& key, const QString& group)
    {
        QVariant var = value(name, key, group);
        return qtuser_core::strColor2QVector4DColor(var.toString());
    }

    QVariantList ApplicationConfiguration::varlist(const QString& name, const QString& key, const QString& group)
    {
        QVariant var = value(name, key, group);
        QStringList subNames = var.toString().split("|");
        QVariantList vars;
        for (const QString& subName : subNames)
        {
            QString varName = QString("%1_%2").arg(key).arg(subName);
            vars.push_back(vector4(name, varName, group));
        }
        return vars;
    }

    QString ApplicationConfiguration::string(const QString& name, const QString& key, const QString& group)
    {
        return value(name, key, group).toString();
    }
}