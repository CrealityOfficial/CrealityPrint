#include "letterop.h"
#include "letterjob.h"

#include "data/modeln.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/eventinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qcxutil/trimesh2/trimesh2qgeometryrenderer.h"
#include "mmesh/trimesh/trimeshutil.h"
#include "data/shapefiller.h"
#include "data/fdmsupportgroup.h"
#include "interface/spaceinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/uiinterface.h"
#include "kernel/kernelui.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"

using namespace creative_kernel;
using namespace qtuser_3d;

#define FLOAT_SMALL 1e-5f

LetterOp::LetterOp(QObject* parent): SceneOperateMode(parent), m_vRight(1.0f, 0.0f, 0.0f)
{
	m_bShouldUpdateTextModels = false;
	m_strTextFont = "SourceHanSansCN-Normal";
	m_fTextHeight = 20.0f;
	m_fTextWidth = 0.0f;
	m_fTextThickness = 5.0f;
	m_fEmbededDepth = 1.0f;
	m_strText = "TEXT";
	m_iTextSide = 1;

	m_pModel = nullptr;
	m_iFace = -1;
	m_iMode = 0;
}

LetterOp::~LetterOp()
{
	DeleteMeshes();
	DeleteEntities();
}

void LetterOp::SetFont(QString font)
{
	m_strTextFont = font;
	m_bShouldUpdateTextModels = true;
}

QString LetterOp::GetFont()
{
	return m_strTextFont;
}

void LetterOp::SetHeight(float height)
{
	m_fTextHeight = height;
	m_bShouldUpdateTextModels = true;
}

float LetterOp::GetHeight()
{
	return m_fTextHeight;
}

void LetterOp::SetThickness(float thickness)
{
	m_fTextThickness = thickness;
	m_bShouldUpdateTextModels = true;
}

float LetterOp::GetThickness()
{
	return m_fTextThickness;
}

void LetterOp::SetText(QString text)
{
	m_strText = text;
	m_bShouldUpdateTextModels = true;
}

QString LetterOp::GetText()
{
	return m_strText;
}

void LetterOp::SetTextSide(int side)
{
	m_iTextSide = side;
}

int LetterOp::GetTextSide()
{
	return m_iTextSide;
}

void LetterOp::SetMode(int theMode)
{
	m_iMode = theMode;
	if (m_iMode == 0)
		m_pModel = nullptr;
}

void LetterOp::onAttach()
{
	ShowEntities(true);

	addLeftMouseEventHandler(this);
	addHoverEventHandler(this);
	addKeyEventHandler(this);

	addSelectTracer(this);
	onSelectionsChanged();

	m_iMode = 0;
}

void LetterOp::onDettach()
{
	ShowEntities(false);

	removeLeftMouseEventHandler(this);
	removeHoverEventHandler(this);
	removeKeyEventHandler(this);

	removeSelectorTracer(this);
}

void LetterOp::onLeftMouseButtonClick(QMouseEvent* event)
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	if (selections.size() < 1)
		return;
	if (!m_pModel || m_vMeshes.size() == 0 || m_vMeshPoses.size() == 0)
	{
		return;
	}

	m_iMode++;

	if (m_iMode == 2)
	{
		if (m_pModel->mesh()->faces.size() > 300000)
		{
			
			if(m_iTextSide <= 0)
			{
				requestQmlDialog(this, "messageDlg");
			}
			else{
				StartBoolean();
			}
		}
		else
		{
			StartBoolean();
		}
	}
}

void LetterOp::onHoverMove(QHoverEvent* event)
{
	ShowEntities(false);

	QList<creative_kernel::ModelN*> selections = selectionms();
	if (selections.size() < 1)
		return;

	QVector3D thePosition;
	QVector3D theNormal;
	int theFace;
	m_pModel = checkPickModel(event->pos(), thePosition, theNormal, &theFace);

	if (m_pModel && m_iMode < 2)
	{
		if (m_iMode == 0)
		{
			m_vPosition = thePosition;
			m_vNormal = theNormal;
			m_iFace = theFace;
		}
		else if (m_iMode == 1)
		{
			m_vRight = thePosition - m_vPosition;
		}
		
		UpdateTextMesh();
		ShowEntities(true);
	}

	requestVisUpdate();
}

