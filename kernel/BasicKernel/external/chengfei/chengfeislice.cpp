#include "chengfeislice.h"

#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"

#include "data/modeln.h"
#include "topomesh/interface/poly.h"

namespace creative_kernel {

ChengFeiSlice::ChengFeiSlice(QObject* parent) {}

ChengFeiSlice::~ChengFeiSlice() {}

void ChengFeiSlice::reset() {
  m_chengfeiSplit = {};
}

void ChengFeiSlice::setSliceType(int type) {
  m_chengfeiSplit.type = static_cast<chengfeiSplit::Type>(type);
  if (m_chengfeiSplit.type == chengfeiSplit::Type::CONTOUR_PARTITION) {
    getPloygons();
    Q_EMIT modelContourListChanged();
  }
}

void ChengFeiSlice::setTopHeight(const QString h) {
  m_chengfeiSplit.topHeight = h.toFloat();
}

void ChengFeiSlice::setBottomHeight(const QString h) {
  m_chengfeiSplit.bottomHeight = h.toFloat();
}

void ChengFeiSlice::setToPlate(bool toPlate) {
  m_chengfeiSplit.toPlate = toPlate;
}

void ChengFeiSlice::setWidthSplitPos(const QStringList& posList) {
  std::list<float> list;
  for (const auto& pos : posList) {
    list.emplace_back(pos.toFloat());
  }
  m_chengfeiSplit.widthSplitPosList = std::move(list);
}

void ChengFeiSlice::setDepthSplitPos(const QStringList& posList) {
  std::list<float> list;
  for (const auto& pos : posList) {
    list.emplace_back(pos.toFloat());
  }
  m_chengfeiSplit.depthSplitPosList = std::move(list);
}

void ChengFeiSlice::setSliceOrder(const QVariantList& orderList) {
  std::list<size_t> list;
  for (const auto& order : orderList) {
    list.emplace_back(order.toUInt());
  }
  m_chengfeiSplit.sliceOrder = std::move(list);
}

void ChengFeiSlice::setLineList(const QVariantList& lineList) {
  std::list<chengfeiSplit::Line> line_list;
  for (const auto& point_list : lineList) {
    chengfeiSplit::Line line;
    for (const auto& point_variant: point_list.toList()) {
      auto point = point_variant.toPointF();
      auto x = static_cast<float>(point.x());
      auto y = static_cast<float>(point.y());
      line.emplace_back(std::make_pair(x, y));
    }
    line_list.emplace_back(std::move(line));
  }
  m_chengfeiSplit.lineList = std::move(line_list);
}

void ChengFeiSlice::setOrderedPointList(const QVariantList& orderedPointList) {
  decltype(m_chengfeiSplit.orderPointList) list;

  for (const auto& point_var : orderedPointList) {
    auto point = point_var.toPointF();
    auto x = static_cast<float>(point.x());
    auto y = static_cast<float>(point.y());
    list.emplace_back(std::make_pair(x, y));
  }

  m_chengfeiSplit.orderPointList = std::move(list);
}

void ChengFeiSlice::setInterval(float interval) {
  m_chengfeiSplit.interval = interval;
}

void ChengFeiSlice::preSlice() {
  contourSplit();
  Q_EMIT modelContourListChanged();
}

chengfeiSplit ChengFeiSlice::getChengfeiSplit() {
    dirtyModelSpace();
    return m_chengfeiSplit;
}

void convert(const std::vector<topomesh::Contour>& polys, std::vector<chengfeiSplit::Contour>& contours)
{
    contours.clear();
    for (const topomesh::Contour& poly : polys)
    {
        chengfeiSplit::Contour c;
        for (const topomesh::Path& path : poly)
        {
            chengfeiSplit::Path p;
            p.solid = path.solid;
            for (const trimesh::vec2& v : path.line)
                p.line.push_back(std::make_pair(v.x, v.y));
            c.push_back(p);
        }
        contours.push_back(c);
    }
}

void ChengFeiSlice::contourSplit()
{
    std::vector<std::vector<trimesh::vec2>> inputs;
    const std::list<chengfeiSplit::Line>& splitLine = m_chengfeiSplit.lineList;
    for (const chengfeiSplit::Line& line : splitLine)
    {
        if (line.size() > 0)
        {
            std::vector<trimesh::vec2> input;
            for (const std::pair<float, float>& point : line)
                input.push_back(trimesh::vec2(point.first, point.second));
            inputs.push_back(input);
        }
    }

    m_interactiveSlice.contourSplit(inputs, m_chengfeiSplit.interval);

    std::vector<topomesh::Contour> polys;
    m_interactiveSlice.getCurrentContours(polys);
    convert(polys, m_contours);

    updatePoly();
}

void ChengFeiSlice::updatePoly()
{
    contours_cache_.clear();
    //poly2contour
    for (auto& contour : m_contours)
    {
        //chengfeiSplit::Contour contour;
        //poly2contour(poly, contour);
        addCache(contour);
    }
}

void ChengFeiSlice::addCache(chengfeiSplit::Contour& contour)
{
    QVariantList contour_var;

    for (const auto& path : contour) {
        QVariantList points_var;
        for (const auto& point : path.line) {
            points_var.push_back(QVariant::fromValue(QPointF{ point.first, point.second }));
        }

        contour_var.push_back(QVariant::fromValue(QVariantList{
            QVariant::fromValue(path.solid),
            QVariant::fromValue(std::move(points_var)),
        }));
    }

    contours_cache_.push_back(QVariant::fromValue(std::move(contour_var)));
}

void ChengFeiSlice::getPloygons()
{
    QList<ModelGroup*> groups = modelGroups();
    std::vector<TriMeshPtr> meshptrs;
    for (ModelGroup* group : groups)
    {
         QList<ModelN*> models = group->models();
         for (ModelN* model : models)
         {
             TriMeshPtr mesh = model->globalMesh();
             meshptrs.push_back(mesh);
         }
    }
    std::vector<trimesh::TriMesh* > meshs;
    for (TriMeshPtr m : meshptrs)
        meshs.push_back(m.get());

    m_interactiveSlice.reset(meshs);

    std::vector<topomesh::Contour> polys;
    m_interactiveSlice.getCurrentContours(polys);
    convert(polys, m_contours);

    //poly2contour
    updatePoly();
}

QVariantList ChengFeiSlice::getSelectedModelContour() {
  auto selected_model_list = selectionms();
  if (selected_model_list.empty()) {
    return {};
  }

  QVariantList pointer_list;
  for (const auto& pointer : getModelHorizontalContour(selected_model_list.front())) {
    pointer_list.push_back(QVariant::fromValue(pointer));
  }
  return pointer_list;
}

QVariantList ChengFeiSlice::getModelContourList() const {
  return contours_cache_;
}

}  // namespace creative_kernel
