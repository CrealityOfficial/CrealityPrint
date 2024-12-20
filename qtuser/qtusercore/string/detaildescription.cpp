#include "detaildescription.h"

namespace qtuser_core
{
	DetailDescription::DetailDescription(QObject* parent)
		:QObject(parent)
	{
	}

	DetailDescription::~DetailDescription()
	{
	}

	QString DetailDescription::get(const QString& key)
	{
		QHash<QString, QString>::iterator it = m_details.find(key);
		if (it != m_details.end())
			return it.value();
		return QString();
	}

	void DetailDescription::set(const QString& key, const QString& text)
	{
		QHash<QString, QString>::iterator it = m_details.find(key);
		if (it != m_details.end())
			it.value() = text;
		
		m_details.insert(key, text);
	}

	QStringList DetailDescription::enumKeys()
	{
		QStringList keys;
		for (QHash<QString, QString>::iterator it = m_details.begin(); it != m_details.end(); ++it)
			keys << it.key();
		return keys;
	}

	void DetailDescription::parse(const QString& str)
	{
		QStringList strs = str.split(";");
		for (const QString& s : strs)
		{
			QStringList kv = s.split("=");
			if (kv.size() == 2)
			{
				m_details.insert(kv.at(0), kv.at(1));
			}
		}
	}

	float DetailDescription::value(const QString& key)
	{
		QString v;
		QHash<QString, QString>::iterator it = m_details.find(key);
		if (it != m_details.end())
			v = it.value();

		bool ok = false;
		float d = v.toFloat(&ok);
		if (ok)
			return d;
		return 0.0f;
	}

	void DetailDescription::clear()
	{
		m_details.clear();
	}
}
