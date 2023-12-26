#ifndef _creative_kernel_KERNELUI_1589818161757_H
#define _creative_kernel_KERNELUI_1589818161757_H
#include "data/interface.h"
#include "qtusercore/module/singleton.h"
#include "qtusercore/plugin/toolcommand.h"
#include "qtuser3d/event/eventhandlers.h"
#include "menu/actioncommand.h"
#include "data/modeln.h"

#include <QtQml/QQmlApplicationEngine>
#include <QtCore/QAbstractListModel>
#include <QTranslator>
#include <QUrl>
#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QQuickItem>

namespace qtuser_qml
{
	class ToolCommandCenter;

    enum class ToolCommandGroupType : int {
        LEFT_TOOLBAR_MAIN,
        LEFT_TOOLBAR_OTHER,
    };

    enum class ToolCommandType : int {
        PICK             = 0 ,
        TRANSLATE        = 1 ,
        SCALE            = 2 ,
        ROTATE           = 3 ,
        CLONE            = 4 , // other panel
        LETTER           = 5 , // other panel
        PICK_BUTTON      = 6 ,
        SPLIT            = 7 , // other panel
        LAYOUT           = 8 ,
        SUPPORT_PAINTING = 9 ,
        MIRROR           = 10, // other panel
        DRILL            = 11, // other panel
        DISTANCE_MEASURE = 12, // other panel
        HALLOW           = 13, // other panel
        SEAM_PAINTING    = 14, // other panel
        MODEL_EFFECT     = 15,
    };
}

namespace creative_kernel
{
    class Translator;
}

