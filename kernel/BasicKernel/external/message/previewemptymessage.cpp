#include "previewemptymessage.h"

namespace creative_kernel
{
    PreviewEmptyMessage::PreviewEmptyMessage(QObject* parent) : MessageObject(parent)
    {
   
    }

	PreviewEmptyMessage::~PreviewEmptyMessage()
    {

    }

    int PreviewEmptyMessage::codeImpl()
    {
        return MessageType::PreviewEmpty;
    }

    int PreviewEmptyMessage::levelImpl()
    {
        return MessageLevel::Warning;
    }

    QString PreviewEmptyMessage::messageImpl()
    {
        return tr("There is no model object in the current printing plate, please add at least one model before performing the slice preview operation.");
    }

    QString PreviewEmptyMessage::linkStringImpl()
    {
        return "";
    }

    void PreviewEmptyMessage::handleImpl()
    {

    }

}