void LetterOp::onKeyPress(QKeyEvent* event)
{
	if (event->key() == Qt::Key::Key_Escape)
	{
		if (m_iMode == 1)
		{
			m_iMode = 0;
			m_vRight = QVector3D(1.0f, 0.0f, 0.0f);
		}
	}
}

void LetterOp::selectChanged(qtuser_3d::Pickable* pickable)
{

}

void LetterOp::onSelectionsChanged()
{
	emit mouseLeftClicked();
}

XEntity* LetterOp::CreateEntity()
{
	QVector4D color = CONFIG_PLUGIN_VEC4(letter_text_color, letter_group);

	XEntity* entity = new qtuser_3d::XEntity();
	entity->setParameter("color", color);

	XRenderPass* pass = new XRenderPass("phong", entity);
	pass->addFilterKeyMask("view", 0);

	XEffect* effect = new XEffect(entity);
	effect->addRenderPass(pass);
	
	entity->setEffect(effect);

	return entity;
}

void LetterOp::DeleteEntities()
{
	for (XEntity* entity : m_vEntities)
	{
		if (entity)
		{
			delete entity;
		}
	}
	m_vEntities.clear();
}

void LetterOp::ShowEntities(bool show)
{
	for (XEntity* entity : m_vEntities)
	{
		if (entity)
		{
			if (show)
			{
				visShow(entity);
			}
			else
			{
				visHide(entity);
			}
		}
	}
}

void LetterOp::DeleteMeshes()
{
	for (trimesh::TriMesh* mesh: m_vMeshes)
	{
		if (mesh)
		{
			delete mesh;
		}
	}
	m_vMeshWidths.clear();
	m_vMeshes.clear();

	for (XEntity* entity : m_vEntities)
	{
		if (entity)
		{
			visHide(entity);
			//entity->setEnabled(false);
		}
	}
}

inline QVector3D TrimeshVec3ToQVec3(trimesh::vec3 trimesh_vec3)
{
	return QVector3D(trimesh_vec3.x, trimesh_vec3.y, trimesh_vec3.z);
}


void SegmentIntersectWithXozPlane(QVector3D& A, QVector3D& B, QVector3D& v)
{
	if (fabsf(A.y() - B.y()) < FLOAT_SMALL)
	{
		v.setX(A.x());
		v.setY(0.0f);
		v.setZ(A.z());
		return;
	}

	float t = A.y() / (A.y() - B.y());
	v.setX((1.0f - t) * A.x() + t * B.x());
	v.setY(0.0f);
	v.setZ((1.0f - t) * A.z() + t * B.z());
}

