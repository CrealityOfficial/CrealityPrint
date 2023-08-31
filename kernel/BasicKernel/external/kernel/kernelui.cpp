#include "kernel/kernelui.h"
#include "qtuserqml/plugin/toolcommandcenter.h"
#include "qtuserqml/plugin/toolcommand.h"
#include "qtusercore/module/systemutil.h"
#include "internal/menu/ccommandsdata.h"
#include "qtuser3d/event/eventsubdivide.h"
#include "qtuser3d/module/glcompatibility.h"

#include "interface/cloudinterface.h"
#include "interface/eventinterface.h"
#include "interface/selectorinterface.h"

#include "internal/rclick_menu/setnozzlerange.h"
#include "internal/rclick_menu/rclickmenulist.h"
#include "internal/tool/clonecommand.h"
#include "internal/tool/supportcommand.h"
#include "internal/tool/layouttoolcommand.h"
#include "internal/tool/pickmode.h"
#include "internal/tool/translatemode.h"
#include "internal/tool/rotatemode.h"
#include "internal/tool/scalemode.h"
#include "internal/tool/mirrorcommand.h"
#include "internal/menu/ccommandsdata.h"

#include <QtQml/QQmlProperty>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>

#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickImageProvider>
#include <QtQml/QQmlEngine>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtCore/QSettings>
#include <QtCore/QDebug>


#ifdef __APPLE__
#include "macwindow/macwindow.h"
#include "macwindow/mycppobject.h"
#include <QWindow>
#endif

#include "kernel/translator.h"
#include "interface/uiinterface.h"
#include "interface/modelinterface.h"
#include "interface/spaceinterface.h"
#include "cxkernel/interface/undointerface.h"
#include "interface/commandinterface.h"

#include "menu/ccommandlist.h"
#include "kernel/projectinfoui.h"
#include "kernel/kernel.h"
#include "kernel/wizardui.h"

KernelUI* getKernelUI()
{
    return creative_kernel::getKernel()->kernelUI();
}

using namespace creative_kernel;
KernelUI::KernelUI(QObject* parent)
    : QObject(parent)
    , m_appWindow(nullptr)
    , m_leftToolbar(nullptr)
    , m_rightPanel(nullptr)
    , m_glMainView(nullptr)
    , m_leftToolbarModelList(nullptr)
    , m_engine(nullptr)
    , m_footer(nullptr)
    , m_topbar(nullptr)
{
    // model list
    m_leftToolbarModelList = new qtuser_qml::ToolCommandCenter(this);
    m_leftOtherToolbarModelList = new qtuser_qml::ToolCommandCenter(this);

    m_translator = new Translator(this);

}

KernelUI::~KernelUI()
{
}

void KernelUI::initialize()
{
    addUIVisualTracer(this);
    ProjectInfoUI::createInstance(this);
    addRightMouseEventHandler(this);

    {
        using group_t = qtuser_qml::ToolCommandGroupType;
        using command_t = qtuser_qml::ToolCommandType;

        //addToolCommand(new PickMode         , group_t::LEFT_TOOLBAR_MAIN , command_t::PICK     );
        addToolCommand(new TranslateMode    , group_t::LEFT_TOOLBAR_MAIN , command_t::TRANSLATE);
        addToolCommand(new ScaleMode        , group_t::LEFT_TOOLBAR_MAIN , command_t::SCALE    );
        addToolCommand(new RotateMode       , group_t::LEFT_TOOLBAR_MAIN , command_t::ROTATE   );
        addToolCommand(new LayoutToolCommand, group_t::LEFT_TOOLBAR_MAIN , command_t::LAYOUT   );
        addToolCommand(new SupportCommand   , group_t::LEFT_TOOLBAR_MAIN , command_t::SUPPORT  );

        addToolCommand(new CloneCommand     , group_t::LEFT_TOOLBAR_OTHER, command_t::CLONE    );
        addToolCommand(new MirrorCommand    , group_t::LEFT_TOOLBAR_OTHER, command_t::MIRROR   );
    }

    visibleAll(false);//

	//check GL version
	if (qtuser_3d::isGles() || qtuser_3d::isSoftwareGL())
	{
		qDebug() << QString("OpenGL Driver is old.");
        emit sigOpenglOld();
	}
}

void KernelUI::uninitialize()
{
    removeImageProvider(QStringLiteral("login_qrcode_image_provider"));
    removeRightMouseEventHandler(this);
}

void KernelUI::changeLanguage(int language)
{
    return m_translator->changeLanguage((MultiLanguage)language);
}

