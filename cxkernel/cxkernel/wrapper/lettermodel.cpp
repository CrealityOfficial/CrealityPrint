#include "lettermodel.h"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtCore/QFile>
#include <QtQml/QQmlProperty>
#include <QtCore/QDebug>
#include <QtGui/QMatrix>

#include "msbase/mesh/tinymodify.h"
#include "msbase/space/cloud.h"

#if USE_TOPOMESH
#include "topomesh/interface/letter.h"
#include "topomesh/interface/poly.h"
#include "cxkernel/interface/cacheinterface.h"
#endif

namespace cxkernel
{
	LetterModel::LetterModel(QObject* parent)
		: QObject(parent)
	{
	}

	LetterModel::~LetterModel()
	{

	}

	float LetterModel::textThickness()
	{
		return pDeep;
	}

	void LetterModel::setTextThickness(float textThickness)
	{
		pDeep = textThickness;
	}

	bool LetterModel::textSide()
	{
		return pConcave;
	}

	void LetterModel::setTextSide(bool outSide)
	{
		pConcave = outSide;
	}

	TriMeshPtr LetterModel::_generate(TriMeshPtr mesh,
		const QSize& surfaceSize, const CameraModel& camera, ccglobal::Tracer* tracer)
	{
#if USE_TOPOMESH
		std::vector<PolygonsModel> polygons;
		generatePolygons(surfaceSize, polygons);

		topomesh::SimpleCamera topoCamera;
		memcpy(&topoCamera, &camera, sizeof(topomesh::SimpleCamera));
		
		topomesh::LetterParam topoParam;
		topoParam.deep = pDeep;
		topoParam.concave = pConcave;
		if (!cxkernel::isReleaseVersion())
		{
			QString cacheName = cxkernel::createNewAlgCache("letter");
			topoParam.fileName = cacheName.toLocal8Bit().constData();
		}

		TriMeshPtr result(topomesh::letter(mesh.get(), topoCamera, topoParam, polygons, nullptr, tracer));
#else
		qDebug() << QString("topomesh letter missing.");
		TriMeshPtr result;
#endif
		return result;
	}

	QSharedPointer<LetterConfigPara> LetterModel::parseLetter(QObject* object)
	{
		if (!object)
		{
			nullptr;
		}

		QVariant tmpDragVariant = QQmlProperty::read(object, "dragType");
		if (tmpDragVariant.isNull() || !tmpDragVariant.isValid())
		{
			qDebug() << "dragType error!";
			return nullptr;
		}

		QSharedPointer<LetterConfigPara> letterParaPtr(new LetterConfigPara());
		QString dragType = tmpDragVariant.toString();
		int x = QQmlProperty::read(object, "x").toInt();
		int y = QQmlProperty::read(object, "y").toInt();
		int tShapeW = QQmlProperty::read(object, "width").toInt();
		int tShapeH = QQmlProperty::read(object, "height").toInt();
		int tRotate = QQmlProperty::read(object, "rotation").toInt();
		letterParaPtr->dragType = dragType;
		letterParaPtr->x = x;
		letterParaPtr->y = y;
		letterParaPtr->tShapeW = tShapeW;
		letterParaPtr->tShapeH = tShapeH;
		letterParaPtr->tRotate = tRotate;
		if ("text" == dragType)
		{
			QString text = QQmlProperty::read(object, "text").toString();
			QString fontfamily = QQmlProperty::read(object, "fontfamily").toString();
			QString fontstyle = QQmlProperty::read(object, "fontstyle").toString();
			int fontsize = QQmlProperty::read(object, "fontsize").toInt();

			letterParaPtr->text = text;
			letterParaPtr->fontfamily = fontfamily;
			letterParaPtr->fontstyle = fontstyle;
			letterParaPtr->fontsize = fontsize;
		}

		return letterParaPtr;
	}

