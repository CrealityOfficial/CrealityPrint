#include "kernel/kernelui.h"
#include "qtusercore/plugin/toolcommandcenter.h"
#include "qtusercore/plugin/toolcommand.h"
#include "qtusercore/module/systemutil.h"
#include "internal/menu/ccommandsdata.h"
#include "qtuser3d/event/eventsubdivide.h"
#include "qtuser3d/module/glcompatibility.h"
#include "cxkernel/utils/glcompatibility.h"

#include <cxcloud/service_center.h>

#include "interface/cloudinterface.h"
#include "interface/eventinterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"

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
#include <QQuickView>
#include <QtCore/QDebug>
#include <QQuickItem>
#include <QColorDialog>

#include <qtusercore/plugin/toolcommandcenter.h>
#include <qtusercore/plugin/toolcommand.h>
#include <qtusercore/module/systemutil.h>
#include <qtusercore/util/settings.h>

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
    , m_closeHook(nullptr)
    , m_layoutCommand(nullptr)
{
    // model list
    m_leftToolbarModelList = new qtuser_qml::ToolCommandCenter(this);
    m_leftOtherToolbarModelList = new qtuser_qml::ToolCommandCenter(this);
    m_topToolbarModelList = new qtuser_qml::ToolCommandCenter(this);

    m_translator = new Translator(this);

}

KernelUI::~KernelUI()
{
}

