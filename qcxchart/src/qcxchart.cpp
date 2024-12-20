#include "qcxchart.h"
#include <QDebug>
 
QCxChart::QCxChart(QQuickItem* parent) : QQuickPaintedItem(parent), m_CustomPlot(nullptr)
{
	setFlag(QQuickItem::ItemHasContents, true);
    //setRenderTarget(QQuickPaintedItem::FramebufferObject);
	initCustomPlot();
 
	connect(this, &QQuickPaintedItem::widthChanged, this, &QCxChart::updateCPSize);
	connect(this, &QQuickPaintedItem::heightChanged, this, &QCxChart::updateCPSize);

	connect(this, &QCxChart::backgroundColorChanged, this, &QCxChart::updateCPBackgroundColor);
	connect(this, &QCxChart::gridLineColorChanged, this, &QCxChart::updateCPGridLineColor);
	connect(this, &QCxChart::labelsColorChanged, this, &QCxChart::updateCPLabelsColor);
}
 
QCxChart::~QCxChart()
{
    delete m_CustomPlot;
    m_CustomPlot = nullptr;
}

void QCxChart::paint(QPainter* painter)
{
	if (m_CustomPlot)
	{
		QPixmap picture(boundingRect().size().toSize());
		QCPPainter qcpPainter(&picture);

		//m_CustomPlot->replot();
		m_CustomPlot->toPainter(&qcpPainter);

		painter->drawPixmap(QPoint(), picture);
	}
}
 
void QCxChart::initCustomPlot()
{
    m_CustomPlot = new QCustomPlot();
    m_CustomPlot->setOpenGl(true);
 
	updateCPSize();
	updateCPBackgroundColor();
	updateCPGridLineColor();
	updateCPLabelsColor();

	QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
	timeTicker->setTickCount(10);
	timeTicker->setDateTimeFormat("hh:mm");
	timeTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
	m_CustomPlot->xAxis->setTicker(timeTicker);

	m_CustomPlot->axisRect()->setupFullAxesBox();
	m_CustomPlot->xAxis2->setTicks(false); m_CustomPlot->yAxis2->setTicks(false);
	m_CustomPlot->xAxis->setSubTicks(false); m_CustomPlot->yAxis->setSubTicks(false);
 
	connect(m_CustomPlot, &QCustomPlot::afterReplot, this, &QCxChart::onCustomReplot);
}

void QCxChart::setupCustomPlot(QVariantList initData)
{
    m_CustomPlot->clearPlottables();
    for (int i = 0; i < m_graphCount; i++)
	{
		m_CustomPlot->addGraph();
		m_CustomPlot->graph(i)->setPen(QPen(QBrush(m_penColors[i].value<QColor>()), 2));
        m_CustomPlot->graph(i)->setBrush(QBrush(m_fillColors[i].value<QColor>()));
	}

    auto curTime = QDateTime::currentDateTime();
    auto tempTime = curTime;
    int count = initData.count();
    for (int i = 0; i < m_graphCount; i++)
    {
        curTime = tempTime;
        int start = count / 60 + (count % 60 != 0);
        for (int j = start; j <= 11; j++)
        {
            double key = curTime.addSecs(j * -60.0).toSecsSinceEpoch();
            m_CustomPlot->graph(i)->addData(key, 0);//fill data
        }

        curTime = tempTime;
        if (count > 0)
        {
            for (int j = 0; j < count; ++j)
            {
                double key = curTime.addSecs(j + 1 - count).toSecsSinceEpoch();
                auto dataGroup = initData[j].toList();
                double value = dataGroup[i].toDouble();
                m_CustomPlot->graph(i)->addData(key, value);
            }
            m_CustomPlot->graph(i)->addData(curTime.addSecs(1 - count).toSecsSinceEpoch(), 0);
        }
    }

	m_CustomPlot->xAxis->setRange(curTime.addSecs(-10 * 60).toSecsSinceEpoch(), curTime.toSecsSinceEpoch());
	m_CustomPlot->xAxis->setTickPen(QPen("transparent"));
	m_CustomPlot->xAxis->setTickLabelFont(m_labelsFont);

	m_CustomPlot->yAxis->setRange(0, 300);
	m_CustomPlot->yAxis->setTickPen(QPen("transparent"));
	m_CustomPlot->yAxis->setTickLabelFont(m_labelsFont);
}

