#ifndef QTUSER_CORE_PROFILER_1596548246988_H
#define QTUSER_CORE_PROFILER_1596548246988_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/singleton.h"
#include <QtCore/QMap>

namespace qtuser_core
{
	class CSVWriter;
	class QTUSER_CORE_API Profiler : public QObject
	{
		Q_OBJECT
		SINGLETON_DECLARE(Profiler)
	public:
		virtual ~Profiler();

		void start(const QString& name, const QStringList& keys);
		void push(const QString& name, double value);
		void tick(const QString& name);
		void ticks(const QString& name);
		void ticke(const QString& name);
		void delta(const QString& name);
		void output(const QString& name, const QString& file);
	protected:
		QMap<QString, CSVWriter*> m_writers;
	};
	
	SINGLETON_EXPORT(QTUSER_CORE_API, Profiler)
}

#ifndef DISABLE_PROFILE
#define PROFILE_START(x, y) qtuser_core::getProfiler()->start(x, y)
#define PROFILE_PUSH(x, y) qtuser_core::getProfiler()->push(x, y)
#define PROFILE_TICK(x) qtuser_core::getProfiler()->tick(x)
#define PROFILE_DELTA(x) qtuser_core::getProfiler()->delta(x)
#define PROFILE_OUTPUT(x, y) qtuser_core::getProfiler()->output(x, y)
#define PROFILE_TICKS(x) qtuser_core::getProfiler()->ticks(x)
#define PROFILE_TICKE(x) qtuser_core::getProfiler()->ticke(x)
#else
#define PROFILE_START(x, y)
#define PROFILE_PUSH(x, y)
#define PROFILE_TICK(x)
#define PROFILE_DELTA(x)
#define PROFILE_OUTPUT(x, y)
#endif

#endif // QTUSER_CORE_PROFILER_1596548246988_H