#ifndef _COMMANDCENTER_1598966059698_H
#define _COMMANDCENTER_1598966059698_H
#include "qtusercore/module/singleton.h"
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <functional>

typedef std::function<void(const QStringList& argv)> cmdFunc;

class QTUSER_CORE_API CommandCenter : public QObject
{
	Q_OBJECT
public:
	CommandCenter();
	virtual ~CommandCenter();

	void addFunc(const QString& cmd, cmdFunc func);
	bool execute(const QString& cmd);
protected:
	QHash<QString, cmdFunc> m_funcs;
};

#define FUNC(x) std::bind(&x, this, std::placeholders::_1)
#endif // _COMMANDCENTER_1598966059698_H