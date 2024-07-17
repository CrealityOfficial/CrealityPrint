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
	class SlicePreviewFlow : public qtuser_3d::RenderGraph
		, public qtuser_3d::KeyEventHandler
		, public qtuser_3d::HoverEventHandler
		, public qtuser_3d::LeftMouseEventHandler
	{
		Q_OBJECT;
		Q_PROPERTY(int currentLayer READ currentLayer NOTIFY stepsChanged);
		Q_PROPERTY(int layers READ layers NOTIFY layersChanged);
		Q_PROPERTY(int steps READ steps NOTIFY stepsChanged);
		Q_PROPERTY(float height READ height NOTIFY layerGCodesChanged);
		Q_PROPERTY(QStringList layerGCodes READ layerGCodes NOTIFY layerGCodesChanged);
		Q_PROPERTY(float currentStepSpeed READ currentStepSpeed NOTIFY currentStepSpeedChanged);

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
        Q_PROPERTY(bool indicatorVisible READ indicatorVisible WRITE setIndicatorVisible NOTIFY indicatorVisibleChanged)

	public:
		SlicePreviewFlow(Qt3DCore::QNode* parent = nullptr);
		virtual ~SlicePreviewFlow();

		void registerContext();
		void initialize();
		SlicePreviewScene* scene();

		void useCachePreview();

		/* 截取预览图 */
		void beginCapturePreview();
		void capturePrinter(Printer* printer, qtuser_3d::RenderCaptor::ReceiverHandleReplyFunc func);
		void endCapturePreview();


		// void requestPreview(const QList<ModelN*>& group, qtuser_3d::namedReplyFunc func);
		// void endRequest(QImage image);

		void notifyClipValue();

		void setSceneClearColor(const QColor& color);

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

		bool isAvailable();

		int currentLayer();
		int layers();
		int steps();
		float height();
		QStringList layerGCodes();
		float currentStepSpeed();
		void clear();
		Q_INVOKABLE void addCustomGcode(const QString& gcode);
		Q_INVOKABLE void delCustomGcode();
		Q_INVOKABLE void saveCustomGcode();

		void setPrinter(Printer* printer);
		void setSliceAttain(SliceAttain* attain);
		void previewSliceAttain(SliceAttain* attain, int layer);
		SliceAttain* attain();
		SliceAttain* takeAttain();
		
		qtuser_3d::ColorPicker* colorPicker();

        bool indicatorVisible();  
        void setIndicatorVisible(bool visible); 

		bool isAttainDisplayCompletly();

	signals:
		void layersChanged();
		void stepsChanged();
		void layerGCodesChanged();
		void currentStepSpeedChanged();
        void indicatorVisibleChanged();

	protected slots:
		void requestCapture(bool capture);

	protected:
		Qt3DCore::QEntity* sceneGraph() override;
		void updateRenderSize(const QSize& size) override;

		void begineRender() override;
		void endRender() override;
#ifdef DEBUG
		bool showDebugOverlay() override;
		void setShowDebugOverlay(bool showDebugOverlay) override;
#endif
	protected:

		void onKeyPress(QKeyEvent* event) override;
		void onKeyRelease(QKeyEvent* event) override;

		void onHoverEnter(QHoverEvent* event) override;
		void onHoverLeave(QHoverEvent* event) override;
		void onHoverMove(QHoverEvent* event) override;

		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;
		
	private:
		void setExtruderTableModel();

	private:
		//QScopedPointer<SliceAttain> m_attain;
		Printer* m_printer { NULL };
		SliceAttain* m_attain{ NULL };
		SlicePreviewScene* m_scene;
		int m_indexOffset;
		int m_currentLayer;
		int m_lastLayersCount{-1};
		int m_currentStep;
		bool m_focused=false;
		qtuser_3d::Surface* m_surface;
		qtuser_3d::ColorPicker* m_colorPicker;
		qtuser_3d::TextureRenderTarget* m_textureRenderTarget;

		QImage m_cachePreview;

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

		QScopedPointer<Qt3DRender::QCamera> m_previewCamera;
		qtuser_3d::Selector* m_entitySelector;
		QSize m_surfaceSize;
        
        bool m_indicatorVisible  { false };  
        
	};
}
#endif // _NULLSPACE_SLICEPREVIEWFLOW_1589874729758_H