void KernelUI::initialize()
{
    addUIVisualTracer(this,this);
    ProjectInfoUI::instance()->setParent(this);
    addRightMouseEventHandler(this);
    addSelectTracer(this);
    addKeyEventHandler(this);

    {
        using group_t = qtuser_qml::ToolCommandGroupType;
        using command_t = qtuser_qml::ToolCommandType;


        //addToolCommand(new PickMode         , group_t::LEFT_TOOLBAR_MAIN , command_t::PICK     );
        addToolCommand(new TranslateMode    , group_t::LEFT_TOOLBAR_MAIN , command_t::TRANSLATE);
        addToolCommand(new ScaleMode        , group_t::LEFT_TOOLBAR_OTHER , command_t::SCALE    );
        addToolCommand(new RotateMode       , group_t::LEFT_TOOLBAR_MAIN , command_t::ROTATE   );

        m_layoutCommand = new LayoutToolCommand;
        addToolCommand(m_layoutCommand, group_t::LEFT_TOOLBAR_MAIN , command_t::LAYOUT   );

        // addToolCommand(new SupportCommand   , group_t::LEFT_TOOLBAR_MAIN , command_t::SUPPORT  );

        addToolCommand(new CloneCommand     , group_t::LEFT_TOOLBAR_OTHER, command_t::CLONE    );
        addToolCommand(new MirrorCommand    , group_t::LEFT_TOOLBAR_OTHER, command_t::MIRROR   );
    }

    visibleAll(false);//

	//check GL version
	if (qtuser_3d::isGles())
	{
		qDebug() << QString("Creality Print only partially supports OpenGL ES.");
		emit sigUseOpenGLES();
	}
	else if (!cxkernel::isOpenGLVaild())
	{
		qDebug() << QString("OpenGL Driver is old.");
		emit sigOpenglOld();
	}

	// BASIC_KERNEL_API void setContinousRender();
	// BASIC_KERNEL_API void setCommandRender();
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
    qtuser_core::VersionSettings setting;
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
    //assert(m_topbar);
    m_translator->loadUserSettings();

    creative_kernel::sensorAnlyticsTrace("Login", "start");
    qtuser_core::VersionSettings setting;
    setting.beginGroup("profile_setting");
    QString strStartType = setting.value("first_start", "0").toString();

    //QObject* pMachineCommbox = m_topbar->findChild<QObject*>("AddPrinterDlg");
    //QQmlProperty::write(pMachineCommbox, "isFristStart", QVariant::fromValue(strStartType));
    setting.endGroup();
    if (strStartType == "0")
    {
        qInfo() << "startWizardComp first_start";
        QMetaObject::invokeMethod(m_glMainView, "requestFirstConfig");
        setting.beginGroup("cloud_service");
        setting.setValue("sync_official_param","1");
        setting.endGroup();
        
    }
    
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

void KernelUI::switchMode(int id)
{
    QObject* toolBar = leftToolbar();
    if (toolBar)
        QMetaObject::invokeMethod(toolBar, "switchModeById", Q_ARG(QVariant, QVariant::fromValue(id)));
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
     QList<ModelN*> models = modelns();
     QList<ModelN*> copyModels;
     Q_FOREACH(ModelN * m, m_copyModels)
     {
         if (models.contains(m))
         {
             copyModels.append(m);
         }
     }
     creative_kernel::cmdClone(copyModels);
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
    emit sigShowRightMenu();
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
        QObject* pRoot = (QObject *) (qobject_cast<QQuickView*>(m_engine->parent())->rootObject());
        //QObject* pRoot = (QQuickView*)(m_engine->parent())->rootObject();
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
        case qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW:
            center = topBarCenter();
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
        case qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW:
            center = topBarCenter();
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

QObject* KernelUI::messageStack()
{
    return m_messageStack;
}

void KernelUI::setMessageStack(QObject* messageStack) {
    m_messageStack = messageStack;
    if (!m_messageStack) {
        return;
    }

    while (!message_queue_.empty()) {
        auto* message = message_queue_.front();
        QQmlEngine::setObjectOwnership(message, QQmlEngine::ObjectOwnership::JavaScriptOwnership);
        QMetaObject::invokeMethod(m_messageStack, "requestMessage", Q_ARG(QVariant, QVariant::fromValue(message)));
        message_queue_.pop();
    }
}
float getScreenScaleFactor()
{
#ifdef __APPLE__
    QFont font = qApp->font();
    font.setPointSize(12);
    qApp->setFont(font);
    return 1;
#else
    QFont font = qApp->font();
    font.setPointSize(10);
    qApp->setFont(font);
    //int ass = QFontMetrics(font).ascent();
    float fontPixelRatio = QFontMetrics(qApp->font()).ascent() / 11.0;
    fontPixelRatio = int(fontPixelRatio * 4) / 4.0;
    return fontPixelRatio;
#endif
}

#ifdef Q_OS_WINDOWS
UINT_PTR ChooseColorProc(HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) {

    RECT rc;
    long hwndctl;
    long scrWidth;
    long scrHeight;
    long dlgWidth;
    long dlgHeight;
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CHOOSECOLOR* data = (CHOOSECOLOR*)(lParam);
            QQuickView* qv = (QQuickView*)(data->lCustData);
            long mainwindowX = qv->x();
            long mainwindowY = qv->y();
            scrWidth = qv->width();
            scrHeight = qv->height();
            GetWindowRect(hwnd, &rc);

            dlgWidth = rc.right - rc.left;
            dlgHeight = rc.bottom - rc.top;

            long x = mainwindowX + scrWidth - dlgWidth - 400* getScreenScaleFactor() + 8;
            long y = mainwindowY + 30* getScreenScaleFactor() + 8;
            MoveWindow(hwnd, x,
                y, dlgWidth, dlgHeight, 1);
        }
            break;
    default:
        break;
    }
    return 0;
}
#endif
QColor KernelUI::showColorDialog(QColor oldcolor, QObject* parent)
{
    QColor color= oldcolor;
#ifdef Q_OS_WINDOWS
    CHOOSECOLOR cc = {0}; // 颜色选择对话框结构 
    static COLORREF acrCustClr[16] = {0xffffff,0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff}; // 16 个自定义颜色
    //memset(acrCustClr, 0xFFFFFFFF, sizeof(COLORREF) * 16);
    cc.lStructSize = sizeof(cc);
    QQuickView* qv = qobject_cast<QQuickView*>(parent);
    if (!qv)
        return QColor();
    WId winId = qobject_cast<QQuickView*>(parent)->winId();
    HWND hwnd = (HWND)winId;
    cc.hwndOwner = hwnd; // 对话框窗口的父窗口
    cc.lpCustColors = (LPDWORD) acrCustClr; // 传入自定义颜色
    cc.rgbResult = RGB(oldcolor.red(), oldcolor.green(), oldcolor.blue()); // 初始颜色
    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK; // 显示“规定自定义颜色”的内容 | 用 rgbResult 的颜色值初始化
    cc.lpfnHook = &ChooseColorProc;
    cc.lCustData = (LPARAM)parent;


    if (ChooseColor(&cc)) // 打印输出选择的颜色
    {
        color.setRed(GetRValue(cc.rgbResult));
        color.setGreen(GetGValue(cc.rgbResult));
        color.setBlue(GetBValue(cc.rgbResult));
        //wsprintf(s, L"R：%d，G：%d，B：%d\n", GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
        //WriteConsole(out, s, wcslen(s), NULL, NULL);
    }

#else

    color = QColorDialog::getColor(oldcolor,nullptr,"Please choose a color",QColorDialog::DontUseNativeDialog);

#endif
    return color;
}
void KernelUI::requestMessage(QObject* message) {
    if (!message) {
        return;
    }

    if (m_messageStack) {
        QQmlEngine::setObjectOwnership(message, QQmlEngine::ObjectOwnership::JavaScriptOwnership);
        QMetaObject::invokeMethod(m_messageStack, "requestMessage", Q_ARG(QVariant, QVariant::fromValue(message)));
    } else {
        message_queue_.emplace(message);
    }
}

