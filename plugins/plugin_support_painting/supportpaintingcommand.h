#ifndef _NULLSPACE_SUPPORT_PAINTING_COMMAND_1591235079966_H
#define _NULLSPACE_SUPPORT_PAINTING_COMMAND_1591235079966_H
#include "qtusercore/plugin/toolcommand.h"
#include <QObject>
#include <QVector3D>
#include <QTimer>
#include "cxkernel/data/modelndata.h"

class SupportPaintingOperateMode;

class SupportPaintingCommand : public ToolCommand
{
	Q_OBJECT
	Q_PROPERTY(QPoint pos READ pos NOTIFY posChanged)
	Q_PROPERTY(float scale READ scale NOTIFY scaleChanged)
	Q_PROPERTY(float sectionRate READ sectionRate WRITE setSectionRate NOTIFY sectionRateChanged)
	Q_PROPERTY(float colorRadius READ colorRadius WRITE setColorRadius NOTIFY colorRadiusChanged)
	Q_PROPERTY(int colorMethod READ colorMethod WRITE setColorMethod NOTIFY colorMethodChanged)
	Q_PROPERTY(int colorIndex READ colorIndex WRITE setColorIndex NOTIFY colorIndexChanged)
	Q_PROPERTY(QVariantList colorsList READ colorsList WRITE setColorsList NOTIFY colorsListChanged)
	Q_PROPERTY(float highlightOverhangsDeg READ highlightOverhangsDeg WRITE setHighlightOverhangsDeg NOTIFY highlightOverhangsDegChanged)
	Q_PROPERTY(bool overHangsEnable READ overHangsEnable WRITE setOverHangsEnable NOTIFY overHangsEnableChanged)
	Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
	explicit SupportPaintingCommand(QObject *parent = nullptr);
	~SupportPaintingCommand();
	Q_INVOKABLE void execute();
	Q_INVOKABLE void setSupportEnabled(bool enabled);
	Q_INVOKABLE bool checkSelect() override;
	Q_INVOKABLE void resetDirection();
	Q_INVOKABLE void clearAllColor();

protected:
	QPoint pos();

	float scale();

	float sectionRate();
	void setSectionRate(float rate);

	float colorRadius();
	void setColorRadius(float radius);

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

	bool isRunning();

private:
	SupportPaintingOperateMode*m_operateMode;

signals:
	void colorMethodChanged();
	void posChanged();
	void scaleChanged();
	void sectionRateChanged();
	void colorRadiusChanged();
	void colorIndexChanged();
	void colorsListChanged();
	void highlightOverhangsDegChanged();
	void overHangsEnableChanged();
	void isRunningChanged();
};

#endif // _NULLSPACE_SUPPORT_PAINTING_COMMAND_1591235079966_H