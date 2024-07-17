#ifndef CX3DAUTOSAVEPROJECT_H
#define CX3DAUTOSAVEPROJECT_H
#include "data/interface.h"
#include <QtCore/QObject>
#include <QtCore/QTimer>

class  Cx3dAutoSaveProject : public QObject
{
    Q_OBJECT
public:
    Cx3dAutoSaveProject(QObject* parent = nullptr);
    virtual ~Cx3dAutoSaveProject();

    Q_INVOKABLE void startAutoSave();

    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();
    Q_INVOKABLE void triggerAutoSave();
public slots:
    void updateTmpTime();

private:
    QTimer* tmp_timer;
};

#endif // CX3DAUTOSAVEPROJECT_H
