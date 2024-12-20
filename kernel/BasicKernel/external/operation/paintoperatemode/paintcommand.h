#ifndef _NULLSPACE_PAINT_COMMAND_1591235079966_H
#define _NULLSPACE_PAINT_COMMAND_1591235079966_H
#include "qtusercore/plugin/toolcommand.h"
#include <QObject>
#include <QVector3D>
#include <QTimer>
#include "cxkernel/data/modelndata.h"
#include "basickernelexport.h"

class PaintOperateMode;

class BASIC_KERNEL_API PaintCommand : public ToolCommand
{
	Q_OBJECT
	Q_PROPERTY(QPoint pos READ pos NOTIFY posChanged)
	Q_PROPERTY(float scale READ scale NOTIFY scaleChanged)
	Q_PROPERTY(float sectionRate READ sectionRate WRITE setSectionRate NOTIFY sectionRateChanged)
	Q_PROPERTY(float colorRadius READ colorRadius WRITE setColorRadius NOTIFY colorRadiusChanged)
	Q_PROPERTY(float screenRadius READ screenRadius NOTIFY screenRadiusChanged)
	Q_PROPERTY(int colorMethod READ colorMethod WRITE setColorMethod NOTIFY colorMethodChanged)
	Q_PROPERTY(int colorIndex READ colorIndex WRITE setColorIndex NOTIFY colorIndexChanged)
	Q_PROPERTY(QVariantList colorsList READ colorsList WRITE setColorsList NOTIFY colorsListChanged)
	Q_PROPERTY(float highlightOverhangsDeg READ highlightOverhangsDeg WRITE setHighlightOverhangsDeg NOTIFY highlightOverhangsDegChanged)
	Q_PROPERTY(bool overHangsEnable READ overHangsEnable WRITE setOverHangsEnable NOTIFY overHangsEnableChanged)
	Q_PROPERTY(float smartFillAngle READ smartFillAngle WRITE setSmartFillAngle NOTIFY smartFillAngleChanged)
	Q_PROPERTY(bool smartFillEnable READ smartFillEnable WRITE setSmartFillEnable NOTIFY smartFillEnableChanged)
	Q_PROPERTY(float heightRange READ heightRange WRITE setHeightRange NOTIFY heightRangeChanged)
	Q_PROPERTY(float gapArea READ gapArea WRITE setGapArea NOTIFY gapAreaChanged)
	Q_PROPERTY(float sphereSize READ sphereSize WRITE setSphereSize NOTIFY sphereSizeChanged)
	Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
	explicit PaintCommand(QObject *parent = nullptr);
	~PaintCommand();
	Q_INVOKABLE void execute();
	Q_INVOKABLE bool checkSelect() override;
	Q_INVOKABLE void resetDirection();
	Q_INVOKABLE void clearAllColor();
	Q_INVOKABLE void setOperateModeEnable(bool enable);

	void setOperateMode(PaintOperateMode* operateMode);

protected:
	void init();

	QPoint pos();

	float scale();

	float sectionRate();
	void setSectionRate(float rate);

	float colorRadius();
	void setColorRadius(float radius);

	float screenRadius();

	int colorMethod();
	void setColorMethod(int method);

	int colorIndex();
	void setColorIndex(int index);

	QVariantList colorsList();
	void setColorsList(const QVariantList& datasList);

	float highlightOverhangsDeg();
	void setHighlightOverhangsDeg(float deg);

	bool overHangsEnable();
	void setOverHangsEnable(bool enable);

	float smartFillAngle();
	void setSmartFillAngle(float angle);

	bool smartFillEnable();
	void setSmartFillEnable(bool enable);

	float heightRange();
	void setHeightRange(float heightRange);

	float gapArea();
	void setGapArea(float gapArea);
	
	float sphereSize();
	void setSphereSize(float size);

	bool isRunning();

protected:
	PaintOperateMode *m_operateMode;

signals:
	void colorMethodChanged();
	void posChanged();
	void scaleChanged();
	void sectionRateChanged();
	void colorRadiusChanged();
	void screenRadiusChanged();
	void colorIndexChanged();
	void colorsListChanged();
	void highlightOverhangsDegChanged();
	void overHangsEnableChanged();
	void smartFillAngleChanged();
	void smartFillEnableChanged();
	void heightRangeChanged();
	void gapAreaChanged();
	void sphereSizeChanged();
	void isRunningChanged();
};

#endif // _NULLSPACE_PAINT_COMMAND_1591235079966_H