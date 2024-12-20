#ifndef _SPREADCHUNK_1697868785361_H
#define _SPREADCHUNK_1697868785361_H
#include "trimesh2/Vec.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/refactor/pxentity.h"
#include "shader_entity_export.h"

namespace qtuser_3d
{
	class PickXEntity;
	class XEffect;
};

class SpreadChunk : public qtuser_3d::Pickable
{
	Q_OBJECT
public:
	SpreadChunk(int id, QObject* parent = nullptr);
	virtual ~SpreadChunk();

	int id();
	qtuser_3d::XEntity* entity();
	void setEffect(qtuser_3d::XEffect* effect);

	int parentId(int primitiveID);
	bool getFace(int primitiveID, trimesh::vec3& p1, trimesh::vec3& p2, trimesh::vec3& p3);

	void updateData(const std::vector<trimesh::vec3>& position, const std::vector<int>& flags);
	void updateData(const std::vector<trimesh::vec3>& position, const std::vector<int>& flags, const std::vector<int>& highlightFlags);
	void setIndexMap(std::vector<int> indexMap);

	void setPose(const QMatrix4x4& pose);
	QMatrix4x4 pose() const;

	void setDefaultFlag(int defaultFlag);

private:
		
protected:
	int primitiveNum() override;
	void updateMatrix();

protected:
	int m_id;
	qtuser_3d::PickXEntity* m_entity;
	std::vector<int> m_indexMap;
	std::vector<trimesh::vec3> m_positions;
	int m_defaultFlag;
};
#endif // _SPREADCHUNK_1697868785361_H