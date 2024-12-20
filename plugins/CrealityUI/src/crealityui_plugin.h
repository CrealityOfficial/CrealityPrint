#ifndef CREALITYUI_PLUGIN_H
#define CREALITYUI_PLUGIN_H

#include <QQmlExtensionPlugin>

class CrealityUIPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // CREALITYUI_PLUGIN_H
