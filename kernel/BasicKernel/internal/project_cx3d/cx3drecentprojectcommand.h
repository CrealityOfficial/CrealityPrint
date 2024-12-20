#ifndef CX3DRECNETPROJECTCOMMAND_H
#define CX3DRECNETPROJECTCOMMAND_H
#include "loadprojectcommand.h"

class Cx3dRecentProjectCommand : public LoadProjectCommand
{
    Q_OBJECT
public:
    Cx3dRecentProjectCommand(QObject *parent);
    virtual ~Cx3dRecentProjectCommand();

    Q_INVOKABLE void execute();
protected:
    void accept() override;
    void cancel() override;
};

#endif // CX3DRECNETPROJECTCOMMAND_H
