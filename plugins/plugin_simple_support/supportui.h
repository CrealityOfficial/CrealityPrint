#ifndef SUPPORTUI_H
#define SUPPORTUI_H
#include <QObject>
#include <QVector3D>
#include "qtuser3d/module/pickableselecttracer.h"

class SimpleSupportOp;
class SupportUI : public QObject //, public qtuser_3d::SelectorTracer
{
    Q_OBJECT
public:
    explicit SupportUI(QObject *parent = nullptr);
    ~SupportUI();
    Q_INVOKABLE void addMode();
    Q_INVOKABLE void deleteMode();
    Q_INVOKABLE void removeAll();
    Q_INVOKABLE void autoAddSupport(qreal size,qreal angle,bool platform);
    Q_INVOKABLE void changeSize(qreal size);
    Q_INVOKABLE void changeAngle(qreal angle);

    Q_INVOKABLE int getNum(){return 100;}
    Q_INVOKABLE bool hasSupport();

    Q_INVOKABLE void onSliceClicked(bool doSlice = true);
    Q_INVOKABLE void backToPrinterConfig();

    Q_INVOKABLE void execute();
    void showOnKernelUI(bool show);
    void setFDMVisible(bool bFDMType);
protected:
    //void onSelectionsChanged() override;
    //void selectChanged(qtuser_3d::Pickable* pickable) override;

signals:
    void sigTest();

private slots:
    void slotSupportSelState(bool state);
    void slotSupportModelChange();
    void slotSupportExecute();

private:
    SimpleSupportOp *m_pOp;
    bool m_state = false;
protected:
    QObject* m_root;
    bool m_bFDMType;
    QObject* m_qmlUI;
};

#endif // SUPPORTUI_H
