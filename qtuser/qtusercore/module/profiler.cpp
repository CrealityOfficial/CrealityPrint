#include "profiler.h"
#include "qtusercore/module/csvwriter.h"

namespace qtuser_core
{
	SINGLETON_IMPL(Profiler)
	Profiler::Profiler()
		:QObject()
	{
	}
	
	Profiler::~Profiler()
	{
	}

	void Profiler::start(const QString& name, const QStringList& keys)
	{
		CSVWriter* writer = nullptr;
		auto it = m_writers.find(name);
		if (it != m_writers.end())
			writer = *it;
		else
		{
			writer = new CSVWriter(this);
			m_writers.insert(name, writer);
		}

		writer->clear();
		for (const QString& key : keys)
			writer->pushHead(key.toStdString());

		writer->start();
	}

	void Profiler::push(const QString& name, double value)
	{
		auto it = m_writers.find(name);
		if (it != m_writers.end())
		{
			(*it)->pushData(value);
		}
	}

	void Profiler::tick(const QString& name)
	{
		auto it = m_writers.find(name);
		if (it != m_writers.end())
		{
			(*it)->tick();
		}
	}

	void Profiler::ticks(const QString& name)
	{
		auto it = m_writers.find(name);
		if (it != m_writers.end())
		{
			(*it)->tickStart();
		}
	}

	void Profiler::ticke(const QString& name)
	{
		auto it = m_writers.find(name);
		if (it != m_writers.end())
		{
			(*it)->tickEnd();
		}
	}

	void Profiler::delta(const QString& name)
	{
		auto it = m_writers.find(name);
		if (it != m_writers.end())
		{
			(*it)->tick(true);
		}
	}

	void Profiler::output(const QString& name, const QString& file)
	{
		auto it = m_writers.find(name);
		if (it != m_writers.end())
		{
			(*it)->output(file.toStdString());
		}
	}
}