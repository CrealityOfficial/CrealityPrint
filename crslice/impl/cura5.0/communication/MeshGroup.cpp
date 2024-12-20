// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include <limits>
#include <stdio.h>
#include <string.h>

#include "ccglobal/log.h"

#include "MeshGroup.h"
#include "types/Ratio.h" //For the shrinkage percentage and scale factor.
#include "utils/FMatrix4x3.h" //To transform the input meshes for shrinkage compensation and to align in command line mode.
#include "utils/floatpoint.h" //To accept incoming meshes with floating point vertices.

namespace cura52
{

    FILE* binaryMeshBlob = nullptr;

    /* Custom fgets function to support Mac line-ends in Ascii STL files. OpenSCAD produces this when used on Mac */
    void* fgets_(char* ptr, size_t len, FILE* f)
    {
        while (len && fread(ptr, 1, 1, f) > 0)
        {
            if (*ptr == '\n' || *ptr == '\r')
            {
                *ptr = '\0';
                return ptr;
            }
            ptr++;
            len--;
        }
        return nullptr;
    }

    Point3 MeshGroup::min() const
    {
        if (meshes.size() < 1)
        {
            return Point3(0, 0, 0);
        }
        Point3 ret(std::numeric_limits<coord_t>::max(), std::numeric_limits<coord_t>::max(), std::numeric_limits<coord_t>::max());
        for (const Mesh& mesh : meshes)
        {
            if (mesh.settings.get<bool>("infill_mesh") || mesh.settings.get<bool>("cutting_mesh") || mesh.settings.get<bool>("anti_overhang_mesh")) // Don't count pieces that are not printed.
            {
                continue;
            }
            Point3 v = mesh.min();
            ret.x = std::min(ret.x, v.x);
            ret.y = std::min(ret.y, v.y);
            ret.z = std::min(ret.z, v.z);
        }
        return ret;
    }

    Point3 MeshGroup::max() const
    {
        if (meshes.size() < 1)
        {
            return Point3(0, 0, 0);
        }
        Point3 ret(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min());
        for (const Mesh& mesh : meshes)
        {
            if (mesh.settings.get<bool>("infill_mesh") || mesh.settings.get<bool>("cutting_mesh") || mesh.settings.get<bool>("anti_overhang_mesh")) // Don't count pieces that are not printed.
            {
                continue;
            }
            Point3 v = mesh.max();
            ret.x = std::max(ret.x, v.x);
            ret.y = std::max(ret.y, v.y);
            ret.z = std::max(ret.z, v.z);
        }
        return ret;
    }

    void MeshGroup::clear()
    {
        for (Mesh& m : meshes)
        {
            m.clear();
        }
    }

    void MeshGroup::finalize()
    {
        // If the machine settings have been supplied, offset the given position vertices to the center of vertices (0,0,0) is at the bed center.
        Point3 meshgroup_offset(0, 0, 0);
        if (!settings.get<bool>("machine_center_is_zero"))
        {
            meshgroup_offset.x = settings.get<coord_t>("machine_width") / 2;
            meshgroup_offset.y = settings.get<coord_t>("machine_depth") / 2;
        }

#define SKIP_OFFSET_Z 1

        coord_t zOffset = 0;
        bool mesh_surpport_exist = false;

        // If a mesh position was given, put the mesh at this position in 3D space.
        for (Mesh& mesh : meshes)
        {
            Point3 mesh_offset(mesh.settings.get<coord_t>("mesh_position_x"), mesh.settings.get<coord_t>("mesh_position_y"), mesh.settings.get<coord_t>("mesh_position_z"));
            if (mesh.settings.get<bool>("center_object"))
            {
                Point3 object_min = mesh.min();
                Point3 object_max = mesh.max();
                Point3 object_size = object_max - object_min;

#if !SKIP_OFFSET_Z
                zOffset = -object_min.z;
#endif
                mesh_offset += Point3(-object_min.x - object_size.x / 2, -object_min.y - object_size.y / 2, zOffset);
            }
            mesh.offset(mesh_offset + meshgroup_offset);

            if (mesh.settings.get<bool>("support_mesh"))
                mesh_surpport_exist = true;
            int  boxx = mesh.max().x - mesh.min().x;
            int  boxy = mesh.max().y - mesh.min().y;

            const Point3 center = (mesh.max() + mesh.min()) / 2;
            const Point3 origin(center.x, center.y, 0);
            const FMatrix4x3 transformation = FMatrix4x3::scale(0.9999, 0.9999, 0.9999, origin);

            if (boxx == settings.get<coord_t>("machine_width"))
            {
                mesh.transform(transformation);
            }
            else if (boxx == settings.get<coord_t>("machine_depth"))
            {
                mesh.transform(transformation);
            }
            else if (boxy == settings.get<coord_t>("machine_width"))
            {
                mesh.transform(transformation);
            }
            else if (boxy == settings.get<coord_t>("machine_depth"))
            {
                mesh.transform(transformation);
            }

        }
        scaleFromBottom(settings.get<Ratio>("material_shrinkage_percentage_x"), settings.get<Ratio>("material_shrinkage_percentage_y"), settings.get<Ratio>("material_shrinkage_percentage_z")); // Compensate for the shrinkage of the material.

        if (mesh_surpport_exist)
        {
            for (Mesh& mesh : meshes)
            {
                if (!mesh.settings.get<bool>("support_mesh"))
                {
                    mesh.settings.add("support_enable", "false");
                    mesh.settings.add("support_mesh_drop_down", "false");
                }
            }
        }
    }

    void MeshGroup::scaleFromBottom(const Ratio factor_x, const Ratio factor_y, const Ratio factor_z)
    {
        const Point3 center = (max() + min()) / 2;
        const Point3 origin(center.x, center.y, 0);

        const FMatrix4x3 transformation = FMatrix4x3::scale(factor_x, factor_y, factor_z, origin);
        for (Mesh& mesh : meshes)
        {
            mesh.transform(transformation);
        }
    }

} // namespace cura52
