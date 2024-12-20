#include "scence3dapp.h"
#include <QtCore/QCoreApplication>

#include "qtuserqml/macro.h"
#include "qtuserqml/gl/glquickitem.h"
#include "qtuserqml/plugin/toolcommand.h"

#include "kernel/kernelui.h"
#include "kernel/kernel.h"
#include "cxffmpeg/qmlplayer.h"
#include "qcxchart.h"

#define CXSW_SCOPE "com.cxsw.SceneEditor3D"
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define COMMON_REASON "Created by Cpp"

#define CXSW_REG(x) QML_INTERFACE(x, CXSW_SCOPE, VERSION_MAJOR, VERSION_MINOR)
#define CXSW_REG_U(x) QML_INTERFACE_U(x, CXSW_SCOPE, VERSION_MAJOR, VERSION_MINOR, COMMON_REASON)

void scene3dapp_startEngine(QQmlApplicationEngine& engine)
{
    qDebug() << "scene3dapp_startEngine.";
    CXSW_REG(GLQuickItem);
    CXSW_REG(QMLPlayer);
    CXSW_REG(QCxChart);

    CXSW_REG_U(ToolCommand);
    CXSW_REG_U(ActionCommand);

    creative_kernel::Kernel* kernel = getKernel();
    kernel->registerQmlEngine(engine);

    //bind wizard is in mainWindow.qml
    engine.load(QUrl(QLatin1String("qrc:/scence3d/res/main.qml")));
}