int KernelUI::currentLanguage()
{
    return (int)creative_kernel::currentLanguage();
}

void KernelUI::firstStartConfigFinished()
{
    QSettings setting;
    setting.beginGroup("profile_setting");
    setting.setValue("first_start", "1");
}

void KernelUI::copyString2Clipboard(const QString &str)
{
    qtuser_core::copyString2Clipboard(str);
}

void KernelUI::loadUserLanguage()
{
    m_translator->loadUserSettings();
}

void KernelUI::loadUserSettings()
{
    assert(m_topbar);
    m_translator->loadUserSettings();

    creative_kernel::sensorAnlyticsTrace("Login", "start");
    QSettings setting;
    setting.beginGroup("profile_setting");
    QString strStartType = setting.value("first_start", "0").toString();

    QObject* pMachineCommbox = m_topbar->findChild<QObject*>("AddPrinterDlg");
    QQmlProperty::write(pMachineCommbox, "isFristStart", QVariant::fromValue(strStartType));
    if (strStartType == "0")
    {
        qInfo() << "startWizardComp first_start";
        QMetaObject::invokeMethod(m_topbar, "startFirstConfig");
    }
    setting.endGroup();
}

void KernelUI::registerContextObject(const QString& name, QObject* object)
{
    if (!m_engine)
    {
        qWarning() << "KernelUI::registerContextObject: [Null Engine]";
        return;
    }

    m_engine->rootContext()->setContextProperty(name, object);
}

void KernelUI::registerImageProvider(const QString& name, QQuickImageProvider* provider)
{
    if (!m_engine)
    {
        qWarning() << "KernelUI::registerImageProvider: [Null Engine]";
        return;
    }

    m_engine->addImageProvider(name, provider);
}

void KernelUI::removeImageProvider(const QString& name)
{
    if (!m_engine)
    {
        qWarning() << "KernelUI::removeImageProvider: [Null Engine]";
        return;
    }

    m_engine->removeImageProvider(name);
}

void KernelUI::switchPickMode()
{
    QMetaObject::invokeMethod(m_leftToolbar, "switchPickMode", Qt::ConnectionType::QueuedConnection);
}

void KernelUI::executeQmlCommand(const QString& cmd, QObject* receiver, const QString& objectName)
{
}

void KernelUI::requestQmlCloseAction()
{
    ProjectInfoUI::instance()->clearSecen();
    emit sigCloseAction();
}

 bool KernelUI::onCopy()
{
     m_copyModels = selectionms();
     if (m_copyModels.size() == 0)
     {
         return false;
     }
     return true;
}

 bool KernelUI::onPaste()
 {
     if (m_copyModels.size() == 0)
     {
         qDebug() << "paste invalid num.";
         return false;
     }

     for (ModelN* data : m_copyModels)
     {
         if (!data)
         {
             qDebug() << "model data has been released!";
             return false;
         }
     }

     creative_kernel::cmdClone(m_copyModels);
     return true;
 }

void KernelUI::requestQmlDialog(QObject* receiver, const QString& objectName)
{
    QObject* pMenu = m_appWindow->findChild<QObject*>("idAllMenuDialog");
    if (pMenu)
    {
        QMetaObject::invokeMethod(pMenu, "requestMenuDialog", Q_ARG(QVariant, QVariant::fromValue(receiver))
            , Q_ARG(QVariant, QVariant::fromValue(objectName)));
    }
}

void KernelUI::requestQmlDialog(const QString& objectName)
{
    QObject* pMenu = m_appWindow->findChild<QObject*>("idAllMenuDialog");
    if (pMenu)
    {
        QMetaObject::invokeMethod(pMenu, "requestDialog", Q_ARG(QVariant, QVariant::fromValue(objectName)));
    }
}

void KernelUI::requestQmlTipDialog(const QString& message) {
  QObject* menu = m_appWindow->findChild<QObject*>("idAllMenuDialog");
  if (menu == nullptr) { return; }
  QMetaObject::invokeMethod(menu, "requestTipDialog", Q_ARG(QVariant, QVariant::fromValue(message)));
}

QObject* KernelUI::createQmlObjectFromQrc(const QString& fileName)
{
    QQmlComponent component(m_engine, QUrl::fromLocalFile(fileName));
    return component.create(m_engine->rootContext());
}

void KernelUI::invokeCloudUserInfoFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* pTopBar = getUI("loginbtn");
    QObject* pUserInfoDlg = pTopBar->findChild<QObject*>("UserInfoDlg");
	invokeQmlObjectFunc(pUserInfoDlg, func, variant1, variant2);
}

void KernelUI::invokeCloudLoginPanelFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* pTopBar = getUI("loginbtn");
	QObject* pUserInfoDlg = pTopBar->findChild<QObject*>("LoginDlg");
	invokeQmlObjectFunc(pUserInfoDlg, func, variant1, variant2);
}

void KernelUI::invokeModelLibraryDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* object = m_appWindow->findChild<QObject*>("idModelLibraryInfoDialog");
    invokeQmlObjectFunc(object, func, variant1, variant2);
}

void KernelUI::invokeModelRecommendDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* object = m_appWindow->findChild<QObject*>("idModelRecommendDialog");
    invokeQmlObjectFunc(object, func, variant1, variant2);
}

void KernelUI::invokeModelUploadDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* object = m_appWindow->findChild<QObject*>("idUploadModelDialog");
    invokeQmlObjectFunc(object, func, variant1, variant2);
}

void KernelUI::invokeUploadGcodeDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* object = m_appWindow->findChild<QObject*>("idUploadGcodeDialog");
    invokeQmlObjectFunc(object, func, variant1, variant2);
}

void KernelUI::invokeUploadModelDialogFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* object = m_appWindow->findChild<QObject*>("idUploadModelDialog");
    invokeQmlObjectFunc(object, func, variant1, variant2);
}

void KernelUI::invokeModelFDMPreviewFunc(const char* func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* fdmPanel = m_appWindow->findChild<QObject*>("FDMPreviewPanel");

    if(variant1.isNull())
        QMetaObject::invokeMethod(fdmPanel, func);
    else if(variant2.isNull())
        QMetaObject::invokeMethod(fdmPanel, func, Q_ARG(QVariant, variant1));
    else
        QMetaObject::invokeMethod(fdmPanel, func, Q_ARG(QVariant, variant1), Q_ARG(QVariant, variant2));
}

void KernelUI::invokeMainWindowFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    invokeQmlObjectFunc(m_appWindow, func, variant1, variant2);
}

QJSValue KernelUI::invokeJS(const QString& str,QObject* jsContex)
{

    if(!m_engine->globalObject().hasProperty("contex"))
    {
        m_engine->globalObject().setProperty("contex", m_engine->newQObject(jsContex));
        QString wrap_js = "(function(key) { var value = contex.value(key); if(value.indexOf('eval') != -1){ value = eval(value) }; return value; })";
        QJSValue fun1 = m_engine->evaluate(wrap_js);
        wrap_js = "(function (){ var value_infill_pattern=contex.value('infill_pattern'); var temp=0; if(value_infill_pattern=='grid'||value_infill_pattern=='tetrahedral'||value_infill_pattern=='quarter_cubic'){temp=2} else if(value_infill_pattern=='triangles'||value_infill_pattern=='trihexagon'||value_infill_pattern=='cubic'||value_infill_pattern=='cubicsubdiv'){ temp=3 }else{ temp=1 } return temp})";
        QJSValue fun2 = m_engine->evaluate(wrap_js);
        m_engine->globalObject().setProperty("getValueByInfillPattern", fun2);
        m_engine->globalObject().setProperty("getEnumValue", fun1);
    }else{
        m_engine->globalObject().deleteProperty("contex");
        m_engine->globalObject().setProperty("contex", m_engine->newQObject(jsContex));
    }

    QJSValue ret = m_engine->evaluate(str);
    if (ret.isError())
    {
        qDebug() << "JavaSript Error:" << ret.toString();
    }
    else
    {
        //qDebug() << "JavaScript Succeed";
    }

    return ret;
}

void KernelUI::invokeRightClickMenuFunc(const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    QObject* object = m_appWindow->findChild<QObject*>("idRightClickMenu");
    invokeQmlObjectFunc(object, func, variant1, variant2);
}

QObject* KernelUI::getUI(QString uiname)
{
    uiname = uiname.toLower();
    if (uiname == "uiroot")
    {
        return this->root();
    }
    else if (uiname == "loginbtn")
    {
        QObject* pRoot = m_engine->rootObjects().first();
        QObject* logindBtn = pRoot->findChild<QObject*>("loginBtn");
        return logindBtn;
    }
    else if (uiname == "uiappwindow")
    {
        return this->appWindow();
    }
    else if (uiname == "uirightpanel" || uiname == "rightpanel")
    {
        return this->rightPanel();
    }
    else if (uiname == "footer")
    {
        return this->footer();
    }
    else if (uiname == "glmainview")
    {
        return this->glMainView();
    }
    else if (uiname == "topbar")
    {
        return this->topBar();
    }else if(uiname=="lefttoolbar")
    {
        return this->leftToolbar();
    }
    else
    {
        return nullptr;
    }
}

