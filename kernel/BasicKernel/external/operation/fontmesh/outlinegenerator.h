#ifndef OUTLINEGENERATOR_H
#define OUTLINEGENERATOR_H
#include "cxkernel/data/header.h"
#include <QtCore/QObject>
#include <memory>
#include <QFont>
#include <QMatrix>

struct FontConfig;

typedef std::vector<std::vector<trimesh::vec2>> OutLine;

class OutlineGenerator : public QObject
{
	Q_OBJECT
public:
	OutlineGenerator(QObject* parent = nullptr);
	OutlineGenerator(const OutlineGenerator& other);
	virtual ~OutlineGenerator();

	void applyFontConfig(FontConfig config);
	bool canGenerate();

	void setText(const QString& text);
	void setFontSize(int px);
	void setFontFamily(const QString& fontFamily);
	void setBold(bool bold);
	void setItalic(bool italic);
	void setLineSpace(int lineSpace);
	void setWordSpace(int wordSpace);
	//void set

	void generate();	

	std::vector<OutLine> result();

	bool isDirty();
	void resetDirty();

private:
	OutLine parseOutLine(const QList<QPolygonF>& polys);

private:
	QString m_text;
	QFont m_font;
	int m_fontSize{20};
    QString m_fontFamily { QStringLiteral("Arial") };
	bool m_bold { false };
	bool m_italic { false };
	int m_lineSpace{0};
	int m_wordSpace{0};

	std::vector<OutLine> m_outlines;

	bool m_isDirty {false};
	bool m_distanceChanged { false };

};

#endif // OUTLINEGENERATOR_H
