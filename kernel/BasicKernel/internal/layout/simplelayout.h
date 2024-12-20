#ifndef SIMPLE_LAYOUT_H
#define SIMPLE_LAYOUT_H
#include "basickernelexport.h"
#include "data/modeln.h"

#include "nestplacer/placer.h"
#include "layoutmanager.h"

namespace creative_kernel
{
    class OneBinStrategy;
    class LayoutItemEx;

    class BASIC_KERNEL_API SimpleLayout : public QObject
    {
        Q_OBJECT
    public:
        explicit SimpleLayout(QObject* parent = nullptr);
        virtual ~SimpleLayout();

        void setLayoutParameterInfo(const LayoutParameterInfo& paramInfo);

        void setFixItemInfo(const std::vector<LayoutNestFixedItemInfo>& fixInfos);
        void setActiveItemInfo(const QList<CONTOUR_PATH>& activeOutlineInfos);

        void doLayout();

        QList<QVector3D> getLayoutPos();

    protected:
        creative_kernel::OneBinStrategy* m_strategy;
        QList<LayoutItemEx*> m_activeObjects;

        //the current operated bin index
        int m_currentBinIndex;

        LayoutParameterInfo m_layoutParameterInfo;

    private:
        trimesh::box3 m_box;

        std::vector<LayoutNestFixedItemInfo> m_fixInfos;

        std::vector< std::vector<trimesh::vec3> > m_activeOutlines;

        float m_modelspacing;
        float m_platformSpacing;

        //the current operated bin index
        int m_curBinIndex;

        QList<QVector3D> m_layoutPos;
    };
}

#endif // SIMPLE_LAYOUT_JOB_H
