#ifndef CRSLICE_FROM_SCENE_COMMANDLINE_H
#define CRSLICE_FROM_SCENE_COMMANDLINE_H
#include "crslice/crscene.h"
#include "communication/Scene.h"
#include "communication/scenefactory.h"

namespace cura52
{
    class SliceContext;
}

namespace crslice
{
    class CRSliceFromScene : public cura52::SceneFactory
    {
    public:
        CRSliceFromScene(cura52::SliceContext* _application, CrScenePtr scene);
        virtual ~CRSliceFromScene();

        cura52::Scene* createSlice() override;
        bool hasSlice() const override;

    private:
        bool m_haveSlice;
        CrScenePtr m_scene;

        cura52::SliceContext* application;
    };

} //namespace crslice

#endif //CRSLICE_FROM_SCENE_COMMANDLINE_H