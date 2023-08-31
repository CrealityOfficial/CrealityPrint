#ifndef _CHENFEI_PREVIEWWORKER_1603796964788_H
#define _CHENFEI_PREVIEWWORKER_1603796964788_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <list>

namespace creative_kernel {


struct chengfeiSplit {
  enum class Type : int {
    HEIGHT_RESTRICTIONS = 0,
    HORIZONTAL_PARTITION = 1,
    CONTOUR_PARTITION = 2,
  };

  Type type{Type::HEIGHT_RESTRICTIONS};

  float topHeight{ 0.0f };
  float bottomHeight{ 0.0f };
  bool toPlate{ false };

  std::list<float> widthSplitPosList{};  // old
  std::list<float> depthSplitPosList{};  // old
  std::list<size_t> sliceOrder{};  // old

  using Point = std::pair<float, float>;
  using Line = std::list<Point>;
  struct Path {
    bool solid{ true };
    Line line{};
  };
  using Contour = std::list<Path>;

  std::list<Line> lineList{};
  std::list<Contour> contourList{};
  std::list<Point> orderPointList{};

  float interval{ 0.0f };
};

class ChengFeiSlice : public QObject {
  Q_OBJECT;

public:
  ChengFeiSlice(QObject* parent = nullptr);
  virtual ~ChengFeiSlice();

  Q_INVOKABLE void reset();

  /// @param type chengfeiSplit::Type
  Q_INVOKABLE void setSliceType(int type);

  Q_INVOKABLE void setTopHeight(const QString h);
  Q_INVOKABLE void setBottomHeight(const QString h);
  Q_INVOKABLE void setToPlate(bool toPlate);

  Q_INVOKABLE void setWidthSplitPos(const QStringList& posList);  // old
  Q_INVOKABLE void setDepthSplitPos(const QStringList& posList);  // old
  Q_INVOKABLE void setSliceOrder(const QVariantList& orderList);  // old
  Q_INVOKABLE void setLineList(const QVariantList& lineList);
  Q_INVOKABLE void setOrderedPointList(const QVariantList& orderedPointList);
  Q_INVOKABLE void setInterval(float interval);

  Q_INVOKABLE void preSlice();

  chengfeiSplit getChengfeiSplit();

private:
    void getPloygons();
    void addCache(chengfeiSplit::Contour& contour);
    void contourSplit();
    void updatePoly();

public:  // util for qml
  Q_INVOKABLE QVariantList getSelectedModelContour();  // old

  Q_PROPERTY(QVariantList modelContourList READ getModelContourList NOTIFY modelContourListChanged);
  Q_SIGNAL void modelContourListChanged();
  QVariantList getModelContourList();

private:
  chengfeiSplit m_chengfeiSplit;
  QVariantList contours_cache_;

  //std::vector<cxutil::Polygons> m_vctPolys;
  std::vector<chengfeiSplit::Contour> m_contours;
};

}  // namespace creative_kernel

#endif  // _CHENFEI_PREVIEWWORKER_1603796964788_H