bool TriangleIntersectWithXozPlane(QVector3D& p1, QVector3D& p2, QVector3D& p3,
	QVector3D& ev1, QVector3D& ev2, int& beginvertex_flag, int& endvertex_flag)
{
	if (p1.y() > 0.0f && p2.y() < 0.0f && p3.y() < 0.0f)
	{
		SegmentIntersectWithXozPlane(p1, p2, ev1);
		SegmentIntersectWithXozPlane(p1, p3, ev2);
		beginvertex_flag = 4;
		endvertex_flag = 6;
	}
	else if (p1.y() < 0.0f && p2.y() >= 0.0f && p3.y() >= 0.0f)
	{
		SegmentIntersectWithXozPlane(p1, p3, ev1);
		SegmentIntersectWithXozPlane(p1, p2, ev2);
		beginvertex_flag = 6;
		endvertex_flag = 4;
		if (p2.y() < FLOAT_SMALL)
		{
			endvertex_flag = 2;
		}
		if (p3.y() < FLOAT_SMALL)
		{
			beginvertex_flag = 3;
		}
	}
	else if (p1.y() < 0.0f && p2.y() > 0.0f && p3.y() < 0.0f)
	{
		SegmentIntersectWithXozPlane(p2, p3, ev1);
		SegmentIntersectWithXozPlane(p1, p2, ev2);
		beginvertex_flag = 5;
		endvertex_flag = 4;
	}
	else if (p1.y() >= 0.0f && p2.y() < 0.0f && p3.y() >= 0.0f)
	{
		SegmentIntersectWithXozPlane(p1, p2, ev1);
		SegmentIntersectWithXozPlane(p2, p3, ev2);
		beginvertex_flag = 4;
		endvertex_flag = 5;
		if (p1.y() < FLOAT_SMALL)
		{
			endvertex_flag = 1;
		}
		if (p3.y() < FLOAT_SMALL)
		{
			beginvertex_flag = 3;
		}
	}
	else if (p1.y() < 0.0f && p2.y() < 0.0f && p3.y() > 0.0f)
	{
		SegmentIntersectWithXozPlane(p1, p3, ev1);
		SegmentIntersectWithXozPlane(p2, p3, ev2);
		beginvertex_flag = 6;
		endvertex_flag = 5;
	}
	else if (p1.y() >= 0.0f && p2.y() >= 0.0f && p3.y() < 0.0f)
	{
		SegmentIntersectWithXozPlane(p2, p3, ev1);
		SegmentIntersectWithXozPlane(p1, p3, ev2);
		beginvertex_flag = 5;
		endvertex_flag = 6;
		if (p1.y() < FLOAT_SMALL)
		{
			endvertex_flag = 1;
		}
		if (p2.y() < FLOAT_SMALL)
		{
			beginvertex_flag = 2;
		}
	}
	else
	{
		return false;  // 1 or 3 vertex(s) on XoZ plane
	}

	return true;
}

int FindNextFace(QMatrix4x4& pTm, trimesh::TriMesh* mesh, int cur_face, int endvertex_flag)
{
	int adj_face = -1;
	if (endvertex_flag < 4)
	{
		int vertex = mesh->faces[cur_face][endvertex_flag - 1];
		for (size_t i = 0; i < mesh->adjacentfaces[vertex].size(); i++)
		{
			adj_face = mesh->adjacentfaces[vertex][i];
			if (adj_face == cur_face)
			{
				continue;
			}
			trimesh::TriMesh::Face& face = mesh->faces[adj_face];
			if (face[0] == face[1] || face[1] == face[2] || face[2] == face[0])
			{
				continue;  // skip face with 0 area
			}
			QVector3D pV1 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[0]]);
			QVector3D pV2 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[1]]);
			QVector3D pV3 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[2]]);
			if ((fabsf(pV1.y()) < FLOAT_SMALL && pV2.y() * pV3.y() > 0.0f) ||
				(fabsf(pV2.y()) < FLOAT_SMALL && pV1.y() * pV3.y() > 0.0f) || 
				(fabsf(pV3.y()) < FLOAT_SMALL && pV1.y() * pV2.y() > 0.0f))
			{
				continue;
			}
			return adj_face;
		}
	}
	else if (endvertex_flag < 7)
	{
		int vertex1 = mesh->faces[cur_face][endvertex_flag - 4];
		int vertex2 = mesh->faces[cur_face][(endvertex_flag - 3) % 3];
		for (size_t i = 0; i < mesh->adjacentfaces[vertex1].size(); i++)
		{
			if (mesh->adjacentfaces[vertex1][i] == cur_face)  // skip current face
			{
				continue;
			}
			for (size_t j = 0; j < mesh->adjacentfaces[vertex2].size(); j++)
			{
				int cur_adj_face_id = mesh->adjacentfaces[vertex2][j];
				trimesh::TriMesh::Face& face = mesh->faces[cur_adj_face_id];
				if (face[0] == face[1] || face[1] == face[2] || face[2] == face[0])
				{
					continue;  // skip face with 0 area
				}
				if (mesh->adjacentfaces[vertex1][i] == mesh->adjacentfaces[vertex2][j])
				{
					return mesh->adjacentfaces[vertex1][i];
				}
			}
		}
	}

	return -1;
}

