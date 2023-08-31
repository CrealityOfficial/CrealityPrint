#ifndef CREALITYUI_H
#define CREALITYUI_H

#include <QQuickItem>

class CrealityUI : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(CrealityUI)

public:
    CrealityUI(QQuickItem *parent = nullptr);
    ~CrealityUI();
};

#endif // CREALITYUI_H
