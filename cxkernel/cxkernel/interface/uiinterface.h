/**
 * @file uiinterface.h
 * @author zenggui (anoob@sina.cn)
 * @brief 
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef CXKERNEL_UIINTERFACE_1681977959351_H
#define CXKERNEL_UIINTERFACE_1681977959351_H
#include "cxkernel/cxkernelinterface.h"
#include <QtQuick/QQuickImageProvider>

namespace cxkernel
{
	CXKERNEL_API QObject* createQmlObjectFromQrc(const QString& fileName);

	CXKERNEL_API void registerContextObject(const QString& name, QObject* object);
	CXKERNEL_API void registerImageProvider(const QString& name, QQuickImageProvider* provider);
	CXKERNEL_API void removeImageProvider(const QString& name);

	CXKERNEL_API void setObjectOwnership(QObject* object);

	CXKERNEL_API QVariant invokeQmlJs(const QString& script);
	CXKERNEL_API QJSValue invokeJS(const QString& script, const QString& name, QObject* context);

	CXKERNEL_API QObject* findItem(const QString& name);
	CXKERNEL_API void invokeItemFunc(const QString& name, const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
}

#endif // CXKERNEL_UIINTERFACE_1681977959351_H