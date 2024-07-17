#include "uiinterface.h"
#include "kernel/kernelui.h"
#include "kernel/translator.h"
#include "qtusercore/plugin/toolcommandcenter.h"

namespace creative_kernel
{
	void addLCommand(ToolCommand* command)
	{
		getKernelUI()->lCenter()->addCommand(command);
	}

	void removeLCommand(ToolCommand* command)
	{
		getKernelUI()->lCenter()->removeCommand(command);
	}

	QObject* getUIRoot()
	{
		return getKernelUI()->root();
	}

	QObject* getUIAppWindow()
	{
		return getKernelUI()->appWindow();
	}
    QObject* getUIRightPanel()
    {
        return getKernelUI()->rightPanel();
    }

	QObject* getFooter()
	{
		return getKernelUI()->footer();
	}

    QObject* getGLMainView()
    {
        return getKernelUI()->glMainView();
    }

	void registerContextObject(const QString& name, QObject* object)
	{
		getKernelUI()->registerContextObject(name, object);
	}

	void registerImageProvider(const QString& name, QQuickImageProvider* provider)
	{
		getKernelUI()->registerImageProvider(name, provider);
	}

	void removeImageProvider(const QString& name)
	{
		getKernelUI()->removeImageProvider(name);
	}

	void executeQmlCommand(const QString& cmd, QObject* receiver, const QString& objectName)
	{
		getKernelUI()->executeQmlCommand(cmd, receiver, objectName);
	}

	void requestQmlDialog(QObject* receiver, const QString& objectName)
	{
		getKernelUI()->requestQmlDialog(receiver, objectName);
	}

	void requestQmlTipDialog(const QString& message)
	{
		getKernelUI()->requestQmlTipDialog(message);
	}

	void requestQmlDialog(const QString& objectName)
	{
		getKernelUI()->requestQmlDialog(objectName);
	}
	void requestQmlMessage(QObject* messageSource)
	{
		getKernelUI()->requestMessage(messageSource);
	}

	void destroyQmlMessage(int code)
	{
		getKernelUI()->destroyMessage(code);
	}

	void destroyFloatMessage(int code)
	{

	}

	void requestQmlCloseAction()
	{
		getKernelUI()->requestQmlCloseAction();
	}

	void setCloseHook(CloseHook* hook)
	{
		getKernelUI()->setCloseHook(hook);
	}

	QObject* createQmlObjectFromQrc(const QString& fileName)
	{
		return getKernelUI()->createQmlObjectFromQrc(fileName);
	}

	void invokeCloudLoginPanelFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeCloudLoginPanelFunc(func, variant1, variant2);
	}

	void invokeCloudUserInfoFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeCloudUserInfoFunc(func, variant1, variant2);
	}

	void invokeModelLibraryDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeModelLibraryDialogFunc(func, variant1, variant2);
	}

	void invokeModelRecommendDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeModelRecommendDialogFunc(func, variant1, variant2);
	}

    void invokeUploadGcodeDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
    {
        getKernelUI()->invokeUploadGcodeDialogFunc(func, variant1, variant2);
    }

	void invokeUploadModelDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeUploadModelDialogFunc(func, variant1, variant2);
	}

	void invokeModelFDMPreviewFunc(const char* func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeModelFDMPreviewFunc(func, variant1, variant2);
	}

	void invokeRightClickMenuFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeRightClickMenuFunc(func, variant1, variant2);
	}

	void invokeMainWindowFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
	{
		getKernelUI()->invokeMainWindowFunc(func, variant1, variant2);
	}

	QJSValue invokeJS(const QString& str,QObject *jsContex)
	{
		return getKernelUI()->invokeJS(str, jsContex);
	}
}
