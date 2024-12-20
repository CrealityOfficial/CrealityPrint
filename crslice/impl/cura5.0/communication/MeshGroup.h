//Copyright (C) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef MESH_GROUP_H
#define MESH_GROUP_H

#include "mesh.h"
#include "tools/NoCopy.h"

namespace cura52
{

    class FMatrix4x3;

    /*!
     * A MeshGroup is a collection with 1 or more 3D meshes.
     *
     * One MeshGroup is a whole which is printed at once.
     * Generally there is one single MeshGroup, though when using one-at-a-time printing, multiple MeshGroups are processed consecutively.
     */
    class MeshGroup : public NoCopy
    {
    public:
        std::vector<Mesh> meshes;
        FPoint3 m_offset;
        Settings settings;

        Point3 min() const; //! minimal corner of bounding box
        Point3 max() const; //! maximal corner of bounding box

        void clear();

        void finalize();

        /*!
         * Scale the entire mesh group, with the bottom center as origin point.
         *
         * The mesh group is scaled around the bottom center of its bounding box. So
         * that's the center in the X and Y directions, but Z=0. This simulates the
         * shrinkage while sticking to the build plate.
         */
        void scaleFromBottom(const Ratio factor_x, const Ratio factor_y, const Ratio factor_z);
    };

} //namespace cura52

#endif //MESH_GROUP_H
