#include "mycppobject.h"
#include <QDebug>
MyCppObject::MyCppObject(QObject *parent) : QObject(parent)
{

}

void MyCppObject::ExampleMethod(const std::string&str)
{
    qDebug() << "12315667 =" << str.c_str();

}

void MyCppObject::closeFunction()
{
    emit closeSignal();
    qDebug() << "closeFunction";
}
