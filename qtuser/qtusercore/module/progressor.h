#ifndef _QTUSER_CORE_PROGRESSOR_1590984418923_H
#define _QTUSER_CORE_PROGRESSOR_1590984418923_H
#include <QtCore/QObject>
namespace qtuser_core
{
	class Progressor
	{
	public:
		virtual ~Progressor() {}

		virtual bool interrupt() = 0;
		virtual void progress(float r) = 0;
		virtual void failed(const QString& message) = 0;
		virtual void message(const QString& msg) = 0;
		virtual QString getFailReason() = 0;
	};
}
#endif // _QTUSER_CORE_PROGRESSOR_1590984418923_H
