#include "commandcenter.h"

CommandCenter::CommandCenter()
	:QObject()
{
}

CommandCenter::~CommandCenter()
{
}

void CommandCenter::addFunc(const QString& cmd, cmdFunc func)
{
	QHash<QString, cmdFunc>::iterator it = m_funcs.find(cmd);
	if (func && it == m_funcs.end())
		m_funcs.insert(cmd, func);
}

bool CommandCenter::execute(const QString& cmd)
{
	QStringList argv = cmd.split(" ");
	if (argv.size() > 0)
	{
		QHash<QString, cmdFunc>::iterator it = m_funcs.find(argv[0]);
		if (it != m_funcs.end())
		{
			argv.pop_front();
			it.value()(argv);
			return true;
		}
	}

	return false;
}
