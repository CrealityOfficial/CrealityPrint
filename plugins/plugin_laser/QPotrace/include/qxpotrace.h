#ifndef QXPOTRACE_H
#define QXPOTRACE_H

#include <qxpotrace_global.h>
#include <QImage>
#include <QList>
#include <QPolygonF>

class QxPotrace
{
public:
  struct Polygon
  {
    QPolygonF boundary;
    QList<QPolygonF> holes;
  };

  QxPotrace();

  bool trace(const QImage &image);

  QList<Polygon> polygons() const;

  void setAlphaMax(qreal alphaMax);
  void setTurdSize(int turdSize);
  void setCurveTolerance(qreal tolerance);
  void setThreshold(int threshold);
  void setBezierPrecision(int precision);

private:
  qreal m_alphaMax;
  int m_turdSize;
  qreal m_curveTolerance;
  int m_threshold;
  int m_bezierPrecision;


  QList<Polygon> m_meshDefs;
};

#endif // QXPOTRACE_H
