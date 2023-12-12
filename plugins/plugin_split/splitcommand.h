#ifndef _NULLSPACE_SPLITCOMMAND_1591235079966_H
#define _NULLSPACE_SPLITCOMMAND_1591235079966_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"
#include <QtGui/QVector3D>

class SplitOp;
class SplitCommand: public ToolCommand
    , public creative_kernel::UIVisualTracer
{
	Q_OBJECT
    //Q_PROPERTY(QVector3D position READ position  NOTIFY onPositionChanged)
    Q_PROPERTY(QVector3D dir READ dir  NOTIFY onRotateAngleChanged)
    //Q_PROPERTY(QVector3D defauletPos READ defauletPos WRITE setDefauletPos)
    Q_PROPERTY(QVector3D defauletDir READ defauletDir WRITE setDefauletDir)
    Q_PROPERTY(float offsetVal READ getOffset NOTIFY onOffsetChanged)
public:
	SplitCommand(QObject* parent = nullptr);
	virtual ~SplitCommand();

	Q_INVOKABLE void execute();
	Q_INVOKABLE void split();
    Q_INVOKABLE bool checkSelect() override;
    Q_INVOKABLE void undo();//by TCJ

    Q_INVOKABLE void blockSignalSplitChanged(bool block);

    Q_INVOKABLE void accept();
    Q_INVOKABLE void cancel();

    //Q_INVOKABLE void setQmlPosition(float val, int nXYZFlag);
    //Q_INVOKABLE void setQmlRotate(float val, int nXYZFlag);
    //// ÖØÖÃpos ºÍ rotate
    //Q_INVOKABLE void resetCmd();

    Q_INVOKABLE void changeAxisType(int type);
    Q_INVOKABLE void changeOffset(float off);

    void setMessage(bool isRemove);
    bool message();

private  slots:
    void slotRotateAngleChanged();
    void slotMouseLeftClicked();

protected:
    void onThemeChanged(creative_kernel::ThemeCategory category) override;
    void onLanguageChanged(creative_kernel::MultiLanguage language) override;
protected:
	SplitOp* m_op;
    int m_AxisType = 2;     //Æ«ÒÆµÄÀàÐÍ 0 : x ;1 : y ; z:2

    QVector3D position();
    QVector3D dir();
    QVector3D defauletPos();
    QVector3D defauletDir();
    QVector3D m_DefauletPos;
    QVector3D m_DefauletDir;
    void setPositon(QVector3D pos);
    void setDir(QVector3D d);
    void setDefauletPos(QVector3D pos);
    void setDefauletDir(QVector3D pos);

    float getOffset();
signals:
    void onPositionChanged();
    //void onDirChanged();
    void onRotateAngleChanged();
    void onOffsetChanged();
};
#endif // _NULLSPACE_SPLITCOMMAND_1591235079966_H
