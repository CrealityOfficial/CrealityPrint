#ifndef _NULLSPACE_SLICEPREVIEWFLOW_1589874729758_H
#define _NULLSPACE_SLICEPREVIEWFLOW_1589874729758_H

#include <memory>

#include <QtCore/QAbstractListModel>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <Qt3DRender/QCamera>

#include <crslice/gcode/define.h>
#include <cxgcode/model/gcodeextruderlistmodel.h>
#include <cxgcode/model/gcodeextrudertablemodel.h>
#include <cxgcode/model/gcodefanspeedlistmodel.h>
#include <cxgcode/model/gcodeflowlistmodel.h>
#include <cxgcode/model/gcodelayerhightlistmodel.h>
#include <cxgcode/model/gcodelayertimelistmodel.h>
#include <cxgcode/model/gcodelinewidthlistmodel.h>
#include <cxgcode/model/gcodespeedlistmodel.h>
#include <cxgcode/model/gcodestructurelistmodel.h>
#include <cxgcode/model/gcodetemperaturelistmodel.h>

#include <qtuser3d/event/eventhandlers.h>
#include <qtuser3d/framegraph/rendergraph.h>
#include <qtuser3d/framegraph/colorpicker.h>

#include "external/data/kernelenum.h"

namespace qtuser_3d
{
	class Surface;
	class TextureRenderTarget;
	class Selector;
}

namespace creative_kernel
{
	class ModelN;
	class SliceAttain;
	class SlicePreviewScene;
	class Printer;
	class SlicePreviewFlow : public QObject
	{
		Q_OBJECT;
		Q_PROPERTY(int currentLayer READ currentLayer NOTIFY stepsChanged);
		Q_PROPERTY(int layers READ layers NOTIFY layersChanged);
		Q_PROPERTY(int steps READ steps NOTIFY stepsChanged);
		Q_PROPERTY(int stepsMax READ stepsMax NOTIFY stepsMaxChanged);
		Q_PROPERTY(float height READ height NOTIFY layerGCodesChanged);
		Q_PROPERTY(QStringList layerGCodes READ layerGCodes NOTIFY layerGCodesChanged);
		Q_PROPERTY(float currentStepSpeed READ currentStepSpeed NOTIFY currentStepSpeedChanged);
		Q_PROPERTY(QVector3D position READ position NOTIFY positionChanged);
		Q_PROPERTY(float layerHeight READ layerHeight NOTIFY layerHeightChanged);
		Q_PROPERTY(int gCodeVisualType READ gCodeVisualType NOTIFY gCodeVisualTypeChanged);
		Q_PROPERTY(float temperature READ temperature NOTIFY temperatureChanged);
		Q_PROPERTY(float acc READ acc NOTIFY accChanged);
		Q_PROPERTY(float lineWidth READ lineWidth NOTIFY lineWidthChanged);
		Q_PROPERTY(float flow READ flow NOTIFY flowChanged);
		Q_PROPERTY(float layerTime READ layerTime NOTIFY layerTimeChanged);
		Q_PROPERTY(float fanSpeed READ fanSpeed NOTIFY fanSpeedChanged);
		Q_PROPERTY(float speed READ speed NOTIFY speedChanged);

		Q_PROPERTY(QAbstractListModel* speedModel READ getSpeedModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* structureModel READ getStructureModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* extruderModel READ getExtruderModel CONSTANT);
		Q_PROPERTY(QAbstractTableModel* extruderTableModel READ getExtruderTableModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* layerHightModel READ getLayerHightModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* lineWidthModel READ getLineWidthModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* flowModel READ getFlowModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* layerTimeModel READ getLayerTimeModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* fanSpeedModel READ getFanSpeedModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* temperatureModel READ getTemperatureModel CONSTANT);
		Q_PROPERTY(QAbstractListModel* accModel READ getAccModel CONSTANT);
        Q_PROPERTY(bool indicatorVisible READ indicatorVisible WRITE setIndicatorVisible NOTIFY indicatorVisibleChanged)

	public:
		SlicePreviewFlow(Qt3DCore::QNode* parent = nullptr);
		virtual ~SlicePreviewFlow();

		void registerContext();
		void initialize();
		SlicePreviewScene* scene();

		void useCachePreview();

		void notifyClipValue();

		Q_INVOKABLE void setGCodeVisualType(int type);
		// Q_INVOKABLE void setIndicatorVisible(bool visible);
		// Q_INVOKABLE void setPrinterVisible(bool visible);
		Q_INVOKABLE void showGCodeType(int type, bool show);

		Q_INVOKABLE void setCurrentLayer(int layer, bool randonStep);
		Q_INVOKABLE void setCurrentStep(int step);
		Q_INVOKABLE int findViewIndexFromStep(int nStep);
		Q_INVOKABLE int findStepFromViewIndex(int nViewIndex);

