#include "gcodepathoutofprinterscopemessage.h"

namespace creative_kernel
{
    GCodePathOutOfPrinterScopeMessage::GCodePathOutOfPrinterScopeMessage(QObject* parent) 
        : MessageObject(parent)
    {
    }

	GCodePathOutOfPrinterScopeMessage::~GCodePathOutOfPrinterScopeMessage()
    {

    }

    int GCodePathOutOfPrinterScopeMessage::codeImpl()
    {
        return MessageType::GCodePathOutofPrinterScope;
    }

    int GCodePathOutOfPrinterScopeMessage::levelImpl()
    {
        return int(MessageLevel::Error);
    }

    QString GCodePathOutOfPrinterScopeMessage::messageImpl()
    {
        return tr("A G-code path goes beyond plate boundaries.");
    }

    QString GCodePathOutOfPrinterScopeMessage::linkStringImpl()
    {
        return QString();
    }

    void GCodePathOutOfPrinterScopeMessage::handleImpl()
    {
    }

}