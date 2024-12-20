//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef SCENE_H
#define SCENE_H

#include "ExtruderTrain.h" //To store the extruders in the scene.
#include "MeshGroup.h" //To store the mesh groups in the scene.
#include "settings/Settings.h" //To store the global settings.
#include "crslice/gcode/header.h"

namespace cura52
{
    class Application;
    /*
     * Represents a scene that should be sliced.
     */
    class Scene
    {
    public:
        /*
         * \brief The global settings in the scene.
         */
        Settings settings;

        gcode::GcodeTracer* fDebugger = nullptr;
        std::string gcodeFile;
        std::string ploygonFile;
        std::string supportFile;
        std::string antiSupportFile;
        std::string seamFile;
        std::string antiSeamFile;
        std::vector<std::string> m_Object_Exclude_FileName;
        /*
         * \brief Which extruder to evaluate each setting on, if different from the
         * normal extruder of the object it's evaluated for.
         */
        std::unordered_map<std::string, cura52::ExtruderTrain*> limit_to_extruder;

        /*
         * \brief The mesh groups in the scene.
         */
        std::vector<MeshGroup> mesh_groups;

        /*
         * \brief The extruders in the scene.
         */
        std::vector<ExtruderTrain> extruders;
        bool machine_center_is_zero = false;
        /*
         * \brief The mesh group that is being processed right now.
         *
         * During initialisation this may be nullptr. For the most part, during the
         * slicing process, you can be assured that this will not be null so you can
         * safely dereference it.
         */
        std::vector<MeshGroup>::iterator current_mesh_group;

        /*
         * \brief Create an empty scene.
         *
         * This scene will have no models in it, no extruders, no settings, no
         * nothing.
         * \param num_mesh_groups The number of mesh groups to allocate for.
         */
        Scene(const size_t num_mesh_groups);

        /*
         * \brief Gets a string that contains all settings.
         *
         * This string mimics the command line call of CuraEngine. In theory you
         * could call CuraEngine with this output in the command in order to
         * reproduce the output.
         */
        const std::string getAllSettingsString() const;

        /*
         * \brief Empty out the slice instance, restoring it as if it were a new
         * instance.
         *
         * Since you are not allowed to copy, move or assign slice objects, this is
         * the only way in which you can prepare for the next slice.
         */
        void reset();
    private:
        /*
         * \brief You are not allowed to copy the scene.
         */
        Scene(const Scene&) = delete;

        /*
         * \brief You are not allowed to copy by assignment either.
         */
        Scene& operator =(const Scene&) = delete;
    };

} //namespace cura52

#endif //SCENE_H