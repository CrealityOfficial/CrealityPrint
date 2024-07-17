#ifndef CREATIVE_KERNEL_UIINTERFACE_1592721604112_H
#define CREATIVE_KERNEL_UIINTERFACE_1592721604112_H
#include "basickernelexport.h"
#include "data/interface.h"
#include <QtCore/QVariant>
#include <QtQml/QJSValue>

class ToolCommand;
class QQuickImageProvider;
namespace creative_kernel
{
	BASIC_KERNEL_API void addLCommand(ToolCommand* command);
	BASIC_KERNEL_API void removeLCommand(ToolCommand* command);

	BASIC_KERNEL_API QObject* getUIRoot();
	BASIC_KERNEL_API QObject* getUIAppWindow();
	BASIC_KERNEL_API QObject* getFooter();

    BASIC_KERNEL_API QObject* getUIRightPanel();

    BASIC_KERNEL_API QObject* getGLMainView();

	BASIC_KERNEL_API void registerContextObject(const QString& name, QObject* object);
	BASIC_KERNEL_API void registerImageProvider(const QString& name, QQuickImageProvider* provider);
	BASIC_KERNEL_API void removeImageProvider(const QString& name);

	BASIC_KERNEL_API void executeQmlCommand(const QString& cmd, QObject* receiver, const QString& objectName);
	BASIC_KERNEL_API void requestQmlDialog(QObject* receiver, const QString& objectName);
	BASIC_KERNEL_API void requestQmlDialog(const QString& objectName);
	BASIC_KERNEL_API void requestQmlTipDialog(const QString& message);

	BASIC_KERNEL_API void requestQmlMessage(QObject* messageSource);
	BASIC_KERNEL_API void destroyQmlMessage(int code);

	BASIC_KERNEL_API void requestQmlCloseAction();
	BASIC_KERNEL_API void setCloseHook(CloseHook* hook);

	BASIC_KERNEL_API QObject* createQmlObjectFromQrc(const QString& fileName);

	BASIC_KERNEL_API void invokeCloudLoginPanelFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API void invokeCloudUserInfoFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API void invokeModelLibraryDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API void invokeModelRecommendDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    BASIC_KERNEL_API void invokeUploadGcodeDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API void invokeUploadModelDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    BASIC_KERNEL_API void invokeModelFDMPreviewFunc(const char* func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API void invokeRightClickMenuFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API void invokeMainWindowFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
	BASIC_KERNEL_API QJSValue invokeJS(const QString& str,QObject* jsContex);
}
#endif // CREATIVE_KERNEL_UIINTERFACE_1592721604112_H
