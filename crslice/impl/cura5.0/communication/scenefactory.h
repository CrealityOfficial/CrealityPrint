//  Copyright (c) 2018-2022 Ultimaker B.V.
//  CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef SCENE_FACTORY_N_H
#define SCENE_FACTORY_N_H

namespace cura52
{
    class Scene;
    /*
     * An abstract class to provide a common interface to create scene
     */
    class SceneFactory
    {
    public:
        virtual ~SceneFactory() {}

        virtual bool hasSlice() const = 0;
        virtual Scene* createSlice() = 0;
    };

} //namespace cura52

#endif //SCENE_FACTORY_N_H

