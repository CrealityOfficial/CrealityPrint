#include "chengfeislice.h"

#include "interface/modelinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"

#include "data/modeln.h"
#include "cxutil/slicer/meshslice.h"

//#define MM2INT(n) ((n) * 1000 + 0.5 * (((n) > 0) - ((n) < 0)))
//#define INT2MM(n) (static_cast<double>(n) / 1000.0)

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

void poly2contour(cxutil::Polygons& poly, chengfeiSplit::Contour& contour)
{
    for (auto& path : poly.paths)
    {
        chengfeiSplit::Path _path;
        _path.solid = ClipperLib::Orientation(path);
        for (auto& point : path)
        {
            _path.line.push_back(std::make_pair(INT2MM(point.X), INT2MM(point.Y)));
        }
        contour.push_back(_path);
    }
}

void contour2poly(chengfeiSplit::Contour& contour, cxutil::Polygons& poly)
{
    for (auto& path : contour)
    {
        ClipperLib::Path _path;
        for (auto p : path.line)
        {
            _path.push_back(ClipperLib::IntPoint(MM2INT(p.first), MM2INT(p.second)));
        }
        poly.add(_path);
    }
}

void ChengFeiSlice::contourSplit()
{
    int interval = MM2INT(m_chengfeiSplit.interval);

    std::list<chengfeiSplit::Line>& splitLine = m_chengfeiSplit.lineList;

    cxutil::Polygons ploygon;
    for (auto line: splitLine)
    {
        ClipperLib::Path path;
        for (auto& point : line)
        {
            path.push_back(ClipperLib::IntPoint(MM2INT(point.first), MM2INT(point.second)));
        }
        if (!path.empty())
        {
            ploygon.add(path);
        }
    }

    auto getPloygons = [&interval](cxutil::Polygons& ploygon)
    {
        ClipperLib::ClipperOffset clipper;
        clipper.AddPaths(ploygon.paths, ClipperLib::JoinType::jtSquare, ClipperLib::etOpenSquare);
        clipper.Execute(ploygon.paths, interval);
    };
    getPloygons(ploygon);


    std::vector<cxutil::Polygons> _vctPolys;
    for (auto&contour : m_contours)
    {
        cxutil::Polygons poly;
        contour2poly(contour, poly);
        _vctPolys.push_back(poly);
    }

    for (auto& poly : _vctPolys)
    {
        poly = poly.difference(ploygon);
    }

    std::vector<cxutil::PolygonsPart> splitIntoParts;
    for (auto& poly : _vctPolys)
    {
        std::vector<cxutil::PolygonsPart> _splitIntoParts;
        _splitIntoParts = poly.splitIntoParts();

        splitIntoParts.insert(splitIntoParts.end(), _splitIntoParts.begin(), _splitIntoParts.end());
    }

    m_contours.clear();
    for (auto& poly : splitIntoParts)
    {
        chengfeiSplit::Contour contour;
        poly2contour(poly, contour);
        m_contours.push_back(contour);
    }

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
    trimesh::box3 b;

    QList<ModelGroup*> groups = modelGroups();
    std::vector<trimesh::TriMesh* > meshs;
    for (ModelGroup* group : groups)
    {
         QList<ModelN*> models = group->models();
         for (ModelN* model : models)
         {
             trimesh::TriMesh* mesh = new trimesh::TriMesh();
             *mesh = *model->globalMesh().get();
             mesh->need_bbox();
             b += mesh->bbox;
             meshs.push_back(mesh);
         }
    }

    std::vector<cxutil::SlicedMesh> slicedMeshes;
    std::vector<int> z;

    int height = b.max.z - b.min.z;
    const int layer = 10;
    float per = height*1.0f / layer;
    for (size_t i = 1; i <= layer; i++)
    {
        int h = per * i;
        if (h > 0)
        {
            z.push_back(per * i);
        }
    }

    cxutil::sliceMeshes_src(meshs, slicedMeshes, z);

    m_contours.clear();
    //std::vector<cxutil::Polygons> vctPolys;

    //过滤掉有开区间的层
    bool haveClosedLayer = false;
    for (auto& smesh : slicedMeshes)
    {
        for (auto& poly : smesh.m_layers)
        {
            if (!poly.polygons.empty() && poly.openPolylines.empty())
            {
                haveClosedLayer = true;
                break;
            }
        }
    }


    for (auto& smesh : slicedMeshes)
    {
        cxutil::Polygons polys;
        for (auto& poly : smesh.m_layers)
        {
            if (haveClosedLayer && !poly.openPolylines.empty())
            {
                continue;
            }
            //polys = polys.unionPolygons(poly.polygons);
            polys.add(poly.polygons);
        }
        polys = polys.unionPolygons();
        if (!polys.paths.empty())
        {
            chengfeiSplit::Contour contour;
            poly2contour(polys, contour);
            m_contours.push_back(contour);
        }
    }

    //poly2contour
    updatePoly();

    for (auto mesh : meshs)
    {
        delete mesh;
        mesh = nullptr;
    }
    meshs.clear();
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

QVariantList ChengFeiSlice::getModelContourList() {
  return contours_cache_;
}

}  // namespace creative_kernel
