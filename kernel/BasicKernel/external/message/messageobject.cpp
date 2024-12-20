#include "messageobject.h"
#include <QDebug>

namespace creative_kernel
{
    MessageObject::MessageObject(QObject* parent) : QObject(parent)
    {

    }

    int MessageObject::code()
    {
        return codeImpl();
    }

    int MessageObject::level()
    {
        return levelImpl();
    }

    QString MessageObject::message()
    {
        return messageImpl();
    }

    QString MessageObject::linkString()
    {
        return linkStringImpl();
    }

    void MessageObject::handle()
    {
        handleImpl();
    }
    
}