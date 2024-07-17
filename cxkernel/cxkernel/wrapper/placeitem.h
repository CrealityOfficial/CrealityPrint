#ifndef CXKERNEL_PLACE_ITEM_H
#define CXKERNEL_PLACE_ITEM_H
#include "cxkernel/data/header.h"
#include "nestplacer/placer.h"

namespace cxkernel
{

    class PlaceItemEx : public QObject, public nestplacer::PlacerItem
    {
        Q_OBJECT
    public:
        PlaceItemEx(const std::vector<trimesh::vec3>& outline, QObject* parent = nullptr);
        virtual ~PlaceItemEx();

        void polygon(nestplacer::PlacerItemGeometry& geometry) override;  // loop polygon

        void setNestResult(const nestplacer::PlacerResultRT& _result);

        void setFixPanIndex(int panIndex);

    public:
        std::vector<trimesh::vec3> m_outline;
        nestplacer::PlacerResultRT result;
        int m_panIndex;
    };

}

#endif // CXKERNEL_PLACE_ITEM_H
