#ifndef _NULLSPACE_MODELSPACEOBSERVER_1590117524181_H
#define _NULLSPACE_MODELSPACEOBSERVER_1590117524181_H
#include "qtuser3d/math/box3d.h"

class SpaceTracer
{
public:
	virtual ~SpaceTracer() {}

	virtual void onBoxChanged(qtuser_3d::Box3D& box) = 0;
    virtual void onSceneChanged(qtuser_3d::Box3D& box) = 0;
};

#endif // _NULLSPACE_MODELSPACEOBSERVER_1590117524181_H
