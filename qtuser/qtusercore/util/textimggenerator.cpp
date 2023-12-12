#include "textimggenerator.h"

#include <QPainter>
#include <QImage>
#include <QPixmap>

using namespace qtuser_core;


qtuser_core::TextImgGenerator::TextImgGenerator(QObject* parent)
	: QObject(parent)
	, m_wrapFlag(false)
	, m_verticalFlag(false)
	, m_alignFlag(Qt::AlignCenter)
	, m_imgWidth(-1)
	, m_imgHeight(-1)
	, m_foreColor(Qt::white)
	, m_backColor(Qt::black)
	, m_text("")
{
}

qtuser_core::TextImgGenerator::~TextImgGenerator()
{
}

void qtuser_core::TextImgGenerator::setText(QString text)
{
	m_text = text;
}

void qtuser_core::TextImgGenerator::setFont(QFont font)
{
	m_font = font;
}

void qtuser_core::TextImgGenerator::setAlignFlag(int flag)
{
	m_alignFlag = flag;
}

void qtuser_core::TextImgGenerator::setTextWrap(bool wrap)
{
	m_wrapFlag = wrap;
}

void qtuser_core::TextImgGenerator::setTextVerticalFlag(bool flag)
{
	m_verticalFlag = flag;
	m_wrapFlag = true;
}

void qtuser_core::TextImgGenerator::setImgWidth(int width)
{
	m_imgWidth = width;
}

void qtuser_core::TextImgGenerator::setImgHeight(int height)
{
	m_imgHeight = height;
}

void qtuser_core::TextImgGenerator::setForeColor(QColor color)
{
	m_foreColor = color;
}

void qtuser_core::TextImgGenerator::setBackColor(QColor color)
{
	m_backColor = color;
}

QImage qtuser_core::TextImgGenerator::generateImage()
{
	int fontSize = m_font.pixelSize();
	QRect textRect = m_verticalFlag ? QRect(0, 0, fontSize, (fontSize + 10) * m_text.length()) : QRect(0, 0, fontSize * m_text.length(), fontSize);

	QPixmap panel = QPixmap(textRect.width(), textRect.height());
	panel.fill(m_backColor);

	QPainter painter(&panel);
	painter.setFont(m_font);
	painter.setPen(QPen(m_foreColor, 10));

	QRect boundingRect;
	painter.drawText(textRect, m_alignFlag | (m_wrapFlag ? Qt::TextWordWrap : 0), m_text, &boundingRect);

	return panel.toImage();
}
