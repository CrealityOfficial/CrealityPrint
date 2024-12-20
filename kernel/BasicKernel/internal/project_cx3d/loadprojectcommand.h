#ifndef LOAD_PROJECT_CX3DRECNETPROJECTCOMMAND_H
#define LOAD_PROJECT_CX3DRECNETPROJECTCOMMAND_H
#include "menu/actioncommand.h"
#include "interface/projectinterface.h"

class LoadProjectCommand : public ActionCommand
    , public creative_kernel::ProjectOpenCallback
{
public:
    LoadProjectCommand(QObject* parent = nullptr);
    virtual ~LoadProjectCommand();

protected:

    void openProject(const QString& fileName);
    
    QString saveAsDialogPath();
protected:
    QString m_willOpenProject;
};

#endif // LOAD_PROJECT_CX3DRECNETPROJECTCOMMAND_H
