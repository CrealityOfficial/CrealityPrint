#ifndef QTUSER_CORE_PROGRESSORTRACER_1635210496419_H
#define QTUSER_CORE_PROGRESSORTRACER_1635210496419_H

#include <functional>

#include "ccglobal/tracer.h"
#include "qtusercore/qtusercoreexport.h"

namespace qtuser_core
{
	class Progressor;
	class QTUSER_CORE_API ProgressorTracer : public QObject,
		public ccglobal::Tracer
	{
	public:
		ProgressorTracer(Progressor* progressor, QObject* parent = nullptr);
		virtual ~ProgressorTracer();

		void progress(float r) override;
		bool interrupt() override;

		void message(const char* msg) override;
		void failed(const char* msg) override;
		void success() override;

		void recordExtraMessage(const char* keyMsg, const char* valueMsg, size_t objId) override;
		int extraMessageSize() override;
		std::map< std::string, std::pair<std::string, size_t> > getExtraRecordMessage() override;

		bool error();

		QString getFailReason();

	protected:
		Progressor* m_progressor;
		bool m_failed;
		std::map< std::string, std::pair<std::string, size_t> > m_extraSliceWarnings;
	};

	class QTUSER_CORE_API ProgressorMessageTracer : public ProgressorTracer {
		Q_OBJECT;
	public:
		explicit ProgressorMessageTracer(std::function<void(const char*)> message_callback,
																		 Progressor* progressor,
																		 QObject* parent = nullptr);
		virtual ~ProgressorMessageTracer() = default;

	public:
		void message(const char* message) override;

	private:
		std::function<void(const char*)> message_callback_{ nullptr };
	};
}

#endif // QTUSER_CORE_PROGRESSORTRACER_1635210496419_H
