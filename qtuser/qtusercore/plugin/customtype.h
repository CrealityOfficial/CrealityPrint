#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H

#include <QObject>

class CustomType : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int indentation READ indentation WRITE setIndentation NOTIFY indentationChanged)

public:
    explicit CustomType(QObject *parent = 0);
    CustomType(const CustomType &other);
    ~CustomType();

    QString text();
    void setText(QString text);

    int indentation();
    void setIndentation(int indentation);

signals:
    void textChanged();
    void indentationChanged();

private:
    QString myText;
    int myIndentation;
};

Q_DECLARE_METATYPE(CustomType)

#endif // CUSTOMTYPE_H
