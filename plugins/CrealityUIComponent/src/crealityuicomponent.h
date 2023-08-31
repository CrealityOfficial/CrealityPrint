#ifndef CREALITYUI_H
#define CREALITYUI_H

#include <QQuickItem>

class CrealityUIComponent : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(CrealityUIComponent)

public:
    CrealityUIComponent(QQuickItem *parent = nullptr);
    ~CrealityUIComponent();
};

#endif // CREALITYUI_H
