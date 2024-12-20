// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include "Scene.h"
#include "Application.h"

namespace cura52
{

    Scene::Scene(const size_t num_mesh_groups) : mesh_groups(num_mesh_groups), current_mesh_group(mesh_groups.begin())
    {
        for (MeshGroup& mesh_group : mesh_groups)
        {
            mesh_group.settings.setParent(&settings);
        }
    }

    const std::string Scene::getAllSettingsString() const
    {
        std::stringstream output;
        output << settings.getAllSettingsString(); // Global settings.

        // Per-extruder settings.
        for (size_t extruder_nr = 0; extruder_nr < extruders.size(); extruder_nr++)
        {
            output << " -e" << extruder_nr << extruders[extruder_nr].settings.getAllSettingsString();
        }

        for (size_t mesh_group_index = 0; mesh_group_index < mesh_groups.size(); mesh_group_index++)
        {
            if (mesh_group_index == 0)
            {
                output << " -g";
            }
            else
            {
                output << " --next";
            }

            // Per-mesh-group settings.
            const MeshGroup& mesh_group = mesh_groups[mesh_group_index];
            output << mesh_group.settings.getAllSettingsString();

            // Per-object settings.
            for (size_t mesh_index = 0; mesh_index < mesh_group.meshes.size(); mesh_index++)
            {
                const Mesh& mesh = mesh_group.meshes[mesh_index];
                output << " -e" << mesh.settings.get<size_t>("extruder_nr") << " -l \"" << mesh_index << "\"" << mesh.settings.getAllSettingsString();
            }
        }
        output << "\n";

        return output.str();
    }

    void Scene::reset()
    {
        extruders.clear();
        mesh_groups.clear();
        settings = Settings();
    }
} // namespace cura52