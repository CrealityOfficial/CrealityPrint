#include "qtusercore/plugin/qmlentry.h"

QmlEntry::QmlEntry(QObject* parent)
	:QObject(parent)
    , m_enabled(true)
    , m_order(0)
{
}

QmlEntry::~QmlEntry()
{
}

bool QmlEntry::enabled()
{
	if (!m_enabled)
		return false;

	return checkEnabled();
}

bool QmlEntry::operateEnabled()
{
	return checkOperateEnabled();
}

bool QmlEntry::shot()
{
	return isShot();
}

bool QmlEntry::checkEnabled()
{
	return true;
}

bool QmlEntry::checkOperateEnabled()
{
	return true;
}

QString QmlEntry::enabledIcon() const
{
	return m_enabledIcon;
}

QString QmlEntry::disableIcon() const
{
	return m_disableIcon;
}

QString QmlEntry::pressedIcon() const
{
	return m_pressedIcon;
}

QString QmlEntry::source() const
{
	return m_source;
}

QString QmlEntry::name()
{
	return nameImpl();
}

void QmlEntry::setEnabledIcon(const QString& icon)
{
	m_enabledIcon = icon;
	emit enabledIconChanged();
}

void QmlEntry::setPressedIcon(const QString& icon)
{
	m_pressedIcon = icon;
	emit pressedIconChanged();
}

void QmlEntry::setName(const QString& name)
{
	m_name = name;
	emit nameChanged();
}

void QmlEntry::setSource(const QString& source)
{
	m_source = source;
	emit sourceChanged();
}

void QmlEntry::setDisableIcon(const QString& icon)
{
	m_disableIcon = icon;
	emit disableIconChanged();
}

void QmlEntry::setEnabled(bool enabled)
{
	if (m_enabled != enabled)
	{
		m_enabled = enabled;
		emit enabledChanged();
	}
}

void QmlEntry::updateEnabled()
{
	emit enabledChanged();
}

void QmlEntry::execute()
{
	onExecute();
}

void QmlEntry::onExecute()
{
	//override
}

QString QmlEntry::nameImpl()
{
	return m_name;
}

bool QmlEntry::isShot()
{
	return false;
}

void QmlEntry::updateState(unsigned mask)
{
	emit enabledChanged();
	emit operateEnabledChanged();
}

void QmlEntry::updateTrans()
{
	emit nameChanged();
}

int QmlEntry::order()
{
	return m_order;
}