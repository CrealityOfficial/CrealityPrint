#ifndef QCXUTIL_LETTERMODEL_1687253994913_H
#define QCXUTIL_LETTERMODEL_1687253994913_H
#include "cxkernel/data/header.h"
#include <QtCore/QObject>
#include <memory>

namespace cxkernel
{
	struct CameraModel
	{
		float f;
		float n;
		float fov;
		float aspect;

		trimesh::point pos;
		trimesh::point center;
		trimesh::point up;
	};

	typedef std::vector<trimesh::vec3> PolygonModel;
	typedef std::vector<PolygonModel> PolygonsModel;

	struct LetterConfigPara
	{
		QString dragType = "Text";
		int x = 0;
		int y = 0;
		int tShapeW = 0;
		int tShapeH = 0;
		int tRotate = 0;

		QString text = "text";
		QString fontfamily = "";
		QString fontstyle = "";
		int fontsize = 20;
	};

	class CXKERNEL_API LetterModel : public QObject
	{
		Q_OBJECT
		Q_PROPERTY(float textThickness READ textThickness WRITE setTextThickness NOTIFY textThicknessChanged)
		Q_PROPERTY(bool textSide READ textSide WRITE setTextSide NOTIFY textSideChanged)
	public:
		LetterModel(QObject* parent = nullptr);
		virtual ~LetterModel();

		float textThickness();
		void setTextThickness(float textThickness);
		bool textSide();
		void setTextSide(bool outSide);

		TriMeshPtr _generate(TriMeshPtr mesh,
			const QSize& surfaceSize, const CameraModel& camera,
			ccglobal::Tracer* tracer = nullptr);
		//解析qml的图元的 坐标等属性，在主线程中调用，在子线程中进行QQmlProperty处理会有偶现崩溃
		void parseQmlData(const QList<QObject*>& objectList);
		static QSharedPointer<LetterConfigPara> parseLetter(QObject* object);

		void generatePolygons( const QSize& surfaceSize, std::vector<PolygonsModel>& outPolygons);
		static trimesh::fxform calculateTransform(int w, int h);

		static void convertPolygons(const QList<QPolygonF>& polys, PolygonsModel& pathes);
		static void font2Polygons(QSharedPointer<LetterConfigPara> lett, PolygonsModel& polygons);

		static void transformPolygons(const QSize& surfaceSize, PolygonsModel& polygons);
	signals:
		void textThicknessChanged();
		void textSideChanged();

	protected:
		bool pConcave = true;
		float pDeep = 2.0f;

		QList<QSharedPointer<LetterConfigPara>> m_listLetterConfig;
	};
}

#endif // QCXUTIL_LETTERMODEL_1687253994913_H