class ToolCommand;
class QQuickImageProvider;
class SliceUI;
class BASIC_KERNEL_API KernelUI: public QObject
    , public qtuser_3d::RightMouseEventHandler
    , public creative_kernel::UIVisualTracer
    , public qtuser_3d::KeyEventHandler
{
	Q_OBJECT
    Q_PROPERTY(QObject* root READ root CONSTANT)

    Q_PROPERTY(bool firstStart READ isFirstStart CONSTANT);

	Q_PROPERTY(QObject* leftToolbar READ leftToolbar WRITE setLeftToolbar)
    Q_PROPERTY(QObject* footer READ footer WRITE setFooter)
    Q_PROPERTY(QObject* topbar READ topBar WRITE setTopBar)
	Q_PROPERTY(QObject* appWindow READ appWindow WRITE setAppWindow)
	// Q_PROPERTY(QAbstractListModel* lmodel READ lmodel CONSTANT)

    Q_PROPERTY(QAbstractListModel* lMainModel READ lMainModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* lOtherModel READ lOtherModel CONSTANT)

    Q_PROPERTY(QObject* rightPanel READ rightPanel WRITE setRightPanel)
    Q_PROPERTY(QObject* glMainView READ glMainView WRITE setGLMainView)

    Q_PROPERTY(QStringList fontList READ getFontList WRITE setFontList);

    Q_PROPERTY(int currentLanguage READ currentLanguage WRITE changeLanguage NOTIFY currentLanguageChanged);
public:
    KernelUI(QObject* parent = nullptr);
	virtual ~KernelUI();

    void initialize();
    void uninitialize();

    Q_INVOKABLE void changeLanguage(int language);
    Q_INVOKABLE int currentLanguage();
    Q_INVOKABLE void firstStartConfigFinished();
    Q_INVOKABLE void copyString2Clipboard(const QString &str);
    void loadUserLanguage();
    void loadUserSettings();
    void registerContextObject(const QString& name, QObject* object);
    void registerImageProvider(const QString& name, QQuickImageProvider* provider);
    void removeImageProvider(const QString& name);

    void switchPickMode();

    void executeQmlCommand(const QString& cmd, QObject* receiver, const QString& objectName);
    void requestQmlDialog(QObject* receiver, const QString& objectName);
    Q_INVOKABLE void requestQmlDialog(const QString& objectName);
    Q_INVOKABLE void requestQmlTipDialog(const QString& message);
    void requestQmlCloseAction();
    Q_INVOKABLE bool onCopy();
    Q_INVOKABLE bool onPaste();

    QObject* createQmlObjectFromQrc(const QString& fileName);

    Q_INVOKABLE void invokeCloudUserInfoFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeCloudLoginPanelFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeRightClickMenuFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeModelLibraryDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeModelRecommendDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeModelUploadDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeUploadGcodeDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    void invokeUploadModelDialogFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    Q_INVOKABLE void invokeModelFDMPreviewFunc(const char* func, const QVariant& variant1, const QVariant& variant2);
    void invokeMainWindowFunc(const QString& func, const QVariant& variant1 = QVariant(), const QVariant& variant2 = QVariant());
    QJSValue invokeJS(const QString& str, QObject* jsContex);
    QJSValue invokeJSFunc(const QString& str, QObject* jsContex);

    QObject* getUI(QString uiname);

    int addToolCommand(ToolCommand* command, qtuser_qml::ToolCommandGroupType group, int index);
    int addToolCommand(ToolCommand* command, qtuser_qml::ToolCommandGroupType group, qtuser_qml::ToolCommandType type);
    int removeToolCommand(ToolCommand* command, qtuser_qml::ToolCommandGroupType group);

    int addActionCommand(ActionCommand* command, QString key);
    int removeActionlCommand(ActionCommand* command, QString key);

    creative_kernel::Translator* translator();

	QObject* leftToolbar();
	void setLeftToolbar(QObject* leftToolbar);
    QObject* footer();
    void setFooter(QObject* footer);
    QObject* topBar();
    void setTopBar(QObject* topbar);
	QObject* root();

    QObject* appWindow();
    void setAppWindow(QObject* appWindow);

    QObject* rightPanel();
    void setRightPanel(QObject* rightPanel);

    QObject* glMainView();
    void setGLMainView(QObject* glMainView);

    bool isFirstStart() const;

    // see ToolCommandGroupType & ToolCommandType
    qtuser_qml::ToolCommandCenter* lCenter();
    qtuser_qml::ToolCommandCenter* lOtherCenter();
    QAbstractListModel* lMainModel();
    QAbstractListModel* lOtherModel();

    QStringList getFontList() const;
    void setFontList(const QStringList&);

    void visibleAll(bool visible);

    //lisugui 2020-10-30 qml closewindow，signal closewindow to closaCmd function
    void setCloseHook(creative_kernel::CloseHook* hook);
    Q_INVOKABLE void beforeCloseWindow();
    //end

    void registerQmlEngine(QQmlEngine& engine);
    QQmlEngine* getQmlEngine();

    void setScene3DWrapper(QQuickItem* wrapper);

signals:
    void sigChangeMenuLanguage();
    void sigCloseAction();
    void closeWindow();
    void sigOpenglOld();
    void sigUseOpenGLES();
    void currentLanguageChanged();
protected:
    //bool eventFilter(QObject* object, QEvent* event) override;

    void onRightMouseButtonPress(QMouseEvent* event) override;
    void onRightMouseButtonRelease(QMouseEvent* event) override;
    void onRightMouseButtonMove(QMouseEvent* event) override;
    void onRightMouseButtonClick(QMouseEvent* event) override;
	void onKeyPress(QKeyEvent* event);
	void onKeyRelease(QKeyEvent* event);

    void onThemeChanged(creative_kernel::ThemeCategory category) override;
    void onLanguageChanged(creative_kernel::MultiLanguage language) override;
    void invokeQmlObjectFunc(QObject* object, const QString& func, const QVariant& variant1, const QVariant& variant2);
protected:
	QObject* m_appWindow;
	QObject* m_leftToolbar;
    QObject* m_footer;
    QObject* m_topbar;
    QObject* m_rightPanel;
    QObject* m_glMainView;
    QQuickItem* m_scene3DWrapper { NULL };

	qtuser_qml::ToolCommandCenter* m_leftToolbarModelList;
    qtuser_qml::ToolCommandCenter* m_leftOtherToolbarModelList;

    creative_kernel::Translator* m_translator;
    QQmlEngine* m_engine;

    QList<creative_kernel::ModelN*> m_copyModels;

    QStringList font_list_;

    creative_kernel::CloseHook* m_closeHook;
};

BASIC_KERNEL_API KernelUI* getKernelUI();

#endif // _creative_kernel_KERNELUI_1589818161757_H
