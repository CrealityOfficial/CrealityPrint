#ifndef BASICTREECOMMAND_H
#define BASICTREECOMMAND_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QObject>
class BasicTreeModel;
class  QTUSER_CORE_API BasicTreeCommand : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(int level READ level)
    Q_PROPERTY(BasicTreeModel subNodel READ subNode)

public:
    explicit BasicTreeCommand();
    virtual ~BasicTreeCommand();

    QString name() const;
    int level() const;
    BasicTreeModel *subNode() const;

    void setName(const QString& name);
    void setLevel(const int& level);
    void setSuNode(BasicTreeModel *subnode);
    Q_INVOKABLE void execute();

private:
    int m_level;            //The level of the node is automatically determined by this attribute,default = 0; do not need update,auto update
protected:
    QString m_name;
    BasicTreeModel  *m_subModel;
};
#endif // BASICTREECOMMAND_H
