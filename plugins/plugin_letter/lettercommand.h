#pragma once
#include "qtusercore/plugin/toolcommand.h"
#include <QObject>
#include <QVector3D>
#include "cxkernel/wrapper/lettermodel.h"

namespace creative_kernel
{
	class MoveOperateMode;
}

class LetterCommand : public ToolCommand
{
	Q_OBJECT
	Q_PROPERTY(QStringList font_list READ getFontList NOTIFY onFontListChanged)
	Q_PROPERTY(float text_thickness READ getThickness WRITE setThickness NOTIFY onThicknessChanged)
	Q_PROPERTY(int text_side READ getTextSide WRITE setTextSide NOTIFY onTextSideChanged)

public:
	explicit LetterCommand(QObject *parent = nullptr);
	~LetterCommand();
	Q_INVOKABLE void execute();
	Q_INVOKABLE bool checkSelect() override;
	Q_INVOKABLE void generatePolygonData(const QList<QObject*>& objectList);


protected:
	QStringList getFontList();
	float getThickness();
	void setThickness(float the_thickness);
	int getTextSide();
	void setTextSide(int the_text_side);

	cxkernel::LetterModel *m_model;
	QStringList m_listFonts;
	creative_kernel::MoveOperateMode* m_mode { NULL };

signals:
	void onFontListChanged();
	void onThicknessChanged();
	void onTextSideChanged();
};