//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

//#include "tools/macros.h"

#include "Fill/Quarter/NoZigZagConnectorProcessor.h"


namespace Slic3r
{

void NoZigZagConnectorProcessor::registerVertex(const Point&)
{
    //No need to add anything.
}

void NoZigZagConnectorProcessor::registerScanlineSegmentIntersection(const Point&, int)
{
    //No need to add anything.
}

void NoZigZagConnectorProcessor::registerPolyFinished()
{
    //No need to add anything.
}

} // namespace cura52 
