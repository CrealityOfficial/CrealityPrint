#ifndef SPREADHEADER_1632383314974_H
#define SPREADHEADER_1632383314974_H

#include "spread/interface.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include <vector>
#include <memory>
#include <string>

#include "ccglobal/tracer.h"

namespace spread
{

    struct SPREAD_API SceneData
    {
        int faceId;
        //int chunkId;
        //int chunkIndex;
        trimesh::vec center;
        trimesh::vec cameraPos;
        //trimesh::vec normal;
        //trimesh::fxform pose;
        float radius{1.0f};
        float support_angle_threshold_deg{ 0.0f };
        trimesh::vec planeNormal;	//剖面法线
	    float planeOffset { 0 };
    };

    enum SPREAD_API CursorType {
        TRIANGLE, //三角片 
        CIRCLE,//2d圆
        FILL,
        GAP_FILL,
        SPHERE,//3d球
        // BBS
        HEIGHT_RANGE,
    };
}


#endif // SPREADHEADER_1632383314974_H