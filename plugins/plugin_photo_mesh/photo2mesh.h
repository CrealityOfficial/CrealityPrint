#ifndef _NULLSPACE_PHOTO2MESH_1589849922902_H
#define _NULLSPACE_PHOTO2MESH_1589849922902_H
#include "photo2meshjob.h"
#include "cxkernel/wrapper/photomeshmodel.h"

class Photo2Mesh : public QObject
{
	Q_OBJECT
public:
	Photo2Mesh(QObject* parent = nullptr);
	virtual ~Photo2Mesh();

	void processFiles(const QStringList& fileNames);

	Q_INVOKABLE void accept();
	Q_INVOKABLE void cancel();

	Q_INVOKABLE void setBaseHeight(float baseHeight);
	Q_INVOKABLE void setMaxHeight(float maxHeight);
	Q_INVOKABLE void setLighterOrDarker(int index);
	Q_INVOKABLE void setMeshWidth(float meshX);
	Q_INVOKABLE void setBlur(int blur);
protected:
	cxkernel::PhotoMeshModel* m_model;
	QStringList m_fileNames;
};
#endif // _NULLSPACE_PHOTO2MESH_1589849922902_H
