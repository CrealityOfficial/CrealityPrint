#ifndef MULTI_BIN_EXTEND_STRATEGY_H
#define MULTI_BIN_EXTEND_STRATEGY_H
#include "nestplacer/placer.h"
#include "qtuser3d/math/box3d.h"

namespace creative_kernel
{

    // this strategy only need the first box info, and need some translation after layout algorythm
    class  MultiBinExtendStrategy : public nestplacer::BinExtendStrategy
    {
    public:
        MultiBinExtendStrategy(int priorBin = 0);
        virtual ~MultiBinExtendStrategy();

        trimesh::box3 bounding(int index) const override;
        trimesh::box3 fixBounding(int fixBinId) const override;

        std::vector<int> getPriorIndexArray() const;
        int getPriorPrinterIndex(int resultIndex) const;

        void setEstimateBoxSize(int boxSize);

    public:
        qtuser_3d::Box3D m_box0;  // the first printer box in scene 
        int m_priorBinId = 0;
        bool m_hasWipeTowerFlag;
        int m_estimateBoxSizes;
        QList<qtuser_3d::Box3D> m_estimateBoxs;
    };

}

#endif // CXKERNEL_PLACE_ITEM_H
