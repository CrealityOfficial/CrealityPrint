#pragma once
#include "qtuserqml/plugin/toolcommand.h"
#include <QObject>
#include <QVector3D>

class LetterOp;

class LetterCommand : public ToolCommand
{
	Q_OBJECT
	Q_PROPERTY(QStringList font_list READ getFontList NOTIFY onFontListChanged)
	Q_PROPERTY(int cur_font_index READ getCurFontIndex WRITE setCurFontIndex NOTIFY onCurFontIndexChanged)
	Q_PROPERTY(float height READ getHeight WRITE setHeight NOTIFY onHeightChanged)
	Q_PROPERTY(float text_thickness READ getThickness WRITE setThickness NOTIFY onThicknessChanged)
	Q_PROPERTY(QString text READ getText WRITE setText NOTIFY onTextChanged)
	Q_PROPERTY(int text_side READ getTextSide WRITE setTextSide NOTIFY onTextSideChanged)

public:
	explicit LetterCommand(QObject *parent = nullptr);
	~LetterCommand();
	Q_INVOKABLE void execute();
	Q_INVOKABLE void startLetter();
	Q_INVOKABLE void endLetter();
	Q_INVOKABLE bool checkSelect() override;
	Q_INVOKABLE void loadFont();

	Q_INVOKABLE void accept();
	Q_INVOKABLE void cancel();

	void setMessage(bool isRemove);
	bool message();

protected:
	QStringList getFontList();
	int getCurFontIndex();
	void setCurFontIndex(int cur_font_index);
	float getHeight();
	void setHeight(float the_height);
	float getThickness();
	void setThickness(float the_thickness);
	QString getText();
	void setText(QString the_text);
	int getTextSide();
	void setTextSide(int the_text_side);

	//QStringList supportFilters() override;
	//void handle(const QString& fileName) override;
	//void handle(const QStringList& fileNames) override;

	LetterOp* m_pOp;
	QStringList m_listFonts;
private  slots:
	void slotMouseLeftClicked();

signals:
	void onFontListChanged();
	void onCurFontIndexChanged();
	void onHeightChanged();
	void onThicknessChanged();
	void onTextChanged();
	void onTextSideChanged();
};