#ifndef SCALEMODE_1589765331924_H
#define SCALEMODE_1589765331924_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
    class ScaleOp;
    class ScaleMode : public ToolCommand
        , public UIVisualTracer
    {
        Q_OBJECT
            Q_PROPERTY(QVector3D scale READ scale  NOTIFY scaleChanged)
            Q_PROPERTY(QVector3D size READ size NOTIFY sizeChanged)
            Q_PROPERTY(bool uniformCheck READ uniformCheck WRITE setUniformCheck NOTIFY checkChanged)
            Q_PROPERTY(QVector3D orgSize READ orgSize CONSTANT)
            Q_PROPERTY(QVector3D bindScale READ bindScale CONSTANT)
            Q_PROPERTY(QVector3D bindSize READ bindSize CONSTANT)
            //Q_PROPERTY(bool message READ message WRITE setMessage NOTIFY supportMessage)
    public:
        ScaleMode(QObject* parent = nullptr);
        virtual ~ScaleMode();

        /*void setMessage(bool isRemove);
        bool message();
        Q_INVOKABLE void accept();
        Q_INVOKABLE void cancel();*/

        Q_INVOKABLE void execute();
        Q_INVOKABLE void reset();
        QVector3D size();
        QVector3D orgSize() const;
        QVector3D scale();

        //lisugui 2020-10-14 use to qml,avoid Binding loop value
        QVector3D bindScale() const;
        QVector3D bindSize() const;
        //end


        void setScale(QVector3D scale);

        bool uniformCheck()const;
        void setUniformCheck(const bool check);

        Q_INVOKABLE void setQmlSize(float scaleValue, int xyzFlag);

        Q_INVOKABLE void setQmlScale(float scaleValue, int xyzFlag);  //x: 0 ;y 1 z2 xyz3

        Q_INVOKABLE bool checkSelect() override;
        Q_INVOKABLE void blockSignalScaleChanged(bool block);
    private  slots:
        void slotMouseLeftClicked();
    signals:
        void sizeChanged();
        void scaleChanged();
        void checkChanged();
        //void supportMessage();

    protected:
        void onThemeChanged(ThemeCategory category) override;
        void onLanguageChanged(MultiLanguage language) override;

    protected:
        ScaleOp* m_op;
    };
}
#endif // SCALEMODE_1589765331924_H
