#include "configbundle.h"

#include <QtCore/QCoreApplication>

#if _DEBUG
#include "../buildinfo.h"
#endif 

using namespace qtuser_core;


ConfigBundle::ConfigBundle(QObject* parent, const QString& bundleName, const QString& bundleType)
	: QObject(parent)
	, m_bundleDir("")
{
#if _DEBUG
	m_bundleDir = SOURCE_ROOT + QString("/resources/config/");
#else
	#if defined (__APPLE__)
		int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
		m_bundleDir = QCoreApplication::applicationDirPath().left(index) + "/Resources/resources/config/";
	#else
		m_bundleDir = QCoreApplication::applicationDirPath() + "/resources/config/";
	#endif
#endif

	m_bundleName = bundleName.isEmpty() ? DEFAULT_BUNDLE_NAME : bundleName;
	m_bundleType = bundleType.isEmpty() ? "" : bundleType + "/";
	QString iniPath = m_bundleDir + m_bundleType + m_bundleName + ".ini";

	m_pConfigSettings = new QSettings(iniPath, QSettings::Format::IniFormat, this);
	m_pConfigSettings->setIniCodec("UTF-8");
}

ConfigBundle::~ConfigBundle()
{
}

void ConfigBundle::setValue(const QString& key, const QVariant& value, const QString& group)
{
	if (!group.isEmpty())
		m_pConfigSettings->beginGroup(group);
	
	m_pConfigSettings->setValue(key, value);

	if (!group.isEmpty())
		m_pConfigSettings->endGroup();
}

void ConfigBundle::removeValue(const QString& key, const QString& group)
{
	if (!group.isEmpty())
		m_pConfigSettings->beginGroup(group);

	m_pConfigSettings->remove(key);

	if (!group.isEmpty())
		m_pConfigSettings->endGroup();
}

void ConfigBundle::clear()
{
	m_pConfigSettings->clear();
}

QStringList ConfigBundle::keys(const QString& group)
{
	if (!group.isEmpty())
		m_pConfigSettings->beginGroup(group);

	QStringList keys = m_pConfigSettings->allKeys();

	if (!group.isEmpty())
		m_pConfigSettings->endGroup();

	return keys;
}

bool ConfigBundle::hasKey(const QString& key, const QString& group)
{
	if (!group.isEmpty())
		m_pConfigSettings->beginGroup(group);

	bool hasKey = m_pConfigSettings->contains(key);

	if (!group.isEmpty())
		m_pConfigSettings->endGroup();

	return hasKey;
}

QVariant qtuser_core::ConfigBundle::value(const QString& key, const QString& group)
{
	if (!group.isEmpty())
		m_pConfigSettings->beginGroup(group);

	QVariant value = m_pConfigSettings->value(key, "");

	if (!group.isEmpty())
		m_pConfigSettings->endGroup();

	return value;
}

void ConfigBundle::sync()
{
	m_pConfigSettings->sync();
}
