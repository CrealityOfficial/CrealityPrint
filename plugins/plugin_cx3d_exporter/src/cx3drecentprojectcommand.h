#ifndef CX3DRECNETPROJECTCOMMAND_H
#define CX3DRECNETPROJECTCOMMAND_H

#include <QObject>
#include "menu/actioncommand.h"
namespace creative_kernel
{
    class ProjectInfoUI;
}

class Cx3dRecentProjectCommand: public ActionCommand
{
    Q_OBJECT
public:
    Cx3dRecentProjectCommand(QObject *parent);
    ~Cx3dRecentProjectCommand();
    Q_INVOKABLE void execute();
private:
    QString m_strFilePath;
    creative_kernel::ProjectInfoUI* m_projectUI;
public slots:
    void slotSaveLastPro();
    void slotCancelSaveLastPro();
};

#endif // CX3DRECNETPROJECTCOMMAND_H