void KernelUI::destroyMessage(int code)
{
    if (m_messageStack)
    {
        QMetaObject::invokeMethod(m_messageStack, "destroyMessage", Q_ARG(QVariant, QVariant::fromValue(code)));
    }
}

bool KernelUI::isFirstStart() const {
  qtuser_core::VersionSettings setting;
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
#if 1
#ifdef __APPLE__
    QObject *frameLessView =m_engine->rootContext()->contextProperty("frameLessView").value<QObject*>();
    QWindow* applicationWindiow = dynamic_cast<QWindow*> (frameLessView);
    MacWindow macWindow;
    
    macWindow.setContentWindow(applicationWindiow);
    macWindow.setTitleVisibility(false);
    macWindow.setTitlebarAppearsTransparent(true);
    macWindow.setFullSizeContentView(true);
    macWindow.show();
    MyCppObject* macAppObject = macWindow.getMacAppObject();
    connect(macAppObject, &MyCppObject::closeSignal, [=]
        {
            qDebug() << "signal KernelUI success ";
            applicationWindiow->close();
            // ?????????????
            //this->requestCommonDialog(creative_kernel::gContext->project, "CloseProFileDlg");

        });
#endif
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
qtuser_qml::ToolCommandCenter* KernelUI::topBarCenter() {
    return m_topToolbarModelList;
}
QAbstractListModel* KernelUI::lMainModel() {
    return static_cast<QAbstractListModel*>(m_leftToolbarModelList);
}

QAbstractListModel* KernelUI::lOtherModel() {
    return static_cast<QAbstractListModel*>(m_leftOtherToolbarModelList);
}
QAbstractListModel* KernelUI::topBarModel()
{
    return static_cast<QAbstractListModel*>(m_topToolbarModelList);
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

void KernelUI::setCloseHook(creative_kernel::CloseHook* hook)
{
    m_closeHook = hook;
}

void KernelUI::beforeCloseWindow()
{
    if (m_closeHook)
    {
        m_closeHook->onWindowClose();
        return;
    }

    requestQmlCloseAction();
}

void KernelUI::registerQmlEngine(QQmlEngine& engine)
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

    registerImageProvider(QStringLiteral("login_qrcode_image_provider"),
        creative_kernel::CloudAccountService().lock()->getQrcodeImageProvider());
}

QQmlEngine* KernelUI::getQmlEngine()
{
    return m_engine;
}


void KernelUI::setCommandIndex(int index)
{
    if (index == m_commandIndex)
        return;

    m_commandIndex = index;
    m_otherCommandIndex = -1;
    emit commandChanged();

    if (m_commandIndex == -1)
        setVisOperationMode(NULL);
}

int KernelUI::commandIndex()
{
    return m_commandIndex;
}

void KernelUI::setOtherCommandIndex(int index)
{
    if (index == m_otherCommandIndex)
        return;

    m_otherCommandIndex = index;
    m_commandIndex = -1;
    emit commandChanged();

    if (m_otherCommandIndex == -1)
        setVisOperationMode(NULL);
}

int KernelUI::otherCommandIndex()
{
    return m_otherCommandIndex;
}

void KernelUI::showToolTip(const QPoint& pos, const QString& text)
{
    QMetaObject::invokeMethod(appWindow(), "showToolTip", Q_ARG(QVariant, QVariant::fromValue(pos)),
                                                        Q_ARG(QVariant, QVariant::fromValue(text)));
}

void KernelUI::hideToolTip()
{
    QMetaObject::invokeMethod(appWindow(), "hideToolTip");
}

void KernelUI::requestPrinterSettingsDialog(QObject* printer)
{
    QMetaObject::invokeMethod(appWindow(), "requestPrinterSettingsDialog", Q_ARG(QVariant, QVariant::fromValue(printer)));
}

void KernelUI::requestPrinterNameEditorDialog(QObject* printer)
{
    QMetaObject::invokeMethod(appWindow(), "requestPrinterNameEditorDialog", Q_ARG(QVariant, QVariant::fromValue(printer)));
}

void KernelUI::setAutoResetOperateMode(bool enabled)
{
    m_autoResetOperateMode = enabled;
}

void KernelUI::setModelCenter()
{
  qtuser_3d::Box3D modelsbox;
	auto models = creative_kernel::selectionms();
	for (auto m : models)
	{
		qtuser_3d::Box3D modelbox = m->globalSpaceBox();
		modelsbox += modelbox;
	}
	QVector3D movePos;
	float newZ = 0;
	if (modelsbox.valid)
	{
		qtuser_3d::Box3D box = creative_kernel::baseBoundingBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		newZ = center.z() - size.z() / 2.0f;

		movePos = box.center() - modelsbox.center();
		movePos.setZ(0);
	}

	for (auto m : models)
	{
		QVector3D oldLocalPosition = m->localPosition();
		QVector3D newPositon = oldLocalPosition + movePos;
		//newPositon.setZ(newZ);

		moveModel(m, oldLocalPosition, newPositon, true);
	}

}

void KernelUI::setModelBottom()
{
    auto models = creative_kernel::selectionms();
	for (auto m : models)
    {
        qtuser_3d::Box3D box = m->globalSpaceBox();

		float fOffset = -box.min.z();

        QVector3D zoffset = QVector3D(0.0f, 0.0f, fOffset);
        QVector3D localPosition = m->localPosition();

        moveModel(m, localPosition, localPosition + zoffset, true);
    }
}

LayoutToolCommand* KernelUI::getLayoutCommand()
{
    Q_ASSERT(m_layoutCommand);
    return m_layoutCommand;
}

void KernelUI::setScene3DCursor(const QCursor& cursor)
{
    QQuickItem* item = dynamic_cast<QQuickItem*>(m_glMainView);
    if (item)
        item->setCursor(cursor);
}

QCursor KernelUI::getScene3DCursor()
{
    QCursor cursor;
    QQuickItem* item = dynamic_cast<QQuickItem*>(m_glMainView);
    if (item)
        cursor = item->cursor();

    return cursor;
}

void KernelUI::onKeyPress(QKeyEvent* event)
{
    creative_kernel::Kernel* kernel = getKernel();
    int phase = kernel->currentPhase();

    if (event->modifiers() == Qt::ControlModifier &&
        event->key() == Qt::Key_Z &&
        phase == 0) {
        qDebug() << "Ctrl+Z";
        cxkernel::undo();
    }
    if (event->modifiers() == (Qt::ControlModifier) &&
        event->key() == Qt::Key_Y &&
        phase == 0) {
        qDebug() << "Ctrl+Y";
        cxkernel::redo();
    }

    if(!wipeTowerSelected()&&getKernel()->isDefaultVisScene()){
        if (event->modifiers() == (Qt::ControlModifier) &&
            event->key() == Qt::Key_C &&
            phase == 0) {
            qDebug() << "Ctrl+C";
            onCopy();
        }
        if (event->modifiers() == (Qt::ControlModifier) &&
            event->key() == Qt::Key_V &&
            phase == 0) {
            qDebug() << "Ctrl+V";
            onPaste();
        }
    }

    if (event->modifiers() == (Qt::AltModifier) &&
        event->key() == Qt::Key_S &&
        phase == 0) {
        qDebug() << "Alt+S";
    }
    if (event->modifiers() == (Qt::AltModifier) &&
        event->key() == Qt::Key_M &&
        phase == 0) {
        QMetaObject::invokeMethod(m_topbar, "excuteOpt", Q_ARG(QVariant, QVariant::fromValue(QString("Models"))));
        qDebug() << "Alt+M";
    }
    if (event->modifiers() == (Qt::AltModifier) &&
        event->key() == Qt::Key_F4 &&
        phase == 0) {
        qDebug() << "Alt+F4";
    }
    if (event->modifiers() == (Qt::AltModifier) &&
        event->key() == Qt::Key_C &&
        phase == 0) {
        qDebug() << "Alt+C";
    }


}

void KernelUI::onKeyRelease(QKeyEvent* event)
{

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

    if(phase==0 && kernel->isDefaultVisScene()) invokeRightClickMenuFunc("requstShow");
    if(phase == 1) Q_EMIT rightMouseClick();


}

void KernelUI::onThemeChanged(creative_kernel::ThemeCategory category)
{
    QMetaObject::invokeMethod(m_appWindow, "changeThemeColor", Q_ARG(QVariant, QVariant::fromValue((int)category)));

    m_leftToolbarModelList->refreshModel();
    m_leftOtherToolbarModelList->refreshModel();
    m_topToolbarModelList->refreshModel();
}

void KernelUI::onLanguageChanged(creative_kernel::MultiLanguage language)
{
    Q_EMIT currentLanguageChanged();
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

void KernelUI::onSelectionsChanged()
{
    auto mode = visOperationMode();
    if (m_autoResetOperateMode && selectionms().isEmpty())
    {
        if (mode)
        {
            if (mode->type() == qtuser_3d::SceneOperateMode::FixedMode)
            {
                return;
            }
            else if (mode->type() == qtuser_3d::SceneOperateMode::ReusableMode)
            {
                if (m_leftToolbar)
                    QMetaObject::invokeMethod(m_leftToolbar, "setOperatePanelVisible", Q_ARG(QVariant, false));
                return;
            }
        }
        if (commandIndex() != -1)
            setCommandIndex(-1);
        else if (otherCommandIndex() != -1)
            setOtherCommandIndex(-1);
    }
    else
    {
        if (mode)
        {
            if (mode->type() == qtuser_3d::SceneOperateMode::ReusableMode)
            {
                if (m_leftToolbar)
                    QMetaObject::invokeMethod(m_leftToolbar, "setOperatePanelVisible", Q_ARG(QVariant, true));
                return;
            }
        }
    }
}

void KernelUI::selectChanged(qtuser_3d::Pickable* pickable)
{

}
