#ifndef CRSLICE_CRGROUP_CREATE_1669515380929_H
#define CRSLICE_CRGROUP_CREATE_1669515380929_H
#include "communication/Scene.h"
#include "crslice/crscene.h"

namespace cura52
{
	class SliceContext;
}
namespace crslice
{
	cura52::Scene* createSliceFromCrScene(cura52::SliceContext* application, CrScenePtr scene);
}

#endif // CRSLICE_CRGROUP_CREATE_1669515380929_H