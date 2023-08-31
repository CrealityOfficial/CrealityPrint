#include "crealityui_plugin.h"
#include "crealityui.h"

#include "DockController.h"

#include <qqml.h>
#include <QDebug>
void CrealityUIPlugin::registerTypes(const char *uri)
{
    // @uri com.creality.qmlcomponents
    Q_INIT_RESOURCE(qml);

    qmlRegisterType<CrealityUI>(uri, 1, 0, "CrealityUI");

    DockController::RegisterQmlComponent();
}

