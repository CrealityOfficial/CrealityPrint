#ifndef ONE_BIN_STRATEGY_H
#define ONE_BIN_STRATEGY_H
#include "nestplacer/placer.h"
#include "qtuser3d/math/box3d.h"

namespace creative_kernel
{

    class  OneBinStrategy : public nestplacer::BinExtendStrategy
    {
    public:
        OneBinStrategy(int binIndex = 0);
        virtual ~OneBinStrategy();

        trimesh::box3 bounding(int index) const override;
        trimesh::box3 fixBounding(int fixBinId) const override;

    public:
        qtuser_3d::Box3D m_box;
        int m_binIndex = 0;
    };

}

#endif // ONE_BIN_STRATEGY_H
