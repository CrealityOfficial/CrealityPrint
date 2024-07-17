#include "stringhistory.h"
#include "qtusercore/util/settings.h"

namespace qtuser_core
{
	StringHistory::StringHistory(const QString& countID, const QString& stringID, QObject* parent)
		:QObject(parent)
		, m_countID(countID)
		, m_stringsID(stringID)
	{
		if (m_countID.isEmpty())
			m_countID = QString("DefaultCountID");
		if (m_stringsID.isEmpty())
			m_stringsID = QString("DefaultFilesID");

		qtuser_core::VersionSettings settings;
		if (!settings.value(m_countID).isValid())
			settings.setValue(m_countID, QVariant(4));

		if (!settings.allKeys().contains(m_stringsID)) {
			settings.setValue(m_stringsID, QVariant(QStringList()));
		}
	}

	StringHistory::~StringHistory()
	{

	}

	QStringList StringHistory::strings()
	{
		qtuser_core::VersionSettings settings;
		int numOfRecentFiles = settings.value(m_countID, QVariant(4)).toInt();
		QStringList MyRecentFileList = settings.value(m_stringsID).toStringList();

		int nCount = MyRecentFileList.size();
		while (nCount - numOfRecentFiles > 0)
		{
			MyRecentFileList.pop_front();
			nCount--;
		}

		settings.setValue(m_stringsID, QVariant(MyRecentFileList));
		return MyRecentFileList;
	}

	QString StringHistory::lastOne()
	{
		QStringList strs = strings();
		if (strs.size() > 0)
			return strs.last();
		return QString();
	}

	void StringHistory::setNum(int count)
	{
		int n = count;
		if (n <= 0)
			n = 4;
		{
			qtuser_core::VersionSettings settings;
			settings.setValue(m_countID, QVariant(n));
		}

		emit sigDataChange(strings());
	}
	void StringHistory::removeString(const QString& str)
	{
		if (str.isEmpty())
			return;
		qtuser_core::VersionSettings settings;
		QStringList recentFileList = settings.value(m_stringsID).toStringList();
		recentFileList.removeOne(str);
		settings.setValue(m_stringsID, QVariant(recentFileList));
		emit sigDataChange(strings());

	}
	void StringHistory::setRecentString(const QString& str)
	{
		if (str.isEmpty())
			return;

		{
			qtuser_core::VersionSettings settings;
			QStringList recentFileList = settings.value(m_stringsID).toStringList();
			recentFileList.removeOne(str);
			recentFileList.append(str);
			settings.setValue(m_stringsID, QVariant(recentFileList));
		}

		emit sigDataChange(strings());
	}

	void StringHistory::clear()
	{
		{
			qtuser_core::VersionSettings settings;
			QStringList recentFileList;
			settings.setValue(m_stringsID, QVariant(recentFileList));
		}
		emit sigDataChange(strings());
	}
}
