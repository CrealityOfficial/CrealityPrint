#ifndef _NULLSPACE_SLICEATTAIN_1590249988869_H
#define _NULLSPACE_SLICEATTAIN_1590249988869_H
#include <QtCore/QObject>
#include <QtCore/QUrl>
//#include "crslice/gcode/define.h"
#include "crslice/gcode/header.h"

#include "crslice/gcode/sliceresult.h"
#include "cxgcode/simplegcodebuilder.h"

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>

#include "qtuser3d/math/box3d.h"

#include <unordered_map>
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"

namespace cxgcode
{
	enum class SliceAttainType
	{
		sat_slice,
		sat_file
	};

	class CXGCODE_API SliceAttain : public QObject
	{
		Q_OBJECT
	public:
		SliceAttain(const QString& slicePath, const QString& tempPath,
			SliceResultPointer result, SliceAttainType type, QObject* parent = nullptr);
		virtual ~SliceAttain();

		void build(ccglobal::Tracer* tracer = nullptr);

		Q_INVOKABLE QString sliceName();
		Q_INVOKABLE QString sourceFileName();
        Q_INVOKABLE bool isFromFile();

		Q_INVOKABLE int printTime();
		Q_INVOKABLE QString material_weight();
		Q_INVOKABLE QString printing_time();
		Q_INVOKABLE QString material_money();
		Q_INVOKABLE QString material_length();
		const QString& layerGcode(int layer);

		trimesh::box3 box();
		int layers();
		int steps(int layer);
		int totalSteps();
		int nozzle();
		float minSpeed();
		float maxSpeed();

		float minTimeOfLayer();
		float maxTimeOfLayer();

		float minFlowOfStep();
		float maxFlowOfStep();

		float minLineWidth();
		float maxLineWidth();

		float minLayerHeight();
		float maxLayerHeight();

		float minTemperature();
		float maxTemperature();

		float layerHeight();
		float lineWidth();

		gcode::TimeParts getTimeParts() const;

		QImage* getImageFromGcode();
		QString fileNameFromGcode();

		float traitSpeed(int layer, int step);
		trimesh::vec3 traitPosition(int layer, int step);
		trimesh::fxform modelMatrix();

		int findViewIndexFromStep(int layer, int nStep);
		int findStepFromViewIndex(int layer, int nViewIndex);
		void updateStepIndexMap(int layer);

        float getMachineHeight();
        float getMachineWidth();
        float getMachineDepth();
        int getBeltType();

		//fdm
		void setGCodeVisualType(gcode::GCodeVisualType type);
		Qt3DRender::QGeometry* createGeometry();
		Qt3DRender::QGeometry* createRetractionGeometry();
		Qt3DRender::QGeometry* createZSeamsGeometry();

		Qt3DRender::QGeometryRenderer* createRetractionGeometryRenderer();
		Qt3DRender::QGeometryRenderer* createZSeamsGeometryRenderer();

        void saveGCode(const QString& fileName, QImage* previewImage);
		void saveTempGCode();
		void saveTempGCodeWithImage(QImage& image);

		QString tempGCodeFileName();
        QString tempGcodeThumbnail();
		QString tempGCodeImageFileName();
		QString tempImageFileName();
	protected:
		SimpleGCodeBuilder builder;
		SliceResultPointer m_result;

		Qt3DRender::QGeometry* m_cache;
		Qt3DRender::QAttribute* m_attribute;

		SliceAttainType m_type;

		QString m_tempGCodeFileName;
		QString m_tempGCodeImageFileName;

		QList<int> m_stepGCodesMap;

		QString m_slicePath;
		QString m_tempPath;
	};
}

#endif // _NULLSPACE_SLICEATTAIN_1590249988869_H
