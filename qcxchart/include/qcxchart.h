#ifndef _QCXCHART_H_
#define _QCXCHART_H_

#include <QtQuick>
#include "qcustomplot.h"

#if defined(QCXCHART_DLL)
#  define QCXCHART_API Q_DECL_EXPORT
#else
#  define QCXCHART_API Q_DECL_IMPORT
#endif

class QCXCHART_API QCxChart : public QQuickPaintedItem
{
    Q_OBJECT
        
    Q_PROPERTY(int graphCount READ graphCount WRITE setGraphCount)// NOTIFY graphCountChanged
    Q_PROPERTY(QVariantList penColors READ penColors WRITE setPenColors)// NOTIFY penColorsChanged
    Q_PROPERTY(QVariantList fillColors READ fillColors WRITE setFillColors)// NOTIFY fillColorsChanged
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor gridLineColor READ gridLineColor WRITE setGridLineColor NOTIFY gridLineColorChanged)
	Q_PROPERTY(QColor labelsColor READ labelsColor WRITE setLabelsColor NOTIFY labelsColorChanged)
	Q_PROPERTY(QFont labelsFont READ labelsFont WRITE setLabelsFont)// NOTIFY labelsFontChanged
 
public:
    QCxChart( QQuickItem* parent = 0 );
    virtual ~QCxChart();
 
    void paint( QPainter* painter );
 
    void initCustomPlot();
    Q_INVOKABLE void setupCustomPlot(QVariantList initData);
    Q_INVOKABLE void addData(QVariantList data, int msecOffset = 0);

    int graphCount() const;
    void setGraphCount(const int& graphCount);

    QVariantList penColors() const;
    void setPenColors(QVariantList penColors);

    QVariantList fillColors() const;
    void setFillColors(QVariantList fillColors);

	QColor labelsColor() const;
	void setLabelsColor(const QColor& color);

    QColor gridLineColor() const;
    void setGridLineColor(const QColor& gridLineColor);

	QColor backgroundColor() const;
	void setBackgroundColor(const QColor& backgroundColor);

	QFont labelsFont() const;
	void setLabelsFont(const QFont& font);
  
private:
    QCustomPlot* m_CustomPlot;

    int m_graphCount = 1;
    QVariantList m_penColors;
    QVariantList m_fillColors;
    QColor m_backgroundColor;
	QColor m_gridLineColor;
    QColor m_labelsColor;
	QFont m_labelsFont;

signals:
	//void graphCountChanged();
	//void penColorsChanged();
	//void fillColorsChanged();
	void backgroundColorChanged();
	void gridLineColorChanged();
	void labelsColorChanged();
	//void labelsFontChanged();
 
private slots:
    void onCustomReplot();
    void updateCPSize();
	void updateCPBackgroundColor();
	void updateCPGridLineColor();
	void updateCPLabelsColor();
};

#endif // _QCXCHART_H_
