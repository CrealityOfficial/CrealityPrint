#ifndef QTUSER_CORE_FORMATTRACER_1635210496419_H
#define QTUSER_CORE_FORMATTRACER_1635210496419_H
#include "qtusercore/module/progressor.h"
#include "qtusercore/module/progressortracer.h"

namespace qtuser_core
{
	class QTUSER_CORE_API FormatTracer : public ProgressorTracer
	{
	public:
		FormatTracer(Progressor* progressor, QObject* parent = nullptr);
		virtual ~FormatTracer();

		void variadicFormatMessage(int msg, ...) override;
		void message(const char* msg) override;
	protected:
		QString rich(const QString& msg);

		virtual int indexCount();
		virtual QString indexStr(int msg, va_list vars);
	protected:
	};
}

#endif // QTUSER_CORE_FORMATTRACER_1635210496419_H
