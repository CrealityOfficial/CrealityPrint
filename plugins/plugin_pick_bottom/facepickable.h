#ifndef _QTUSER_3D_FACEPICKABLE_1639375311688_H
#define _QTUSER_3D_FACEPICKABLE_1639375311688_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/module/pickable.h"
#include "trimesh2/TriMesh.h"

#include <QVector4D>


namespace qtuser_3d
{
	class PickFaceEntity;
}

class FacePickable : public qtuser_3d::Pickable
{
	Q_OBJECT
public:
	FacePickable(trimesh::TriMesh* mesh,QObject* parent = nullptr);
	virtual ~FacePickable();
	void setEntity(qtuser_3d::PickFaceEntity* faceEntity);
	trimesh::vec3 gNormal();
	qtuser_3d::PickFaceEntity* getEntity();

	void setHoverColor(QVector4D color) { m_hoverColor = color; }
	void setNoHoverColor(QVector4D color) { m_noHoverColor = color; }
	void setOriginFacesArea(trimesh::TriMesh* mesh);
	double getOriginFacesArea();
protected:
	int primitiveNum() override;
	void onStateChanged(qtuser_3d::ControlState state) override;
	void faceBaseChanged(int faceBase) override;
private:
	QVector4D m_hoverColor;
	QVector4D m_noHoverColor;

	trimesh::TriMesh* m_mesh;
	qtuser_3d::PickFaceEntity* m_face;
	double m_origin_faces_area;
};

#endif // _QTUSER_3D_FACEPICKABLE_1639375311688_H