inline void GetPointOnSegmentByLength(QVector3D& p1, QVector3D& p2, float length_from_p1, QVector3D& pi)
{
	pi = p1 + (p2 - p1).normalized() * length_from_p1;
}

void LetterOp::CalculateTextEntityPositionsAndNormals(QMatrix4x4& ppc, std::vector<QVector3D>& positions, std::vector<QVector3D>& normals)
{
	trimesh::TriMesh* mesh = m_pModel->mesh();
	mmesh::dumplicateMesh(mesh, nullptr);
	if (mesh->faces.size() <= m_iFace)
	{
		return;
	}

	// searching ranges (w.r.t. ppc)
	float space = m_fTextHeight * 0.1;  // space between characters
	float range = -space;
	for (size_t i = 0; i < m_vMeshWidths.size(); i++)
	{
		range += m_vMeshWidths[i] + space;
	}
	range *= 0.5f;

	std::vector<float> letter_center_offsets;
	letter_center_offsets.push_back(m_vMeshWidths[0] * 0.5 - range);
	for (size_t i = 1; i < m_vMeshWidths.size(); i++)
	{
		letter_center_offsets.push_back(letter_center_offsets[i-1] + (m_vMeshWidths[i-1] + m_vMeshWidths[i]) * 0.5 + space);
	}

	// transfrom mesh to ppc
	float sum_length = 0.0f;
	float cur_length = 0.0f;
	QMatrix4x4 pTw = ppc.inverted();
	QMatrix4x4 pTm = pTw * m_pModel->globalMatrix();
	QVector3D pRangeCenter = pTw * m_vPosition;  // w.r.t. ppc
	QVector3D pLast = pRangeCenter;
	int fid = m_iFace;
	QVector3D ev1, ev2;  // w.r.t. ppc
	int beginvertex_flag = 0;
	int endvertex_flag = 0;
	normals.resize(letter_center_offsets.size());
	positions.resize(letter_center_offsets.size());

	mesh->need_adjacentfaces();

	/*
	*           originial of pcc
	*                 |
	*    <- backward  v  forward ->
	*  |---|-|------|-|----|-|------|
	*   A space B space C space  D
	*/

	// backward search
	while (true)
	{
		trimesh::TriMesh::Face& face = mesh->faces[fid];
		QVector3D pV1 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[0]]);
		QVector3D pV2 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[1]]);
		QVector3D pV3 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[2]]);

		if (!TriangleIntersectWithXozPlane(pV1, pV2, pV3, ev1, ev2, beginvertex_flag, endvertex_flag))
		{
			return;
		}

		cur_length = (pLast - ev1).length();

		for (size_t i = 0; i < letter_center_offsets.size(); i++)
		{
			if (-sum_length - cur_length < letter_center_offsets[i] && letter_center_offsets[i] <= -sum_length)
			{
				normals[i] = QVector3D::crossProduct(pV2 - pV1, pV3 - pV1);
				GetPointOnSegmentByLength(pLast, ev1, -sum_length - letter_center_offsets[i] , positions[i]);
			}
		}

		sum_length += cur_length;

		if (sum_length >= range - m_vMeshWidths[0] * 0.5f)
		{
			break;
		}

		pLast = ev1;

		if ((fid = FindNextFace(pTm, mesh, fid, beginvertex_flag)) < 0)
		{
			printf("[fatal error] 1 FindNextFace: model is not watertight!\n");
			return;
		}
	}

	// forward search
	sum_length = 0.0;
	pLast = pRangeCenter;
	fid = m_iFace;
	while (true)
	{
		trimesh::TriMesh::Face& face = mesh->faces[fid];
		QVector3D pV1 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[0]]);
		QVector3D pV2 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[1]]);
		QVector3D pV3 = pTm * TrimeshVec3ToQVec3(mesh->vertices[face[2]]);

		if (!TriangleIntersectWithXozPlane(pV1, pV2, pV3, ev1, ev2, beginvertex_flag, endvertex_flag))
		{
			return;
		}

		cur_length = (pLast - ev2).length();

		for (size_t i = 0; i < letter_center_offsets.size(); i++)
		{
			if (sum_length < letter_center_offsets[i] && letter_center_offsets[i] <= sum_length + cur_length)
			{
				normals[i] = QVector3D::crossProduct(pV2 - pV1, pV3 - pV1);
				GetPointOnSegmentByLength(pLast, ev2, letter_center_offsets[i] - sum_length, positions[i]);
			}
		}

		sum_length += cur_length;

		if (sum_length >= range - m_vMeshWidths.back() * 0.5f)
		{
			break;
		}

		pLast = ev2;

		if ((fid = FindNextFace(pTm, mesh, fid, endvertex_flag)) < 0)
		{
			printf("[fatal error] 2 FindNextFace: model is not watertight!\n");
			return;
		}
	}
}

