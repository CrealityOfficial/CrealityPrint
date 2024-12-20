#ifndef CREATIVE_KERNEL_FONT_MESH_WRAPPER_1592790419297_H
#define CREATIVE_KERNEL_FONT_MESH_WRAPPER_1592790419297_H
#include "basickernelexport.h"
#include <QtWidgets/QUndoCommand>
#include <data/interface.h>
#include <QMatrix4x4>	
#include "outlinegenerator.h"
#include "cxkernel/data/meshdata.h"

class BASIC_KERNEL_API FontConfig 
{
public:
	FontConfig();
	FontConfig(const FontConfig& fontConfig);

	FontConfig& operator=(const FontConfig& other);

public:
	int reliefTargetID { -1 };

	QString text { "TEXT" };
	QString font { "Arial" };
	int fontSize { 20 };
	int wordSpace { 0 };
	int lineSpace { 0 };
	float height { 1 };
	float distance { 0 }; // 0-2
	int embossType { 0 };
	bool bold { false };
	bool italic { false };
	float angle { 0.0 };
	// int state { 0 }; //0:水平  1:环绕

	std::string fontMeshData;
};


namespace creative_kernel
{
	class ModelN;
	class ModelGroup;
};

namespace topomesh {
	class FontMesh;
}

class BASIC_KERNEL_API FontMeshWrapper : public QObject
                      , public creative_kernel::SpaceTracer
{
	Q_OBJECT
public:
	FontMeshWrapper(QObject* parent = nullptr);
	FontMeshWrapper(const FontMeshWrapper &other);
	FontMeshWrapper(const std::string& reliefDescription);
	virtual ~FontMeshWrapper();

	std::string serialize(bool isClone = false);

	bool isClonedFontMesh();

	void setRelief(creative_kernel::ModelN* relief);
	void regenerateMesh();

	bool valid();
	bool hasEmbossed();

	int lastTargetId();
	int lastFaceId();
	QVector3D lastCross();
	int lastEmbossType();

	void syncFontMesh();

	void updateOutline();
	void setHeight(float height);
	void setDistance(float distance);

	void setEmbossType(bool isSurround);

	/* rotate */
	void rotate(float angle);

	/* trans */
	void initEmboss();
	void setEmbossTarget(creative_kernel::ModelN* target);
	void syncTarget();
	void emboss(int face_id, trimesh::vec3 location);
	void syncScale();

	/* config */
	FontConfig* config();

	/* output */
	trimesh::vec3 faceTo();
	QString text();
	float angle();
	float height();

	/* modeln */
	creative_kernel::ModelN* model();
	creative_kernel::ModelGroup* group();
	void createModel(creative_kernel::ModelGroup* group);
	void updateModel(bool reset = false);
	void updateInitModel();
	
private:
	void updateMeshData();
	void autoEmboss();
    QString generateReliefName(creative_kernel::ModelGroup* group);
	QVector3D generateDefaultLoaction(creative_kernel::ModelGroup* group, const trimesh::dbox3& reliefBox);

	std::string matrix2String(const QMatrix4x4& matrix);
	void parseDescription(const std::string& reliefDescription);

protected:
	virtual void onModelGroupModified(creative_kernel::ModelGroup* _model_group, 
																const QList<creative_kernel::ModelN*>& removes, 
																const QList<creative_kernel::ModelN*>& adds) override;

private:
	OutlineGenerator* m_outlineGenerator;

	bool m_isCloned { false };

	int m_reliefId { -1 };
	creative_kernel::ModelN* m_relief { NULL };
	topomesh::FontMesh* m_fontMesh { NULL };
	FontConfig m_fc;
	// FontMove m_cras;

	/* calculate data */
	int m_lastFaceId;
	QVector3D m_lastCross; // relative
	QMatrix4x4 m_targetMatrix;

	/* QMatrix4x4 */
	QMatrix4x4 m_firstEmbossMatrix;		// record on the first emboss
	bool m_useFirst{ true };

	bool m_hasEmbossed { false };
	QMatrix4x4 m_initMatrix; // the matrix before emboss

	bool m_meshDirty { false };
	cxkernel::MeshDataPtr m_meshData;
	QMatrix4x4 m_meshMatrix;

};

#endif // CREATIVE_KERNEL_FONT_MESH_WRAPPER_1592790419297_H