int KernelUI::addToolCommand(
        ToolCommand* command, qtuser_qml::ToolCommandGroupType group, int index) {
    qtuser_qml::ToolCommandCenter* center{ nullptr };

    switch (group) {
        case qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN:
            center = lCenter();
            break;
        case qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER:
            center = lOtherCenter();
            break;
        default:
            break;
    }

    if (nullptr == center) {
        return -1;
    }

    center->addCommand(command, index);
    return 0;
}

int KernelUI::addToolCommand(ToolCommand* command,
                             qtuser_qml::ToolCommandGroupType group,
                             qtuser_qml::ToolCommandType type) {
    return addToolCommand(command, group, static_cast<int>(type));
}

int KernelUI::removeToolCommand(ToolCommand* command, qtuser_qml::ToolCommandGroupType group) {
    qtuser_qml::ToolCommandCenter* center{ nullptr };

    switch (group) {
        case qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_MAIN:
            center = lCenter();
            break;
        case qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER:
            center = lOtherCenter();
            break;
        default:
            break;
    }

    if (nullptr == center) {
        return -1;
    }

    center->removeCommand(command);
    return 0;
}

int KernelUI::addActionCommand(ActionCommand* command, QString key)
{
    command->setParent(this);
    CCommandList::getInstance()->addActionCommad(command);
    return 1;
}

int KernelUI::removeActionlCommand(ActionCommand* command, QString key)
{
    CCommandList::getInstance()->removeActionCommand(command);
    return 1;
}

void KernelUI::setLeftToolbar(QObject* leftToolbar)
{
    m_leftToolbar = leftToolbar;
}

creative_kernel::Translator* KernelUI::translator()
{
    return m_translator;
}

QObject* KernelUI::leftToolbar()
{
    return m_leftToolbar;
}

QObject* KernelUI::root()
{
    return m_glMainView;
}

QObject* KernelUI::appWindow()
{
    return m_appWindow;
}

QObject* KernelUI::rightPanel()
{
    return m_rightPanel;
}

QObject* KernelUI::glMainView()
{
    return m_glMainView;
}

void KernelUI::setGLMainView(QObject* glMainView)
{
    m_glMainView = glMainView;
}

bool KernelUI::isFirstStart() const {
  QSettings setting;
  setting.beginGroup("profile_setting");
  return setting.value("first_start", "0").toString() == "0";
}

QObject* KernelUI::footer()
{
    return m_footer;
}

void KernelUI::setFooter(QObject* footer)
{
    m_footer = footer;
}

QObject* KernelUI::topBar()
{
    return m_topbar;
}

void KernelUI::setTopBar(QObject* topbar)
{
    m_topbar = topbar;
}

void KernelUI::setAppWindow(QObject* appWindow)
{
    m_appWindow = appWindow;
    m_appWindow->installEventFilter(this);

#ifdef __APPLE__
    QWindow* applicationWindiow = dynamic_cast<QWindow*> (appWindow);
    MacWindow macWindow;
    macWindow.setContentWindow(applicationWindiow);
    macWindow.setTitleVisibility(false);
    macWindow.setTitlebarAppearsTransparent(true);
    macWindow.show();
    MyCppObject* macAppObject = macWindow.getMacAppObject();
    connect(macAppObject, &MyCppObject::closeSignal, [=]
        {
            qDebug() << "signal KernelUI success ";
            // ?????????????
            emit closeWindow();

        });

#endif
}

void KernelUI::setRightPanel(QObject* rightPanel)
{
    m_rightPanel = rightPanel;
}

void KernelUI::visibleAll(bool visible)
{
    m_leftToolbar->setProperty("visible", visible);
}

qtuser_qml::ToolCommandCenter* KernelUI::lCenter() {
    return m_leftToolbarModelList;
}

qtuser_qml::ToolCommandCenter* KernelUI::lOtherCenter() {
    return m_leftOtherToolbarModelList;
}

QAbstractListModel* KernelUI::lMainModel() {
    return static_cast<QAbstractListModel*>(m_leftToolbarModelList);
}

QAbstractListModel* KernelUI::lOtherModel() {
    return static_cast<QAbstractListModel*>(m_leftOtherToolbarModelList);
}

QStringList KernelUI::getFontList() const {
    return font_list_;
}