	void LetterModel::parseQmlData(const QList<QObject*>& objectList)
	{
		m_listLetterConfig.clear();
		for (QObject* object : objectList)
		{
			QSharedPointer<LetterConfigPara> letterParaPtr = parseLetter(object);
			if(letterParaPtr)
				m_listLetterConfig.push_back(letterParaPtr);
		}
	}

	void LetterModel::convertPolygons(const QList<QPolygonF>& polys, PolygonsModel& pathes)
	{
		pathes.clear();
		int size = polys.size();
		if (size > 0)
		{
			pathes.resize(size);
			for (int i = 0; i < size; ++i)
			{
				const QPolygonF& poly = polys.at(i);
				PolygonModel& path = pathes.at(i);

				int count = poly.size();
				for (int j = 0; j < count; ++j)
				{
					const QPointF& point = poly.at(j);
					path.push_back(trimesh::vec3(point.x(), point.y(), 0.0f));
				}
			}
		}
	}

	void LetterModel::font2Polygons(QSharedPointer<LetterConfigPara> lett, PolygonsModel& polygons)
	{
		if (!lett)
			return;

		int x = lett->x;
		int y = lett->y;
		int tShapeW = lett->tShapeW;
		int tShapeH = lett->tShapeH;
		int tRotate = lett->tRotate;

		QString text = lett->text;
		QString fontfamily = lett->fontfamily;
		QString fontstyle = lett->fontstyle;
		int fontsize = lett->fontsize;

		QPainterPath painterPath;

		QFont font;
		font.setFamily(fontfamily);
		//font.setStyleName(fontstyle);
		font.setPointSize(fontsize);

		QFontMetrics metrics(font);
		int mH = metrics.height();
		int asc = metrics.ascent();
		int py = (tShapeH - mH) / 2 + asc;

		QMatrix t;
		t.translate((qreal)(x + tShapeW / 2), (qreal)(y + tShapeH / 2));
		t.rotate((qreal)tRotate);
		t.translate(-(qreal)(x + tShapeW / 2), -(qreal)(y + tShapeH / 2));
		painterPath.addText((qreal)x, (qreal)(y + py), font, text);
		QList<QPolygonF> polys = painterPath.toSubpathPolygons(t);

		convertPolygons(polys, polygons);
	}

	void LetterModel::transformPolygons(const QSize& surfaceSize, PolygonsModel& polygons)
	{
		int w = surfaceSize.width();
		int h = surfaceSize.height();
		trimesh::fxform transform = LetterModel::calculateTransform(w, h);

		for (PolygonModel& polygon : polygons)
			msbase::applyMatrix2Points(polygon, transform);
	}

	void LetterModel::generatePolygons( const QSize& surfaceSize, std::vector<PolygonsModel>& outPolygons)
	{
		outPolygons.clear();
		for(auto paraPtr : m_listLetterConfig)
		{
			if (!paraPtr)
			{
				continue;
			}

			QString dragType = paraPtr->dragType;
			PolygonsModel paths;
			if ("text" == dragType)
			{
				font2Polygons(paraPtr, paths);
#if USE_TOPOMESH
				topomesh::simplifyPolygons(paths);
#endif
			}

			outPolygons.push_back(paths);
		}

		int w = surfaceSize.width();
		int h = surfaceSize.height();
		trimesh::fxform transform = calculateTransform(w, h);

		for (PolygonsModel& polygons : outPolygons)
			for (PolygonModel& polygon : polygons)
				msbase::applyMatrix2Points(polygon, transform);
	}

	trimesh::fxform LetterModel::calculateTransform(int w, int h)
	{
		trimesh::fxform xf1 = trimesh::fxform::trans(-1.0f, -1.0f, 0.0f);
		trimesh::fxform xf2 = trimesh::fxform::scale(2.0f / (float)w, 2.0f / (float)h, 1.0f);
		trimesh::fxform xfRot = trimesh::fxform::scale(1.0f, -1.0f, 1.0f);
		trimesh::fxform transform = xfRot * xf1 * xf2;
		return transform;
	}
}