void QCxChart::addData(QVariantList data, int msecOffset)
{
    if (!m_CustomPlot) return;

    auto curTime = QDateTime::currentDateTime();
    curTime = curTime.addMSecs(msecOffset);
    double key = curTime.toSecsSinceEpoch();

    QCPRange newRange;
    for (int i = 0; i < data.size(); i++)
    {
        if (i < m_CustomPlot->graphCount())
        {
			bool foundRange;
            m_CustomPlot->graph(i)->addData(key, data[i].toDouble());
            QCPRange tempRange = m_CustomPlot->graph(i)->getValueRange(foundRange, QCP::sdBoth, QCPRange());
            if (newRange == QCPRange())
            {
                newRange = tempRange;
                continue;
            }
            if (tempRange.lower < newRange.lower)
            {
                newRange.lower = tempRange.lower;
            }
            if (tempRange.upper > newRange.upper)
            {
                newRange.upper = tempRange.upper;
            }
        }
    }
    newRange.lower -= 5;
    newRange.upper += 5;
    m_CustomPlot->yAxis->setRange(newRange);
    m_CustomPlot->xAxis->setRange(key, 10 * 60.0, Qt::AlignRight);
    m_CustomPlot->replot();
}

int QCxChart::graphCount() const
{
    return m_graphCount;
}

void QCxChart::setGraphCount(const int& graphCount)
{
    if (m_graphCount == graphCount) return;

    m_graphCount = graphCount;
	//emit graphCountChanged();
}

QVariantList QCxChart::penColors() const
{
    return m_penColors;
}

void QCxChart::setPenColors(QVariantList penColors)
{
    if (m_penColors == penColors) return;

    m_penColors = penColors;
	//emit penColorsChanged();
}

QVariantList QCxChart::fillColors() const
{
    return m_fillColors;
}

void QCxChart::setFillColors(QVariantList fillColors)
{
    if (m_fillColors == fillColors) return;

    m_fillColors = fillColors; 
	//emit fillColorChanged();
}

QColor QCxChart::labelsColor() const
{
    return m_labelsColor;
}

void QCxChart::setLabelsColor(const QColor& color)
{
    if (m_labelsColor == color) return;

	m_labelsColor = color;
	emit labelsColorChanged();
}

QColor QCxChart::gridLineColor() const
{
    return m_gridLineColor;
}

void QCxChart::setGridLineColor(const QColor& gridLineColor)
{
    if (m_gridLineColor == gridLineColor) return;

    m_gridLineColor = gridLineColor;
	emit gridLineColorChanged();
}

QColor QCxChart::backgroundColor() const
{
	return m_backgroundColor;
}

void QCxChart::setBackgroundColor(const QColor& backgroundColor)
{
	if (m_backgroundColor == backgroundColor) return;

	m_backgroundColor = backgroundColor;
	emit backgroundColorChanged();
}

QFont QCxChart::labelsFont() const
{
	return m_labelsFont;
}

void QCxChart::setLabelsFont(const QFont& font)
{
	if (m_labelsFont == font) return;

	m_labelsFont = font;
	//emit labelsFontChanged();
}

void QCxChart::onCustomReplot()
{
	update();
}
 
void QCxChart::updateCPSize()
{
    if (m_CustomPlot) m_CustomPlot->setGeometry(0, 0, width(), height());
}

void QCxChart::updateCPBackgroundColor()
{
	if (m_CustomPlot) m_CustomPlot->setBackground(QBrush(backgroundColor()));
}

void QCxChart::updateCPGridLineColor()
{
	if (m_CustomPlot)
	{
		QPen pen(gridLineColor());

		m_CustomPlot->xAxis->grid()->setPen(pen);
		m_CustomPlot->xAxis->setBasePen(pen); 
		m_CustomPlot->xAxis2->setBasePen(pen);

		m_CustomPlot->yAxis->grid()->setPen(pen);
		m_CustomPlot->yAxis->setBasePen(pen); 
		m_CustomPlot->yAxis2->setBasePen(pen);
	}
}

void QCxChart::updateCPLabelsColor()
{
	if (m_CustomPlot)
	{
		QColor color = labelsColor();
		m_CustomPlot->xAxis->setTickLabelColor(color);
		m_CustomPlot->yAxis->setTickLabelColor(color);
	}
}
