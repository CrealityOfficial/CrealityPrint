#include "outlinegenerator.h"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtCore/QFile>
#include <QtQml/QQmlProperty>
#include <QtCore/QDebug>
#include <QtGui/QMatrix>
#include "fontmeshwrapper.h"

#include "topomesh/interface/poly.h"

OutlineGenerator::OutlineGenerator(QObject* parent)
	: QObject(parent)
{
}

OutlineGenerator::OutlineGenerator(const OutlineGenerator& other)
{
	m_text = other.m_text;
	m_font = other.m_font;
	m_fontSize = other.m_fontSize;
	m_fontFamily = other.m_fontFamily;
	m_bold = other.m_bold;
	m_italic = other.m_italic;
	m_lineSpace = other.m_lineSpace;
	m_wordSpace = other.m_wordSpace;

	m_outlines = other.m_outlines;

	m_isDirty = other.m_isDirty;
	m_distanceChanged = other.m_distanceChanged;
}

OutlineGenerator::~OutlineGenerator()
{

}

void OutlineGenerator::applyFontConfig(FontConfig config)
{
	QString text(config.text.data());
	if (m_text != text)
	{
		m_text = text;
		m_isDirty = true;
	}

	QString font(config.font.data());
	if (m_fontFamily != font)
	{
		m_fontFamily = font;
		m_isDirty = true;
	}

	if (m_fontSize != config.fontSize)
	{
		m_fontSize = config.fontSize;
		m_isDirty = true;
	}
	
	if (m_bold != config.bold)
	{
		m_bold = config.bold;
		m_isDirty = true;
	}

	if (m_italic != config.italic)
	{
		m_italic = config.italic;
		m_isDirty = true;
	}

	if (m_lineSpace != config.lineSpace)
	{
		m_lineSpace = config.lineSpace;
		m_isDirty = true;
	}

	if (m_wordSpace != config.wordSpace)
	{
		m_wordSpace = config.wordSpace;
		m_isDirty = true;
	}

}

bool OutlineGenerator::canGenerate()
{
	return !m_text.isEmpty();
}

void OutlineGenerator::setText(const QString& text)
{
	if (m_text == text)
		return;
		
	m_isDirty = true;
	m_text = text;
}

void OutlineGenerator::setFontSize(int px)
{
	if (m_fontSize == px)
		return;
		
	m_isDirty = true;
	m_fontSize = px;
}

void OutlineGenerator::setFontFamily(const QString& fontFamily)
{
	if (m_fontFamily == fontFamily)
		return;
		
	m_isDirty = true;
	m_fontFamily = fontFamily;
}

void OutlineGenerator::setBold(bool bold)
{
	if (m_bold == bold)
		return;
		
	m_isDirty = true;
	m_bold = bold;
}

void OutlineGenerator::setItalic(bool italic)
{
	if (m_italic == italic)
		return;
		
	m_isDirty = true;
	m_italic = italic;
}

void OutlineGenerator::setLineSpace(int lineSpace)
{
	if (m_lineSpace == lineSpace)
		return;

	m_isDirty = true;
	m_lineSpace = lineSpace;
}

void OutlineGenerator::setWordSpace(int wordSpace)
{
	if (m_wordSpace == wordSpace)
		return;

	m_wordSpace = wordSpace;
	m_isDirty = true;
}

void OutlineGenerator::generate()
{
	m_outlines.clear();

	std::vector<OutLine> result;
	QFont font;
	font.setPixelSize(m_fontSize);
	if (!m_fontFamily.isEmpty())
		font.setFamily(m_fontFamily);
	font.setBold(m_bold);
	font.setItalic(m_italic);
	font.setLetterSpacing(QFont::AbsoluteSpacing, m_wordSpace);

	QFontMetrics metrics(font);
	int fontHeight = metrics.height();
	int y = 0;
	QList<QPair<QPainterPath, QList<QPainterPath>>> lineAndWordList;
	QPair<QPainterPath, QList<QPainterPath>> lineAndWord;
	int lineStart = 0;
	for (int i = 0; i < m_text.length(); ++i)
	{
		QString ch = m_text.mid(i, 1);

		if (ch == "\n")
		{
			if (!lineAndWord.second.isEmpty())
			{
				QString lineString = m_text.mid(lineStart, i - lineStart);
				QPainterPath painterPath;
				painterPath.addText(0, y, font, lineString);
				lineAndWord.first = painterPath;
				lineAndWordList << lineAndWord;

				lineAndWord.first = QPainterPath();
				lineAndWord.second.clear();
			}

			y += (fontHeight + m_lineSpace);
			lineStart = i + 1;
		}
		else
		{
			QPainterPath painterPath;
			painterPath.addText(0, 0, font, ch);
			lineAndWord.second << painterPath;
		}
	}

	if (!lineAndWord.second.isEmpty())
	{
		QString lineString = m_text.mid(lineStart);
		QPainterPath painterPath;
		painterPath.addText(0, y, font, lineString);
		lineAndWord.first = painterPath;
		lineAndWordList << lineAndWord;

		lineAndWord.first = QPainterPath();
		lineAndWord.second.clear();
	}

	QList<OutLine> tempOutlines;

	for (auto lineAndWord : lineAndWordList)
	{
		QList<QPolygonF> linePolys = lineAndWord.first.toSubpathPolygons();
		int linePolysSize = linePolys.size();
		int charIndex = 0;
		for (QPainterPath charPath : lineAndWord.second)
		{
			QList<QPolygonF> charPolys = charPath.toSubpathPolygons();
			int charSize = charPolys.size();
			charPolys.clear();
			for (int i = charIndex, end = charIndex + charSize; i < end; ++i)
			{
				if (i < linePolysSize)
					charPolys << linePolys[i];
			}

			OutLine charOutline = parseOutLine(charPolys);
			m_outlines.push_back(charOutline);

			charIndex += charSize;
		}
		//m_outlines.push_back(lineOutline);
	}
}

std::vector<OutLine> OutlineGenerator::result()
{
	return m_outlines;
}

bool OutlineGenerator::isDirty()
{
	return m_isDirty;
}

void OutlineGenerator::resetDirty()
{
	m_isDirty = false;
}

OutLine OutlineGenerator::parseOutLine(const QList<QPolygonF>& polys)
{
	std::vector<std::vector<trimesh::vec3>> outline;
	int size = polys.size();
	if (size > 0)
	{
		outline.resize(size);
		for (int i = 0; i < size; ++i)
		{
			const QPolygonF& poly = polys.at(i);
			std::vector<trimesh::vec3>& path = outline.at(i);

			int count = poly.size();
			for (int j = 0; j < count; ++j)
			{
				const QPointF& point = poly.at(j);
				path.push_back(trimesh::vec3(point.x(), point.y(), 0));
			}
		}
	}

	topomesh::simplifyPolygons(outline);

	std::vector<std::vector<trimesh::vec2>> result;
	result.resize(outline.size());
	for (int i = 0, count = outline.size(); i < count; ++i)
	{
		auto& opl = outline[i];
		result[i].resize(opl.size());
		for (int j = 0, size = opl.size(); j < size; ++j)
		{
			result[i][j] = trimesh::vec2(opl[j][0], opl[j][1]);
		}
	}
	 return result;
}
