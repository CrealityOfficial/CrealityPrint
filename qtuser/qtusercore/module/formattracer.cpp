#include "formattracer.h"
#include <QtCore/QCoreApplication>

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
			QString qstrMsg = msg;
			if (qstrMsg.contains("layer"))
			{
				QStringList list = qstrMsg.split("layer");
				if (list.size() !=2)
				{
					QString lan = QCoreApplication::translate("ParameterComponent", msg);
					m_progressor->message(lan);
				}
				else
				{
					QString msg2 = list[0] + "layer %1";
					QByteArray ba = msg2.toLatin1();
					int layer = list[1].toInt();
					QString lan = QCoreApplication::translate("ParameterComponent", ba.data()).arg(layer);
					m_progressor->message(lan);
				}
			}
			else
			{
				QString lan = QCoreApplication::translate("ParameterComponent", msg);
				m_progressor->message(lan);
			}
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
