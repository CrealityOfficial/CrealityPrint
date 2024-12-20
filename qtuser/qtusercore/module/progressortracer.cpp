#include "progressortracer.h"
#include "qtusercore/module/progressor.h"
#include <QtCore/QDebug>

namespace qtuser_core
{
	ProgressorTracer::ProgressorTracer(Progressor* progressor, QObject* parent)
		: QObject(parent)
		, m_progressor(progressor)
		, m_failed(false)
	{
		resetProgressScope();
	}

	ProgressorTracer::~ProgressorTracer()
	{

	}

	void ProgressorTracer::progress(float r)
	{
		m_realValue = m_start + r * (m_end - m_start);

#if 0
		qDebug() << QString("ProgressorTracer::progress [%1]").arg(m_realValue);
#endif
		if (m_progressor)
			m_progressor->progress(m_realValue);
	}

	bool ProgressorTracer::interrupt()
	{
		if (m_progressor)
			return m_progressor->interrupt();
		return false;
	}

	void ProgressorTracer::message(const char* msg)
	{
		qDebug() << "ProgressorTracer : " << msg;
	}

	void ProgressorTracer::failed(const char* msg)
	{
		qDebug() << "ProgressorTracer failed: " << msg;
		if (m_progressor)
			m_progressor->failed(QString::fromLocal8Bit(msg));
		m_failed = true;
	}

	QString ProgressorTracer::getFailReason()
	{
		if (m_progressor)
			return m_progressor->getFailReason();
		return QString();
	}

	void ProgressorTracer::success()
	{
		qDebug() << "ProgressorTracer success. ";
	}

	void ProgressorTracer::recordExtraMessage(const char* keyMsg, const char* valueMsg, size_t objId)
	{
		m_extraSliceWarnings[std::string(keyMsg)] = std::make_pair(std::string(valueMsg), objId);
	}

	int ProgressorTracer::extraMessageSize()
	{
		return m_extraSliceWarnings.size();
	}

	std::map<std::string, std::pair<std::string,size_t> > ProgressorTracer::getExtraRecordMessage()
	{
		return m_extraSliceWarnings;
	}

	bool ProgressorTracer::error()
	{
		return m_failed;
	}

	ProgressorMessageTracer::ProgressorMessageTracer(
		std::function<void(const char*)> message_callback,
		Progressor* progressor,
		QObject* parent)
			: ProgressorTracer(progressor, parent)
			, message_callback_(message_callback) {}

	void ProgressorMessageTracer::message(const char* message) {
		if (message_callback_) {
			message_callback_(message);
		}
	}
}
