#pragma once
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include <QtGui/QVector3D>

namespace creative_kernel
{
	class ModelN;
}

namespace qtuser_3d
{
	class XEntity;
}

namespace trimesh
{
	class TriMesh;
}

class LetterOp : public qtuser_3d::SceneOperateMode
	, public qtuser_3d::SelectorTracer
{
	Q_OBJECT
public:
	LetterOp(QObject* parent = nullptr);
	virtual ~LetterOp();

	void SetFont(QString font);
	QString GetFont();
	void SetHeight(float height);
	float GetHeight();
	void SetThickness(float thickness);
	float GetThickness();
	void SetText(QString text);
	QString GetText();
	void SetTextSide(int side);
	int GetTextSide();
	void SetMode(int theMode);

	bool getMessage();
	void setMessage(bool isRemove);

protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;
	void onKeyPress(QKeyEvent* event) override;

	void selectChanged(qtuser_3d::Pickable* pickable) override;
	void onSelectionsChanged() override;

	qtuser_3d::XEntity* CreateEntity();
	void DeleteEntities();
	void ShowEntities(bool show);
	void DeleteMeshes();
	// ppc w.r.t. wcs, positions and normals w.r.t. ppc
	void CalculateTextEntityPositionsAndNormals(QMatrix4x4& ppc, std::vector<QVector3D>& positions, std::vector<QVector3D>& normals);
	void CalculateTextEntityPoses(std::vector<QMatrix4x4>& poses);  // poses w.r.t. wcs
	void UpdateTextMesh();
	void StartBoolean();

	Q_INVOKABLE QString getMessageText();
	Q_INVOKABLE void accept();
	Q_INVOKABLE void cancel();

private:
	bool m_bShouldUpdateTextModels;
	QString m_strTextFont;
	float m_fTextHeight;
	float m_fTextWidth;
	float m_fTextThickness;  // m_fTextThickness>0: relief text(text is outside the shape), m_fTextThickness<0:intaglio text
	float m_fEmbededDepth;
	QString m_strText;
	int m_iTextSide;

	std::vector<qtuser_3d::XEntity*> m_vEntities;
	std::vector<float> m_vMeshWidths;
	std::vector<trimesh::TriMesh*> m_vMeshes;  // text meshes
	std::vector<QMatrix4x4> m_vMeshPoses;      // text mesh poses

	creative_kernel::ModelN* m_pModel;
	QVector3D m_vPosition;  // w.r.t. wcs
	QVector3D m_vNormal;    // w.r.t. wcs
	QVector3D m_vRight;     // w.r.t. wcs
	int m_iFace;
	int m_iMode;  // 0:select position, 1:select direction, 2:merge(boolean operation)
signals:
	void mouseLeftClicked();
};
