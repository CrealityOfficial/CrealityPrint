#include "qtusercore/plugin/toolcommand.h"

ToolCommand::ToolCommand(QObject* parent)
	:QObject(parent)
    , m_enabled(true),
      orderindex(100)
{
}

ToolCommand::~ToolCommand()
{
}

bool ToolCommand::enabled()
{
	return enableImpl();
}

bool ToolCommand::enableImpl()
{
	return m_enabled;
}

QString ToolCommand::enabledIcon() const
{
	return m_enabledIcon;
}

QString ToolCommand::hoveredIcon() const {
	return m_hoveredIcon;
}

QString ToolCommand::disableIcon() const
{
	return m_disableIcon;
}

QString ToolCommand::pressedIcon() const
{
	return m_pressedIcon;
}

QString ToolCommand::source() const
{
	return m_source;
}

QString ToolCommand::name() const
{
	return m_name;
}

void ToolCommand::setEnabledIcon(const QString& icon)
{
	if (m_enabledIcon != icon)
	{
		m_enabledIcon = icon;
		emit enabledIconChanged();
	}

}

void ToolCommand::setHoveredIcon(const QString& icon) {
	if (m_hoveredIcon != icon) {
		m_hoveredIcon = icon;
		emit hoveredIconChanged();
	}
}

void ToolCommand::setPressedIcon(const QString& icon)
{
	if (m_pressedIcon != icon) {
		m_pressedIcon = icon;
		Q_EMIT pressedIconChanged();
	}
}

void ToolCommand::setDisabledIcon(const QString& icon)
{
	if (m_disableIcon != icon) {
		m_disableIcon = icon;
		Q_EMIT disableIconChanged();
	}
}

void ToolCommand::setName(const QString& name)
{
  if (m_name != name) {
    m_name = name;
    Q_EMIT nameChanged();
  }
}

void ToolCommand::setSource(const QString& source)
{
	m_source = source;
}

void ToolCommand::setEnabled(bool enabled)
{
	if (m_enabled != enabled)
	{
		m_enabled = enabled;
		emit enabledChanged();
	}
}

void ToolCommand::execute()
{
	onExecute();
	emit executed();
}

void ToolCommand::onExecute()
{
}
bool ToolCommand::checkSelect()
{
    return true;
}
bool ToolCommand::outofPlatform()
{
	return false;
}
bool ToolCommand::isSelectModel()
{
    return true;
}

void ToolCommand::saveCall()
{
	QMetaObject::invokeMethod(this, "execute", Qt::QueuedConnection);
}
