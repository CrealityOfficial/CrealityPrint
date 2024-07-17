#ifndef _SPREADMODEL_1697868472973_H
#define _SPREADMODEL_1697868472973_H
#include <QtCore/QObject>
#include <set>
#include "cxkernel/data/header.h"
#include "spreadchunk.h"
#include "spread/meshspread.h"

namespace qtuser_3d
{
	class Pickable;
};

class SpreadModel : public QObject
{
	Q_OBJECT
public:
	SpreadModel(QObject* object = nullptr);
	virtual ~SpreadModel();

	void init();
	void clear();
	void addChunk(const std::vector<trimesh::vec3>& positions, const std::vector<int>& flags);
	int chunkCount();

	void updateChunkData(int id, const std::vector<trimesh::vec3>& position, const std::vector<int>& flags);
	void updateChunkData(int id, const std::vector<trimesh::vec3>& position, const std::vector<int>& flags, const std::vector<int>& originFlags);
	void setChunkIndexMap(int id, const std::vector<int>& indexMap);

	void setPose(const QMatrix4x4& pose);
	QMatrix4x4 pose() const;

	void setSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal);
	void setPalette(const std::vector<trimesh::vec>& colorsList);

	int chunkId(qtuser_3d::Pickable* pickable);
	bool getChunkFace(int chunkId, int faceId, trimesh::vec3& p1, trimesh::vec3& p2, trimesh::vec3& p3);
	int spreadFaceParentId(int chunkId, int faceId);

	void setHighlightOverhangsDeg(float deg);

	void setNeedHighlightEnabled(bool enabled);

	void setNeedMixEnabled(bool enabled);

	void setDefaultFlag(int defaultFlag);
	int defaultFlag();

protected:
	QList<SpreadChunk*> m_chunks;
	qtuser_3d::XEffect* m_effect;

	int m_defaultFlag { 1 };

};

#endif // _SPREADMODEL_1697868472973_H