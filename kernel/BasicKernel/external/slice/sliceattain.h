#ifndef _NULLSPACE_SLICEATTAIN_1590249988869_H
#define _NULLSPACE_SLICEATTAIN_1590249988869_H
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include "data/kernelenum.h"

#include "crslice/gcode/sliceresult.h"
#include "cxgcode/simplegcodebuilder.h"

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>

#include "qtuser3d/math/box3d.h"

#include <unordered_map>
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"

#include <crslice/gcode/define.h>
#include "crslice/header.h"

namespace creative_kernel
{
	enum class SliceAttainType
	{
		sat_slice,
		sat_file
	};

	class SliceAttain : public QObject, public gcode::GcodeTracer
	{
		Q_OBJECT
	public:
		SliceAttain(SliceResultPointer result, SliceAttainType type, QObject* parent = nullptr);
		virtual ~SliceAttain();

		void build(ccglobal::Tracer* tracer = nullptr);
		void build_paraseGcode(ccglobal::Tracer* tracer = nullptr);
		void preparePreviewData(ccglobal::Tracer* tracer = nullptr);

		Q_INVOKABLE QString sliceName();
		Q_INVOKABLE QString sourceFileName();
        Q_INVOKABLE bool isFromFile();

		Q_INVOKABLE int printTime();
		Q_INVOKABLE QString material_weight();
		Q_INVOKABLE QString printing_time();
		Q_INVOKABLE QString material_money();
		Q_INVOKABLE QString material_length();
		const QString layerGcode(int layer);

		trimesh::box3 box();
		Q_INVOKABLE int layers();
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

		Q_INVOKABLE float layerHeight(int layer);
		float layerHeight();
		float lineWidth();

		gcode::TimeParts getTimeParts() const;
		std::vector<std::pair<int,float>> getTimeParts_orca() ;
		bool isCuraProducer();

		Q_INVOKABLE QVariantList getNozzleColorList();
		void getFilamentsList(std::vector<std::pair<int,double>>& volumes_per_extruder, std::vector<std::pair<int, double>>& flush_per_filament, std::vector<std::pair<int, double>>& volumes_per_tower);
		int get_filamentchanges();

		void getDataFromOrca();

		QImage* getImageFromGcode();
		QString fileNameFromGcode();

		float traitSpeed(int layer, int step);
		trimesh::vec3 traitPosition(int layer, int step);
		float traitDuration(int layer, int step);

		trimesh::fxform modelMatrix();

		int findViewIndexFromStep(int layer, int nStep);
		int findStepFromViewIndex(int layer, int nViewIndex);
		void updateStepIndexMap(int layer);

        float getMachineHeight();
        float getMachineWidth();
        float getMachineDepth();
        int getBeltType();

		//fdm
		void prepareVisualTypeData(gcode::GCodeVisualType type);
		Qt3DRender::QGeometry* createGeometry();
		Qt3DRender::QGeometry* createRetractionGeometry();
		Qt3DRender::QGeometry* createZSeamsGeometry();
		Qt3DRender::QGeometry* createUnretractGeometry();

		Qt3DRender::QGeometry* rebuildGeometryVisualTypeData();

		Qt3DRender::QGeometryRenderer* createRetractionGeometryRenderer();
		Qt3DRender::QGeometryRenderer* createZSeamsGeometryRenderer();

        void saveGCode(const QString& fileName, QImage* previewImage);
		void saveTempGCode();
		void saveTempGCodeWithImage(QImage& image);

		QString tempGCodeFileName();
        QString tempGcodeThumbnail();
		QString tempGCodeImageFileName();
		QString tempImageFileName();

		void tick(const std::string& tag) {};
		void getPathData(const trimesh::vec3 point, float e, int type, bool isSeam = false,bool isOrca = false)override;
		void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type,int p, bool isG2 = true, bool isOrca = false, bool isseam = false) override;
		void setParam(gcode::GCodeParseInfo& pathParam)override;
		void setLayer(int layer)override;
		void setLayers(int layer)override;
		void setSpeed(float s)override;
		void setAcc(float acc)override;
		void setTEMP(float temp)override;
		void setExtruder(int nr)override;
		void setFan(float fan)override;
		void setZ(float z, float h = -1)override;
		void setE(float e)override;
		void setWidth(float width) override;
		void setLayerHeight(float height) override;
		void setLayerPause(int pause) override;
		void setTime(float time)override;
		void getNotPath()override;
		void set_data_gcodelayer(int layer, const std::string& gcodelayer)override;//set a layer data
		void setNozzleColorList(std::string& colorList)override;
		void writeImages(const std::vector<std::pair<trimesh::ivec2, std::vector<unsigned char>>>& images)override;

		void onSupports(int layerIdx, float z, float thickness, const std::vector<std::vector<trimesh::vec3>>& paths)override;
		void setSceneBox(const trimesh::box3& box)override;

		qtuser_3d::Box3D getGCodePathBoundingBox();
	protected:
		cxgcode::SimpleGCodeBuilder builder;
		SliceResultPointer m_result;

		Qt3DRender::QGeometry* m_cache;
		//Qt3DRender::QAttribute* m_attribute;

		SliceAttainType m_type;

		QString m_tempGCodeFileName;
		QString m_tempGCodeImageFileName;
	};
}

#endif // _NULLSPACE_SLICEATTAIN_1590249988869_H
