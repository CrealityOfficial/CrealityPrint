#ifndef _NULLSPACE_COLOR_EXECUTOR_1591235079966_H
#define _NULLSPACE_COLOR_EXECUTOR_1591235079966_H

#include "colorworker.h"
#include <QMutex>
#include <QElapsedTimer>
#include <QTimer>

class ColorJob;

class ColorExecutor : public QObject
{
  Q_OBJECT
signals: 
	void sig_needUpdate(std::vector<int> dirtyChunks);

public:
  	explicit ColorExecutor(spread::MeshSpreadWrapper *wrapper, QObject* parent = nullptr);
  	virtual ~ColorExecutor() = default;

	void setColorIndex(int colorIndex);
	trimesh::TriMesh* getTrimesh();

	/* 上色 */
	void triangleColor();
	void circleColor(spread::SceneData& data, bool isFirstCircle);
	void fillColor();

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