#ifndef PRINT_PIECE_H
#define PRINT_PIECE_H
#include "nestplacer/export.h"
#include "ccglobal/tracer.h"

#include "trimesh2/Box.h"
#include <vector>

namespace nestplacer {

    typedef std::vector<trimesh::vec3> PR_Polygon;
    
    NESTPLACER_API void loadFromPrintFile(const std::string& fileName, std::vector<PR_Polygon>& polys, trimesh::vec3& mp, bool calDist);
    NESTPLACER_API PR_Polygon sweepAreaProfile(const PR_Polygon& station, const PR_Polygon& orbit, const trimesh::vec3& mp, std::string* fileName = nullptr);

    enum class ContactState :int {
        INTERSECT = 0, ///<相交
        TANGENT,       ///<相切
        SEPARATE,      ///<相离
        //INCLUDE,       ///<包含
    };

    struct PR_RESULT {
        int first;
        int second;
        ContactState state;
        double dist;
    };

    NESTPLACER_API void collisionCheck(const std::vector<PR_Polygon>& polys, std::vector<PR_RESULT>& results, bool calConvex = true, bool calDist = false, std::string* fileName = nullptr);
}
#endif //PRINT_PIECE_H