void LetterOp::CalculateTextEntityPoses(std::vector<QMatrix4x4>& poses)
{
	poses.clear();

	// choose a right direction
	int dimension = 3;
	while (QVector3D::crossProduct(m_vNormal, m_vRight).length() < FLOAT_SMALL && dimension-- > 0)
	{
		float tempf = m_vRight[2];
		m_vRight[2] = m_vRight[1];
		m_vRight[1] = m_vRight[0];
		m_vRight[0] = tempf;
	}
	if (0 == dimension)
	{
		return;  // |m_vNormal| == 0
	}

	// build the "picked point coordinate"
	QVector3D ZAxis = m_vNormal.normalized();
	QVector3D YAxis = QVector3D::crossProduct(ZAxis, m_vRight);
	QVector3D XAxis = QVector3D::crossProduct(YAxis, ZAxis);

	QMatrix4x4 ppc;  // w.r.t. wcs
	ppc.setColumn(0, QVector4D(XAxis.normalized(), 0.0f));
	ppc.setColumn(1, QVector4D(YAxis.normalized(), 0.0f));
	ppc.setColumn(2, QVector4D(ZAxis.normalized(), 0.0f));
	ppc.setColumn(3, QVector4D(m_vPosition, 1.0f));

	std::vector<QVector3D> positions, normals;
	CalculateTextEntityPositionsAndNormals(ppc, positions, normals);

	if (positions.size() < m_vMeshes.size() || normals.size() < m_vMeshes.size())
	{
		return;
	}

	QVector3D prevOffsetVector(0.0f, 0.0f, 0.0f);
	QVector3D currOffsetVector(0.0f, 0.0f, 0.0f);
	QMatrix4x4 pTl;
	float zoffset = 0.0f;

	for (size_t i = 0; i < m_vMeshes.size(); i++)
	{
		if (m_vMeshes.size() == 1)
		{
			currOffsetVector = QVector3D(1.0f, 0.0f, 0.0f);
		}
		else if (m_vMeshes.size() == i + 1)
		{
			currOffsetVector = QVector3D(0.0f, 0.0f, 0.0f);
		}
		else
		{
			currOffsetVector = (positions[i + 1] - positions[i]).normalized();
		}

		ZAxis = normals[i].normalized();
		YAxis = QVector3D::crossProduct(ZAxis, prevOffsetVector + currOffsetVector).normalized();
		XAxis = QVector3D::crossProduct(YAxis, ZAxis).normalized();

		if (m_iTextSide > 0)
		{
			zoffset = m_fEmbededDepth;
		}
		else
		{
			zoffset = m_fTextThickness;
		}

		// adjust letter position: center to left-bottom corner
		positions[i] -= XAxis * (m_vMeshWidths[i] * 0.5f) + YAxis * (m_fTextHeight * 0.5f) + ZAxis * zoffset;
		
		pTl.setColumn(0, QVector4D(XAxis, 0.0f));
		pTl.setColumn(1, QVector4D(YAxis, 0.0f));
		pTl.setColumn(2, QVector4D(ZAxis, 0.0f));
		pTl.setColumn(3, QVector4D(positions[i], 1.0f));

		poses.push_back(ppc * pTl);

		prevOffsetVector = currOffsetVector;
	}
}

