#ifndef OPENRECENTFILECmd_H
#define OPENRECENTFILECmd_H
#include "menu/actioncommand.h"

namespace creative_kernel
{
    class OpenRecentFileCmd : public ActionCommand
    {
        Q_OBJECT
    public:
        OpenRecentFileCmd();
        virtual ~OpenRecentFileCmd();

        Q_INVOKABLE void execute();
    };
}

#endif // OPENRECENTFILECmd_H
