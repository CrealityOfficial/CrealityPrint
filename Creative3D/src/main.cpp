#include "cxkernel/kernel/entry.h"
#include "kernel/kernel.h"

#include <QtWidgets/QAction>
#include <QtCore/QStandardPaths>
#include <QtWebSockets/QWebSocket>
#include <QtQuick/QQuickItem>

//#include <windows.h>

void dumpFunc()
{
    QAction action;
    QWebSocket socket;
    QQuickItem item;
}

int main(int argc, char* argv[])
{
    //HINSTANCE p = LoadLibrary("C:\\Program Files\\RenderDoc\\renderdoc.dll");
    //if (p) {
    //    qDebug() << "load success";
    //}
    //else {
    //    qDebug() << "load fail";
    //}

    dumpFunc();
    cxkernel::AppModuleCreateFunc func = creative_kernel::createC3DContext;
    return cxkernel::appMain(argc, argv, func);
}