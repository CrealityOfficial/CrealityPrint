#ifndef _NULLSPACE_COLOR_EXECUTOR_1591235079966_H
#define _NULLSPACE_COLOR_EXECUTOR_1591235079966_H

#include "colorworker.h"
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>

class ColorJob;

class BASIC_KERNEL_API ColorExecutor : public QObject
{
  Q_OBJECT
signals: 
	void sig_needUpdate(void* wrapper, std::vector<int> dirtyChunks);

public:
  	explicit ColorExecutor(QObject* parent = nullptr);
  	virtual ~ColorExecutor() = default;

	void setColorIndex(int colorIndex);
	trimesh::TriMesh* getTrimesh();

	/* 上色 */
	void triangleColor(spread::MeshSpreadWrapper *wrapper);
	void circleColor(spread::MeshSpreadWrapper *wrapper, const spread::SceneData& data, bool isFirstCircle);
	void sphereColor(spread::MeshSpreadWrapper *wrapper, const spread::SceneData& data, float sphereSize, bool isFirstCircle);
	void fillColor(spread::MeshSpreadWrapper *wrapper);
	void heightRangeColor(spread::MeshSpreadWrapper *wrapper, const spread::SceneData& data, float height);
	void fillGapColor(spread::MeshSpreadWrapper *wrapper);

	/* workers */
	void appendWorker(ColorWorker* worker);
	ColorWorker* takeWorker();
	void clearWorkers();
	void lockWork();
	void unLockWork();

private:
	bool isOperateValid();

	/* job */
	void activeJob();

private:
	spread::MeshSpreadWrapper *m_meshSpreadWrapper { NULL };
	int m_colorIndex { 0 };	// 颜色索引

	QList<ColorWorker*> m_workers;
	QMutex m_workerMutex;

	ColorJob* m_job { NULL };

	QTimer m_operateTimer;
	
};

#endif // _NULLSPACE_COLOR_EXECUTOR_1591235079966_H