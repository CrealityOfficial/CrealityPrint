#ifndef _NULLSPACE_ANSYCSLICER_1591673316708_H
#define _NULLSPACE_ANSYCSLICER_1591673316708_H
#include "crslice/gcode/sliceresult.h"
#include "cxgcode/us/usettings.h"
#include "us/usettings.h"
#include "qtusercore/module/progressor.h"
#include "qtusercore/module/progressortracer.h"
#include "crslice/header.h"
#include "crslice/gcode/define.h"
#include "interface/appsettinginterface.h"
#include "crslice2/crscene.h"
#include "sliceattain.h"

namespace creative_kernel
{
	//enum class Engine_type {
	//	engine_cura,
	//	engine_orca	
	//};

	SettingsPtr convert(const cxgcode::USettings& settings);
	QString generateTempFileName();
	QString sceneTempFile();
	SliceResultPointer generateResult(const QString& fileName, qtuser_core::ProgressorTracer& tracer);

	class ModelGroupInput;
	class ModelN;
	class Printer;
	class SliceInput : public QObject
	{
	public:
		SliceInput(QObject* parent = nullptr);
		~SliceInput();

		bool canSlice();
		trimesh::box3 sceneBox();
		bool hasModel();
		QList<ModelGroupInput*> Groups;
		SettingsPointer G;
		QList<SettingsPointer> Es;
		std::vector<crslice2::ThumbnailData> pictures;
		crslice2::PlateInfo plate;
	};

	class AnsycSlicer : public QObject
	{
	public:
		AnsycSlicer(QObject* parent = nullptr);
		virtual ~AnsycSlicer();

		virtual SliceResultPointer doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, SliceAttain* _fDebugger = nullptr);

		virtual void stop();

	protected:
	};

	void produceSliceInput(SliceInput& input, Printer* printer, EngineType engine_type);

	void cacheInput(const SliceInput& input);
}
#endif // _NULLSPACE_ANSYCSLICER_1591673316708_H
