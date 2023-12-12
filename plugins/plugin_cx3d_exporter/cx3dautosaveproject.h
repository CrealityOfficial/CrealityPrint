#ifndef CX3DAUTOSAVEPROJECT_H
#define CX3DAUTOSAVEPROJECT_H
#include "data/interface.h"
#include <QtCore/QObject>
#include <QtCore/QTimer>

class  Cx3dAutoSaveProject : public QObject,
    public creative_kernel::SpaceTracer
{
    Q_OBJECT
public:
    Cx3dAutoSaveProject(QObject* parent = nullptr);
    virtual ~Cx3dAutoSaveProject();

    void startAutoSave();

    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();
protected:
    void onBoxChanged(const qtuser_3d::Box3D& box) override {}
    void onSceneChanged(const qtuser_3d::Box3D& box) override {}

    void onModelAdded(creative_kernel::ModelN* model) override;
    void onModelRemoved(creative_kernel::ModelN* model) override;
public slots:
    void updateTmpTime();

private:
    QTimer* tmp_timer;
};

#endif // CX3DAUTOSAVEPROJECT_H
