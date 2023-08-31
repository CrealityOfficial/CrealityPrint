#include "qtusercore/module/job.h"
#include <QtCore/QThread>
#include<QImage>

#define MAX_S_LEN 32
class DrawObjectModel;
class LaserSettings;
class PlotterSettings;
//class LaserShapeSettings;
class CX2DLib;
class LaserGcodeWorker : public qtuser_core::Job
{
	Q_OBJECT
public:

	LaserGcodeWorker(QObject* parent = nullptr) ;
	virtual ~LaserGcodeWorker() {};

	virtual QString name() { return "GenGcodeWorker"; };
	virtual QString description() { return ""; };
	virtual void failed() {  };                        // invoke from main thread
	virtual void successed(qtuser_core::Progressor* progressor);                     // invoke from main thread
	virtual void work(qtuser_core::Progressor* progressor);    // invoke from worker thread

	
public:
	void setDrawObjModel(DrawObjectModel* objModel) { m_pDrawObjectModel = objModel; }
	void setGenType(QString genType) { m_genType = genType; }
	void setOrigin(QPoint origin) { m_origin = origin; }
	void setLaserSetting(LaserSettings* setting);
	void setPlotterSetting(PlotterSettings* setting);
	void setExporter(CX2DLib* exporter) { m_3in1exporter = exporter; }
	 
signals:
	void genGcodeSuccess(QString gcodeBuf);
private:
	
	DrawObjectModel* m_pDrawObjectModel = nullptr;
	QString m_genType = "laser";// "laser" "plotter"
	QPoint m_origin;
	QString m_gcodeBuf = "";
	LaserSettings* m_pLaserSettings = nullptr;
	PlotterSettings* m_pPlotterSettings = nullptr;
	//LaserShapeSettings* m_pLaserShapeSettings = nullptr;	
	CX2DLib* m_3in1exporter;
	int m_platWidthPx = 660;//900;
	int m_platHeightPx = 660;//900;

};