#include "placeitem.h"

namespace cxkernel
{
    PlaceItemEx::PlaceItemEx(const std::vector<trimesh::vec3>& outline, QObject* parent)
        : QObject(parent)
        , m_panIndex(-1)
    {
        m_outline = outline;
    }

    PlaceItemEx::~PlaceItemEx()
    {
#ifdef DEBUG
        qInfo() << "==== " << Q_FUNC_INFO;
#endif // DEBUG

    }

    void PlaceItemEx::polygon(nestplacer::PlacerItemGeometry& geometry)
    {
        geometry.outline = m_outline;
        geometry.binIndex = m_panIndex;
        //geometry.holes = 
    }

    void PlaceItemEx::setNestResult(const nestplacer::PlacerResultRT& _result)
    {
        result = _result;
    }

    void PlaceItemEx::setFixPanIndex(int panIndex)
    {
        m_panIndex = panIndex;
    }
}