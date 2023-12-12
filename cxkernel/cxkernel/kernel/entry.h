#ifndef CXKERNEL_QUICK_MAIN_1601388211209_H
#define CXKERNEL_QUICK_MAIN_1601388211209_H
#include "cxkernel/cxkernelinterface.h"
#include <QtQml/QQmlContext>
#include <QtQml/QQmlApplicationEngine>
#include <functional>

class QApplication;
class QQuickView;
class QGuiApplication;
namespace cxkernel
{
	class AppModule
	{
	public:
		virtual ~AppModule() {}

		virtual bool useFrameless() { return false; }
		virtual bool loadQmlEngine(QObject* object, QQmlEngine& engine) = 0;
		virtual void unloadQmlEngine() = 0;
		virtual void shutDown() = 0;
	};

	typedef std::function<AppModule*()> AppModuleCreateFunc;
	CXKERNEL_API int appMain(int argc, char* argv[], AppModuleCreateFunc func);
}

#endif // CXKERNEL_QUICK_MAIN_1601388211209_H
