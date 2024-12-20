#include "customtype.h"

CustomType::CustomType(QObject *parent) : QObject(parent), myIndentation(0)
{
}

CustomType::CustomType(const CustomType &other)
{
    myText = other.myText;
    myIndentation = other.myIndentation;
}

CustomType::~CustomType()
{
}

QString CustomType::text()
{
    return myText;
}

void CustomType::setText(QString text)
{
    myText = text;
    emit textChanged();
}

int CustomType::indentation()
{
    return myIndentation;
}

void CustomType::setIndentation(int indentation)
{
    myIndentation = indentation;
    emit indentationChanged();
}
