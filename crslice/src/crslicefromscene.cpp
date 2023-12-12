#include "crslicefromscene.h"
#include "ccglobal/log.h"

#include "create.h"

namespace crslice
{
    CRSliceFromScene::CRSliceFromScene(cura52::SliceContext* _application, CrScenePtr scene)
        : m_haveSlice(true)
        , m_scene(scene)
        , application(_application)
    {

    }

    CRSliceFromScene::~CRSliceFromScene()
    {
    
    }

    bool CRSliceFromScene::hasSlice() const
    {
        return m_haveSlice;
    }

    cura52::Scene* CRSliceFromScene::createSlice()
    {
        cura52::Scene* slice = createSliceFromCrScene(application, m_scene);
        m_scene->release();
        m_haveSlice = false;
        return slice;
    }
} // namespace crslice