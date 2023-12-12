#ifndef _QML_ENTRY_1589423984319_H
#define _QML_ENTRY_1589423984319_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/statenotifier.h"
#include "qtusercore/module/translatenotifier.h"
#include <QtCore/QObject>

class QTUSER_CORE_API QmlEntry: public QObject
	, public StateReceiver
	, public TranslatorReceiver
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
	Q_PROPERTY(bool operateEnabled READ operateEnabled NOTIFY operateEnabledChanged)
	Q_PROPERTY(QString enabledIcon READ enabledIcon NOTIFY enabledIconChanged)
	Q_PROPERTY(QString disableIcon READ disableIcon NOTIFY disableIconChanged)
	Q_PROPERTY(QString pressedIcon READ pressedIcon NOTIFY pressedIconChanged)
	Q_PROPERTY(QString source READ source NOTIFY sourceChanged)
	Q_PROPERTY(QString name READ name NOTIFY nameChanged)
	Q_PROPERTY(bool shot READ shot CONSTANT)
public:
	QmlEntry(QObject* parent = nullptr);
	virtual ~QmlEntry();
public:
	bool enabled();
	bool operateEnabled();
	bool shot();
	QString enabledIcon() const;
	QString disableIcon() const;
	QString pressedIcon() const;
	QString source() const;
	QString name();

	Q_INVOKABLE void execute();
public:
	void setEnabledIcon(const QString& icon);
	void setPressedIcon(const QString& icon);
	void setDisableIcon(const QString& icon);
	void setName(const QString& name);
    void setSource(const QString& source);
	void setEnabled(bool enabled);
	void updateEnabled();

	int order();
protected:
	virtual bool checkEnabled();
	virtual bool checkOperateEnabled();
	virtual void onExecute();
	virtual QString nameImpl();
	virtual bool isShot();

	void updateState(unsigned mask) override;
	void updateTrans() override;
signals:
	void enabledChanged();
	void enabledIconChanged();
	void disableIconChanged();
	void pressedIconChanged();
	void sourceChanged();
	void nameChanged();
	void operateEnabledChanged();
protected:
	bool m_enabled;
	QString m_enabledIcon;
	QString m_disableIcon;
	QString m_pressedIcon;
	QString m_source;
	QString m_name;

	int m_order;
};
#endif // _QML_ENTRY_1589423984319_H
