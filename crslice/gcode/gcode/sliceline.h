#ifndef _GCODE_NULLSPACE_SLICELINE_1590032412412_H
#define _GCODE_NULLSPACE_SLICELINE_1590032412412_H
#include "trimesh2/Vec.h"

enum class SliceLineType
{
    NoneType = 0, // used to mark unspecified jumps in polygons. libArcus depends on it
    OuterWall = 1,//";TYPE:WALL-OUTER"
    InnerWall = 2,//";TYPE:WALL-INNER"
    Skin = 3,//";TYPE:SKIN"
    Support = 4,//";TYPE:SUPPORT"
    SkirtBrim = 5,//";TYPE:SKIRT"
    Infill = 6,//";TYPE:FILL"
    SupportInfill = 7,//";TYPE:SUPPORT"
    MoveCombing = 8,//";TYPE:SUPPORT-INTERFACE"
    MoveRetraction = 9,//";TYPE:PRIME-TOWER"
    SupportInterface = 10,
    PrimeTower = 11,
    MoveOnly = 12,
    Travel = 13,
    React = 14,
	FlowTravel = 15,
	AdvanceTravel = 16,

    NumPrintFeatureTypes = 17 // this number MUST be the last one because other modules will
                              // use this symbol to get the total number of types, which can
                              // be used to create an array or so
};

#endif // _GCODE_NULLSPACE_SLICELINE_1590032412412_H
