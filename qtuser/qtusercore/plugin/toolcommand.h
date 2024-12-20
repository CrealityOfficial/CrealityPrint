#ifndef _NULLSPACE_TOOLCOMMAND_1589423984319_H
#define _NULLSPACE_TOOLCOMMAND_1589423984319_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QObject>

class QTUSER_CORE_API ToolCommand: public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)

	Q_PROPERTY(QString enabledIcon READ enabledIcon NOTIFY enabledIconChanged)
	Q_PROPERTY(QString hoveredIcon READ hoveredIcon NOTIFY hoveredIconChanged)
	Q_PROPERTY(QString pressedIcon READ pressedIcon NOTIFY pressedIconChanged)
	Q_PROPERTY(QString disabledIcon READ disableIcon NOTIFY disableIconChanged)
	Q_PROPERTY(QString name READ name NOTIFY nameChanged)

    Q_PROPERTY(QString source READ source)
public:
	ToolCommand(QObject* parent = nullptr);
	virtual ~ToolCommand();
    int orderindex;
	bool enabled();
	QString enabledIcon() const;
	QString hoveredIcon() const;
	QString disableIcon() const;
	QString pressedIcon() const;
	QString source() const;
	QString name() const;
	void setEnabledIcon(const QString& icon);
	void setHoveredIcon(const QString& icon);
	void setPressedIcon(const QString& icon);
	void setDisabledIcon(const QString& icon);
	void setName(const QString& name);
    void setSource(const QString& source);
    Q_INVOKABLE virtual bool checkSelect();
	Q_INVOKABLE virtual bool outofPlatform();
	void setEnabled(bool enabled);

	Q_INVOKABLE void execute();
	Q_INVOKABLE virtual bool isSelectModel();
	virtual void onExecute();

protected:
	virtual bool enableImpl();

signals:
	void enabledChanged();
	void executed();
	void enabledIconChanged();
	void hoveredIconChanged();
	void pressedIconChanged();
	void disableIconChanged();
	void nameChanged();
private:
	bool m_enabled;

protected:
	Q_INVOKABLE void saveCall();
	QString m_enabledIcon;
	QString m_hoveredIcon;
	QString m_disableIcon;
	QString m_pressedIcon;
	QString m_source;
	QString m_name;
};
#endif // _NULLSPACE_TOOLCOMMAND_1589423984319_H
