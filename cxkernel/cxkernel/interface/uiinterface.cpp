#include "uiinterface.h"
#include "cxkernel/kernel/cxkernel.h"
#include "cxkernel/kernel/qmlui.h"

namespace cxkernel
{
	QObject* createQmlObjectFromQrc(const QString& fileName)
	{
		return cxKernel->qmlUI()->createQmlObjectFromQrc(fileName);
	}

	void registerContextObject(const QString& name, QObject* object)
	{
		cxKernel->qmlUI()->registerContextObject(name, object);
	}

	void registerImageProvider(const QString& name, QQuickImageProvider* provider)
	{
		cxKernel->qmlUI()->registerImageProvider(name, provider);
	}

	void removeImageProvider(const QString& name)
	{
		cxKernel->qmlUI()->removeImageProvider(name);
	}

	void setObjectOwnership(QObject* object)
	{
		cxKernel->qmlUI()->setObjectOwnership(object);
	}

	QVariant invokeQmlJs(const QString& script)
	{
		return cxKernel->qmlUI()->invokeQmlJs(script);
	}

	QJSValue invokeJS(const QString& script, const QString& name, QObject* context)
	{
		return cxKernel->qmlUI()->invokeJS(script, name, context);
	}

	QObject* findItem(const QString& name)
	{
		return cxKernel->qmlUI()->itemByName(name);
	}

	void invokeItemFunc(const QString& name, const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		cxKernel->qmlUI()->invokeItemFunc(name, func, variant1, variant2);
	}
}