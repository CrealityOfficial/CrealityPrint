#ifndef slic3r_Brim_hpp_
#define slic3r_Brim_hpp_

#include "ExPolygon.hpp"
#include "ObjectID.hpp"
#include "Point.hpp"

#include<map>
#include<vector>

namespace Slic3r {

class Print;
class ExtrusionEntityCollection;
class PrintTryCancel;

// Produce brim lines around those objects, that have the brim enabled.
// Collect islands_area to be merged into the final 1st layer convex hull.
ExtrusionEntityCollection make_brim(const Print& print, PrintTryCancel try_cancel, Polygons& islands_area);
void make_brim(const Print& print, PrintTryCancel try_cancel,
    Polygons& islands_area, std::map<ObjectID, ExtrusionEntityCollection>& brimMap,
    std::map<ObjectID, ExtrusionEntityCollection>& supportBrimMap,
    std::map<ObjectInstanceID, ExtrusionEntityCollection>& brimMapByInstance,
    std::map<ObjectInstanceID, ExtrusionEntityCollection>& supportBrimMapByInstance,
    std::vector<std::pair<ObjectID, unsigned int>>& objPrintVec,
    std::vector<unsigned int>& printExtruders,
    std::map<ObjectInstanceID, ExPolygons>* objectBrimAreasByInstanceOut = nullptr);

// BBS: automatically make brim
ExtrusionEntityCollection make_brim_auto(const Print &print, PrintTryCancel try_cancel, Polygons &islands_area);

} // Slic3r

#endif // slic3r_Brim_hpp_
