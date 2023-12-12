#include "formattracer.h"

namespace qtuser_core
{
	FormatTracer::FormatTracer(Progressor* progressor, QObject* parent)
		: ProgressorTracer(progressor, parent)
	{
	}

	FormatTracer::~FormatTracer()
	{

	}

	void FormatTracer::variadicFormatMessage(int msg, ...)
	{
		va_list ap;
		va_start(ap, msg);

		if (m_progressor)
		{
			int count = indexCount();
			QString lan("Error");
			if (msg >= 0 && msg < count)
			{
				lan = indexStr(msg, ap);
			}
			m_progressor->message(lan);
		}

		va_end(ap);
	}

	void FormatTracer::message(const char* msg)
	{
		if (m_progressor)
		{
			//QString lan(msg);
			//if (indexCount() > 0)
			//{
			//	int count = indexCount();
			//	for (int i = 0; i < count; ++i)
			//		lan.replace(QString("{%1}").arg(i), indexStr(i));
			//}
			//m_progressor->message(lan);
		}
	}

	int FormatTracer::indexCount()
	{
		return 0;
	}

	QString FormatTracer::indexStr(int msg, va_list vars)
	{
		return QString();
	}

	QString FormatTracer::rich(const QString& msg)
	{
		QString result = msg;
		result.replace("<", "&lt;");
		result.replace(">", "&gt;");
		result.replace("\n", "<br />");
		return QString("<font color=\"#F64B80\">%1</font>").arg(result);
	}
}