void LetterOp::UpdateTextMesh()
{
	if (!m_pModel || m_strText.isEmpty() || m_strTextFont.isEmpty() || m_fTextHeight <= 0.0f || m_fTextThickness < FLOAT_SMALL)
	{
		DeleteMeshes();
		return;
	}

	if (m_bShouldUpdateTextModels || m_vMeshes.empty())
	{
		m_bShouldUpdateTextModels = false;

		if (m_vMeshes.size() > 0)
		{
			DeleteMeshes();
		}

		creative_kernel::ShapeCreator::createTextMesh(m_strText, m_strTextFont, m_fTextHeight, m_fTextThickness + m_fEmbededDepth, m_vMeshWidths, m_vMeshes);

		if (0 == m_vMeshes.size())
		{
			return;
		}

		while (m_vEntities.size() < m_vMeshes.size())
		{
			m_vEntities.push_back(CreateEntity());
		}

		for (size_t i = 0; i < m_vMeshes.size(); i++)
		{
			if (!m_vMeshes[i])
			{
				continue;
			}
			//m_vEntities[i]->setEnabled(true);
			Qt3DRender::QGeometry* geom = m_vEntities[i]->geometry();
			if (geom)
			{
				delete geom;
			}
			m_vEntities[i]->setGeometry(qcxutil::createGeometryFromMesh(m_vMeshes[i]));
		}
	}

	CalculateTextEntityPoses(m_vMeshPoses);

	for (size_t i = 0; i < m_vMeshes.size(); i++)
	{
		if (!m_vEntities[i] || i >= m_vMeshPoses.size())
		{
			continue;
		}

		m_vEntities[i]->setPose(m_vMeshPoses[i]);
	}

	requestVisUpdate();
}

void LetterOp::StartBoolean()
{
	LetterJob* job = new LetterJob(this);
	job->SetModel(m_pModel);
	job->SetTextMeshs(m_vMeshes, m_vMeshPoses);
	job->SetIsTextOutside(m_iTextSide > 0);
	cxkernel::executeJob(qtuser_core::JobPtr(job));
}

QString LetterOp::getMessageText()
{
	return tr("Operation is time-consuming(10+ min), continue?");
}

void LetterOp::accept()
{
	StartBoolean();
}

void LetterOp::cancel()
{
	getKernelUI()->switchPickMode();
}

bool LetterOp::getMessage()
{
	//if (m_selectedModels.size())
	QList<ModelN*> selections = selectionms();
	for (size_t i = 0; i < selections.size(); i++)
	{
		ModelN* model = selections.at(i);
		if (model->hasFDMSupport())
		{
			FDMSupportGroup* p_support = selections.at(i)->fdmSupport();
			if (p_support->fdmSupportNum())
			{
				return true;
			}
		}
	}
	return false;
}

void LetterOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		QList<ModelN*> selections = selectionms();
		for (size_t i = 0; i < selections.size(); i++)
		{
			ModelN* model = selections.at(i);
			if (model->hasFDMSupport())
			{
				FDMSupportGroup* p_support = selections.at(i)->fdmSupport();
				p_support->clearSupports();
			}
		}
		requestVisUpdate(true);
	}
}