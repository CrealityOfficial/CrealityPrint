#ifndef SUPPORTCOMMAND_H
#define SUPPORTCOMMAND_H
#include "qtuserqml/plugin/toolcommand.h"

#include <QObject>
#include <QVector3D>

class SimpleSupportOp;
class SupportCommand : public ToolCommand
{
    Q_OBJECT
public:
    explicit SupportCommand(QObject *parent = nullptr);
    Q_INVOKABLE void execute();
    Q_INVOKABLE void addMode();
    Q_INVOKABLE void deleteMode();
    Q_INVOKABLE void removeAll();
    Q_INVOKABLE void autoAddSupport(qreal size,qreal angle,bool platform);
    Q_INVOKABLE bool checkSelect() override;
    Q_INVOKABLE void changeSize(qreal size);
signals:

public slots:
private:
    SimpleSupportOp *m_pOp;
};

#endif // SUPPORTCOMMAND_H
