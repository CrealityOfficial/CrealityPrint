#ifndef _NULLSPACE_COLOR_WORKER_1591235079966_H
#define _NULLSPACE_COLOR_WORKER_1591235079966_H

#include <QObject>
#include <QMatrix4x4>
#include "cxkernel/data/header.h"
#include "spread/meshspread.h"

namespace spread
{
	class MeshSpreadWrapper;
	struct SceneData;
	enum  CursorType;
};

class ColorWorker : public QObject
{
  Q_OBJECT
signals: 

public:
  	explicit ColorWorker(spread::MeshSpreadWrapper *wrapper, int colorIndex, spread::CursorType colorMethod, QObject* parent = nullptr);
  	virtual ~ColorWorker() = default;

	void setTriangleParameter(int triangleID);
	void setCircleParameter(spread::SceneData& data, bool isFirstCircle);
	std::vector<int> execute();

private:
	spread::CursorType m_colorMethod;
	spread::MeshSpreadWrapper *m_meshSpreadWrapper { NULL };
	int m_colorIndex;	// 颜色索引

	int m_faceId;	// 原始面的faceID
	int m_triangleId;// 实际面的索引

	/* Circle参数 */
	bool m_isFirstCircle;
	trimesh::vec m_center;	// 圆心的世界坐标
	trimesh::vec m_cameraPos;		// 相机位置
	float m_radius;					// 圆形半径
	trimesh::vec m_planeNormal;	//剖面法线
	float m_planeOffset { 0 };

	/* Fill参数 */

};

#endif // _NULLSPACE_COLOR_WORKER_1591235079966_H