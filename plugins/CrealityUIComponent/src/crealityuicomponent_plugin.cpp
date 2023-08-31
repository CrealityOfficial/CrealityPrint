#include "crealityuicomponent_plugin.h"
#include "crealityuicomponent.h"

//#include "DockController.h"

#include <qqml.h>
#include <QDebug>
void CrealityUIPlugin::registerTypes(const char *uri)
{
    // @uri com.creality.qmlcomponents
    Q_INIT_RESOURCE(qml);

    qmlRegisterType<CrealityUIComponent>(uri, 1, 0, "CrealityUIComponent");

//    DockController::RegisterQmlComponent();
}