		Q_INVOKABLE void showOnlyLayer(int layer); // layer = -1 reset
		Q_INVOKABLE void setAnimationState(int state);
		Q_INVOKABLE void setCurrentLayerFocused(bool focused);

		Q_INVOKABLE void setNozzleColorList(const QVariantList& list);

		QAbstractListModel* getSpeedModel() const;
		QAbstractListModel* getStructureModel() const;
		QAbstractListModel* getExtruderModel() const;
		QAbstractTableModel* getExtruderTableModel() const;
		QAbstractListModel* getLayerHightModel() const;
		QAbstractListModel* getLineWidthModel() const;
		QAbstractListModel* getFlowModel() const;
		QAbstractListModel* getLayerTimeModel() const;
		QAbstractListModel* getFanSpeedModel() const;
		QAbstractListModel* getTemperatureModel() const;
		QAbstractListModel* getAccModel() const;

		bool isAvailable();

		int currentLayer();
		int layers();
		int steps();
		int stepsMax();
		float height();
		QStringList layerGCodes();
		float currentStepSpeed();
		void clear();
		QVector3D position();
		void setPosition();
		float layerHeight();
		void setLayerHeight();

		float temperature();
		void setTemperature();
		float acc();
		void setAcc();
		float lineWidth();
		void setLineWidth();
		float flow();
		void setFlow();
		float layerTime();
		void setLayerTime();
		float fanSpeed();
		void setFanSpeed();
		float speed();
		void setSpeed();


		int gCodeVisualType();

		Q_INVOKABLE void addCustomGcode(const QString& gcode);
		Q_INVOKABLE void delCustomGcode();
		Q_INVOKABLE void saveCustomGcode();

		void setSliceAttain(SliceAttain* attain);
		void previewSliceAttain(SliceAttain* attain, int layer);
		SliceAttain* attain();
		SliceAttain* takeAttain();

        bool indicatorVisible();  
		Q_INVOKABLE void setIndicatorVisible(bool visible);

		bool isAttainDisplayCompletly();

	signals:
		void layersChanged();
		void stepsChanged();
		void stepsMaxChanged();
		void layerGCodesChanged();
		void currentStepSpeedChanged();
        void indicatorVisibleChanged();
		void positionChanged();
		void layerHeightChanged();
		void gCodeVisualTypeChanged();
		void temperatureChanged();
		void accChanged();
		void lineWidthChanged();
		void flowChanged();
		void layerTimeChanged();
		void fanSpeedChanged();
		void speedChanged();
		
	private:
		void setExtruderTableModel();

	private:
		//QScopedPointer<SliceAttain> m_attain;
		Printer* m_printer { NULL };
		SliceAttain* m_attain{ NULL };
		SlicePreviewScene* m_scene;
		int m_indexOffset;
		int m_currentLayer;
		int m_lastTotalLayers{-1};
		int m_currentStep;
		bool m_focused=false;
		QVector3D m_position;
		float m_layerHeight;
		int m_gCodeVisualType;
		QImage m_cachePreview;
		float m_temperature;
		float m_acc;
		float m_lineWidth;
		float m_flow;
		float m_layerTime;
		float m_fanSpeed;
		float m_speed;
		int m_stepsMax;

		gcode::GCodeVisualType m_visualType;
		std::unique_ptr<cxgcode::GcodeSpeedListModel> m_speedModel;
		std::unique_ptr<cxgcode::GcodeStructureListModel> m_structureModel;
		std::unique_ptr<cxgcode::GcodeExtruderListModel> m_extruderModel;
		std::unique_ptr<cxgcode::GcodeExtruderTableModel> m_extruderTableModel;
		std::unique_ptr<cxgcode::GcodeLayerHightListModel> m_layerHightModel;
		std::unique_ptr<cxgcode::GcodeLineWidthListModel> m_lineWidthModel;
		std::unique_ptr<cxgcode::GcodeFlowListModel> m_flowModel;
		std::unique_ptr<cxgcode::GcodeLayerTimeListModel> m_layerTimeModel;
		std::unique_ptr<cxgcode::GcodeFanSpeedListModel> m_fanSpeedModel;
		std::unique_ptr<cxgcode::GcodeTemperatureListModel> m_temperatureModel;
		std::unique_ptr<cxgcode::GcodePreviewRangeDivideModel> m_accModel;


		QScopedPointer<Qt3DRender::QCamera> m_previewCamera;
        
        bool m_indicatorVisible  { false };  
        
	};
}
#endif // _NULLSPACE_SLICEPREVIEWFLOW_1589874729758_H