void KernelUI::setFontList(const QStringList& font_list) {
    font_list_ = font_list;
    auto last = std::unique(font_list_.begin(), font_list_.end());
    font_list_.erase(last, font_list_.end());
    std::sort(font_list_.begin(), font_list_.end());
}

void KernelUI:: beforeCloseWindow()
{
    emit closeWindow();
}

void KernelUI::registerQmlEngine(QQmlApplicationEngine& engine)
{
    m_engine = &engine;

    CCommandsData* commandData = new CCommandsData(this);
    m_engine->rootContext()->setContextProperty("actionCommands", commandData);

    m_engine->rootContext()->setContextProperty("appDir", qApp->applicationDirPath());

    //wizard function
    WizardUI* wizardUI = new WizardUI();
    m_engine->rootContext()->setContextProperty("WizardUI", wizardUI);
    m_engine->rootContext()->setContextProperty("screenScaleFactor", wizardUI->getScreenScaleFactor());

    QString fontsPath;
#ifdef Q_OS_OSX
    fontsPath = QCoreApplication::applicationDirPath() + "/../Resources/resources/fonts/";
#else
    fontsPath = QCoreApplication::applicationDirPath() + "/resources/fonts/";
#endif

    m_engine->rootContext()->setContextProperty("fontsPath", fontsPath);

    m_engine->rootContext()->setContextProperty("kernel_ui", this);
    m_translator->setQmlEngine(m_engine);
}

QQmlApplicationEngine* KernelUI::getQmlEngine()
{
    return m_engine;
}

bool KernelUI::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_appWindow && QEvent::Type::KeyPress == event->type()) {
        auto* key_press_event = dynamic_cast<QKeyEvent*>(event);

        creative_kernel::Kernel* kernel = getKernel();
        int phase = kernel->currentPhase();
        if (key_press_event->key() == Qt::Key_Delete && phase == 0) {
            creative_kernel::removeSelectionModels(true);
        }

        if (key_press_event->modifiers() == Qt::ControlModifier &&
            key_press_event->key() == Qt::Key_A && phase == 0) {
            selectAll();
        }

        if (key_press_event->modifiers() == Qt::ControlModifier &&
                key_press_event->key() == Qt::Key_Z) {
            qDebug() << "Ctrl+Z";
            cxkernel::undo();
        }

        if (key_press_event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) &&
                key_press_event->key() == Qt::Key_Z) {
            qDebug() << "Ctrl+Shift+Z";
            cxkernel::redo();
        }

        if (key_press_event->modifiers() == Qt::ControlModifier &&
            key_press_event->key() == Qt::Key_C) {
            qDebug() << "Ctrl+C";
            return onCopy();
            //creative_kernel::undo();

        }

        if (key_press_event->modifiers() == Qt::ControlModifier &&
            key_press_event->key() == Qt::Key_V) {
            qDebug() << "Ctrl+V";
            return onPaste();
        }
    }
    return false;
}

void KernelUI::onRightMouseButtonPress(QMouseEvent* event)
{
    (void)(event);
}

void KernelUI::onRightMouseButtonMove(QMouseEvent* event)
{
    (void)(event);
}

void KernelUI::onRightMouseButtonRelease(QMouseEvent* event)
{
    (void)(event);
}

void KernelUI::onRightMouseButtonClick(QMouseEvent* event)
{
    Q_UNUSED(event);
    creative_kernel::Kernel* kernel = getKernel();
    int phase = kernel->currentPhase();
    if(phase==0)
    {
        invokeRightClickMenuFunc("requstShow");
    }
}

void KernelUI::onThemeChanged(creative_kernel::ThemeCategory category)
{
    QMetaObject::invokeMethod(m_appWindow, "changeThemeColor", Q_ARG(QVariant, QVariant::fromValue((int)category)));

    m_leftToolbarModelList->refreshModel();
    m_leftOtherToolbarModelList->refreshModel();
}

void KernelUI::onLanguageChanged(creative_kernel::MultiLanguage language)
{
}

void KernelUI::invokeQmlObjectFunc(QObject* object, const QString& func, const QVariant& variant1, const QVariant& variant2)
{
    if (!object)
        return;

    if (variant1.isNull())
        QMetaObject::invokeMethod(object, func.toStdString().c_str());
    else if (variant2.isNull())
        QMetaObject::invokeMethod(object, func.toStdString().c_str(), Q_ARG(QVariant, variant1));
    else
        QMetaObject::invokeMethod(object, func.toStdString().c_str(), Q_ARG(QVariant, variant1), Q_ARG(QVariant, variant2));
}
