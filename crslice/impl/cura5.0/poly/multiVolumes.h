//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef MULTIVOLUMES_H
#define MULTIVOLUMES_H

#include <vector>

/* This file contains code to help fixing up and changing layers that are built from multiple volumes. */
namespace cura52
{
    class Mesh;
    class SlicedData;
    class MeshGroup;
    void carveMultipleVolumes(MeshGroup* meshGroup, std::vector<SlicedData>& datas);

    /*!
     * Expand each layer a bit and then keep the extra overlapping parts that overlap with other volumes.
     * This generates some overlap in dual extrusion, for better bonding in touching parts.
     */
    void generateMultipleVolumesOverlap(MeshGroup* meshGroup, std::vector<SlicedData>& datas);

    class MultiVolumes
    {
    public:
        /*!
         * Carve all cutting meshes.
         *
         * Make a cutting mesh limited to within the volume of all other meshes
         * and carve this volume from those meshes.
         *
         * Don't carve other cutting meshes.
         *
         * \warning Overlapping cutting meshes result in overlapping volumes. \ref carveMultipleVolumes still needs to be called
         *
         * \param[in,out] volumes The outline data of each mesh
         * \param meshes The meshes which contain the settings for each volume
         */
        static void carveCuttingMeshes(MeshGroup* meshGroup, std::vector<SlicedData>& datas);
    };

}//namespace cura52

#endif//MULTIVOLUMES_H
