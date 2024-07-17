//Copyright (c) 2019 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "polygon.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace svg
{
    size_t ConstPolygonRef::size() const
    {
        return path->size();
    }

    bool ConstPolygonRef::empty() const
    {
        return path->empty();
    }

    bool Polygons::empty() const
    {
        return paths.empty();
    }

    unsigned int Polygons::pointCount() const
    {
        unsigned int count = 0;
        for (const Clipper3r::Path& path : paths)
        {
            count += path.size();
        }
        return count;
    }

    std::vector<PolygonsPart> Polygons::splitIntoParts(bool unionAll) const
    {
        std::vector<PolygonsPart> ret;
        Clipper3r::Clipper clipper(clipper_init);
        Clipper3r::PolyTree resultPolyTree;
        clipper.AddPaths(paths, Clipper3r::ptSubject, true);
        if (unionAll)
            clipper.Execute(Clipper3r::ctUnion, resultPolyTree, Clipper3r::pftNonZero, Clipper3r::pftNonZero);
        else
            clipper.Execute(Clipper3r::ctUnion, resultPolyTree);

        splitIntoParts_processPolyTreeNode(&resultPolyTree, ret);
        return ret;
    }

    void Polygons::splitIntoParts_processPolyTreeNode(Clipper3r::PolyNode* node, std::vector<PolygonsPart>& ret) const
    {
        for (int n = 0; n < node->ChildCount(); n++)
        {
            Clipper3r::PolyNode* child = node->Childs[n];
            PolygonsPart part;
            part.add(child->Contour);
            for (int i = 0; i < child->ChildCount(); i++)
            {
                part.add(child->Childs[i]->Contour);
                splitIntoParts_processPolyTreeNode(child->Childs[i], ret);
            }
            ret.push_back(part);
        }
    }
}//namespace cxutil