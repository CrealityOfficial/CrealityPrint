#ifndef MYCPPOBJECT_H
#define MYCPPOBJECT_H

#include <QObject>
//这个对象用于macwindow.mm 中ob-c 传一个回调给其他cpp对象
class MyCppObject : public QObject
{
    Q_OBJECT
public:
    explicit MyCppObject(QObject *parent = nullptr);
    void ExampleMethod(const std::string&str);
    void closeFunction();
signals:
    void closeSignal();
public slots:
};

#endif // MYCPPOBJECT_H
