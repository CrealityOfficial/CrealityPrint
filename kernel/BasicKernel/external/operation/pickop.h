#ifndef _NULLSPACE_PICKOP_1589770383921_H
#define _NULLSPACE_PICKOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "data/interface.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "qtuser3d/math/box3d.h"
#include "data/modeln.h"

#define PosMax 10000

namespace creative_kernel
{
    class Kernel;
    class BASIC_KERNEL_API PickOp : public qtuser_3d::SceneOperateMode
        , public ModelNSelectorTracer
    {
        enum class TMode
        {
            null,
            x,
            y,
            z,
            xy,
            yz,
            xz,
            sp

        };

        Q_OBJECT
    public:
        PickOp(QObject* parent = nullptr);
        virtual ~PickOp();

        void setType(int type) { m_type = type; }
        void reset();
        void center();
        QVector3D position();
        void setPosition(QVector3D position);
        void setBottom();
    signals:
        void positionChanged();
    protected:
        void onAttach() override;
        void onDettach() override;
        void onLeftMouseButtonPress(QMouseEvent* event) override;
        void onLeftMouseButtonRelease(QMouseEvent* event) override;
        void onLeftMouseButtonMove(QMouseEvent* event) override;
        void onLeftMouseButtonClick(QMouseEvent* event) override;
        void onKeyPress(QKeyEvent* event) override;
        void onKeyRelease(QKeyEvent* event) override;

        bool shouldMultipleSelect() override;

        void onSelectionsChanged() override;
    protected:
        void setSelectedModel(creative_kernel::ModelN* model);
        void setSelectedModel(QList<creative_kernel::ModelN*> models);

        void buildFromSelections();
        QVector3D process(const QPoint& point);
        void getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray);
        QVector3D clip(const QVector3D& delta);

        void updateHelperEntity();
    private:
        qtuser_3d::TranslateHelperEntity* m_helperEntity;

        QVector3D m_spacePoint;

        TMode m_mode;

        QVector3D m_saveLocalPosition;
        QList<QVector3D> m_saveLocalPositions;

        creative_kernel::ModelN* m_selectedModel;
        QList<creative_kernel::ModelN*> m_selectedModels;

        bool moveEnable;
    };
}
#endif // _NULLSPACE_TRANSLATEOP_1589770383921_H
