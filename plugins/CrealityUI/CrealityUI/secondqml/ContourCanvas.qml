import QtQuick 2.13

import QtQml 2.13

import "./"
import "../qml"

Canvas {
  id: root

  property double scaleFactor: 1.0
  property bool reverseYAxis: true
  property bool originAtCenter: false
  property var contourListModel: null
  property bool enableChangeOrder: false

  property int fps: 60
  property string backgroundColor: "rgb(220, 220, 220)"
  property string modelColor: "rgb(120, 120, 120)"
  property string modelFocusedColor: "rgb(60, 60, 60)"
  property string textColor: "rgb(120, 200, 120)"
  property string textFocusedColor: "rgb(100, 255, 100)"

  function scalePointToPrinter(point) {
    let scaled_point = Qt.point(point.x, point.y)

    if (reverseYAxis) {
      scaled_point.y = -scaled_point.y + height
    }

    if (originAtCenter) {
      scaled_point.x -= width / 2
      scaled_point.y -= height / 2
    }

    if (scaleFactor !== 1.0) {
      scaled_point.x *= scaleFactor
      scaled_point.y *= scaleFactor
    }

    return scaled_point
  }

  function scalePointFromPrinter(point) {
    let scaled_point = Qt.point(point.x, point.y)

    if (scaleFactor !== 1.0) {
      scaled_point.x /= scaleFactor
      scaled_point.y /= scaleFactor
    }

    if (originAtCenter) {
      scaled_point.x += width / 2
      scaled_point.y += height / 2
    }

    if (reverseYAxis) {
      scaled_point.y = -scaled_point.y + height
    }

    return scaled_point
  }

  function makeLineFx(first_point, second_point) {
    if (first_point.x == second_point.x) {
      return () => { return null }
    }

    if (first_point.y == second_point.y) {
      return () => { return first_point.y }
    }

    return (x) => {
      let x1 = first_point.x
      let y1 = first_point.y
      let x2 = second_point.x
      let y2 = second_point.y
      return ((y2 - y1) * x - x1 * y2 + x2 * y1) / (x2 - x1)
    }
  }

  function makeLineFy(first_point, second_point) {
    if (first_point.y == second_point.y) {
      return (y) => { return null }
    }

    if (first_point.x == second_point.x) {
      return (y) => { return first_point.x }
    }

    return (y) => {
      let x1 = first_point.x
      let y1 = first_point.y
      let x2 = second_point.x
      let y2 = second_point.y
      return ((x2 - x1) * y + x1 * y2 - x2 * y1) / (y2 - y1)
    }
  }

  function isPointInSegment(point, segment_first_point, segment_second_point) {
    // 检查线段合法性
    if (segment_first_point == segment_second_point) {
      return false
    }

    // 是否为线段端点
    if (segment_first_point == point || segment_second_point == point) {
      return true
    }

    // 是否在线段所处直线上
    let fx = makeLineFx(segment_first_point, segment_second_point)
    let fy = makeLineFy(segment_first_point, segment_second_point)
    let line_x = fy ? fy(point.x) : null
    let line_y = fx ? fx(point.y) : null
    if ((!line_x && !line_y) || (line_x && line_x != point.x) || (line_y && line_y != point.y)) {
      return false
    }

    // 是否在线段中
    let abs_first_x = Math.abs(segment_first_point.x)
    let abs_first_y = Math.abs(segment_first_point.y)
    let abs_second_x = Math.abs(segment_second_point.x)
    let abs_second_y = Math.abs(segment_second_point.y)
    let abs_node_x = Math.abs(point.x)
    let abs_node_y = Math.abs(point.y)
    let is_x_internal = (abs_first_x <= abs_node_x && abs_node_x <= abs_second_x) ||
                        (abs_second_x <= abs_node_x && abs_node_x <= abs_first_x)
    let is_y_internal = (abs_first_y <= abs_node_y && abs_node_y <= abs_second_y) ||
                        (abs_second_y <= abs_node_y && abs_node_y <= abs_first_y)
    return is_x_internal && is_y_internal
  }

  function isPointInPath(point, path_points) {
    if (path_points.length < 2) {
      return false
    }

    let first_index = 0
    let second_index = 1
    while (second_index < path_points.length) {
      let first_point = path_points[first_index]
      let second_point = path_points[second_index]

      if (isPointInSegment(point, first_point, second_point)) {
        return true;
      }

      ++first_index
      ++second_index
    }

    return false
  }

  function isPointInArea(point, area_path_points) {
    if (area_path_points.length < 2) {
      return false
    }

    // 交叉数法
    let cross_times = 0
    let first_index = 0
    let second_index = 1
    while (second_index < area_path_points.length) {
      let first_point = area_path_points[first_index]
      let second_point = area_path_points[second_index]

      // 向 X 轴正向水平射线
      if (first_point.y != second_point.y) {
        if ((first_point.y < point.y && point.y < second_point.y) ||
            (second_point.y < point.y && point.y < first_point.y)) {
          let fy = makeLineFy(first_point, second_point)
          let point_x = fy(point.y)
          if (point_x && point_x > point.x) {
            ++cross_times
          }
        }
      }

      ++first_index
      ++second_index
    }

    return cross_times % 2 === 1
  }

  function isPointInContour(point, contour) {
    let in_any_solid = false
    let in_any_hollow = false

    for (let path of contour.paths) {
      if (path.solid && !in_any_solid) {
        if (isPointInArea(point, path.points)) {
          in_any_solid = true
        }
      } else if (!path.solid) {
        if (isPointInArea(point, path.points)) {
          in_any_hollow = true
          break
        }
      }
    }

    return in_any_solid && !in_any_hollow
  }

  function makeOrderedContourList(scaled = true) {
    let scale_point_if_need = (point) => {
      return scaled ? scalePointToPrinter(point) : point
    }

    let contours = []
    for (let contour of _contours) {
      let paths = []
      for (let path of contour.paths) {
        let points = []
        for (let point of path.points) {
          points.push(scale_point_if_need(point))
        }

        paths.push({
          solid: path.solid,
          points: points,
        })
      }

      contours[contour.order] = {
        order: contour.order,
        focused: contour.focused,  // unnecessary
        hovered: contour.hovered,  // unnecessary
        example: scale_point_if_need(contour.example),
        paths: paths,
      }
    }

    return contours
  }

  function findExamplePointInsideContour(contour, scaled = true) {
    let points = []
    for (let path of contour.paths) {
      for (let point of path.points) {
        points.push(point)
      }
    }

    let found = false
    let first_index = 0
    let second_index = 0
    let random_point = Qt.point(0, 0)
    while (!found) {
      first_index = Math.floor(Math.random() * points.length)
      second_index = Math.floor(Math.random() * points.length)
      if (first_index == second_index) {
        continue
      }

      random_point.x = (points[first_index].x + points[second_index].x) / 2
      random_point.y = (points[first_index].y + points[second_index].y) / 2

      let in_path = false
      for (let path of contour.paths) {
        if (isPointInPath(random_point, path.points)) {
          in_path = true
          break
        }
      }

      if (in_path) {
        points.push(random_point)
        continue
      }

      if (isPointInContour(random_point, contour)) {
        found = true
      }
    }

    return scaled ? scalePointToPrinter(random_point) : random_point
  }

  function makeOrderedExamplePointList(scaled = true) {
    let scale_point_if_need = (point) => {
      return scaled ? scalePointToPrinter(point) : point
    }

    let point_list = []
    for (let contour of _contours) {
      point_list[contour.order] = scale_point_if_need(contour.example)
    }
    return point_list
  }

  property var _contours: {
    // example of _contours: [
    //   {
    //     focused: false,
    //     hovered: false,
    //     order: 0,
    //     example: Qt.point(50, 50),  // example point in contour
    //     paths: [
    //       {
    //         solid: true
    //         points: [
    //           Qt.point(0  , 0  ),
    //           Qt.point(100, 0  ),
    //           Qt.point(100, 100),
    //           ...,
    //           Qt.point(0  , 100),  // same as first element
    //         ]
    //       }
    //     ],
    //   },
    //   {
    //     ...
    //   },
    //   ...
    // ]
    let scaled_contours = []

    for (let contour of contourListModel) {
      let scaled_path = []

      for (let path of contour) {
        let solid = path[0]
        let points = path[1]

        let scaled_points = []
        for (let point of points) {
          let scaled_point = scalePointFromPrinter(point)
          scaled_points.push(scaled_point)
        }

        // last element must same as first element
        if (points[0] !== points[points.length - 1]) {
          scaled_points.push(scalePointFromPrinter(points[0]))
        }

        scaled_path.push({
          solid = solid,
          points = scaled_points,
        })
      }

      let scaled_contour = {
        focused: false,
        hovered: false,
        order: scaled_contours.length,
        example: scaled_path[0].points[0],
        paths: scaled_path,
      }
      scaled_contour.example = findExamplePointInsideContour(scaled_contour, false)
      scaled_contours.push(scaled_contour)
    }

    return scaled_contours;
  }

  antialiasing: true

  Component.onCompleted: {
    getContext("2d")
  }

  onPaint: {
    context.fillStyle = backgroundColor
    context.fillRect(0, 0, width, height)

    for (let contour of _contours) {
      let highlighted = root.enableChangeOrder && (contour.focused || contour.hovered)

      context.beginPath()
      for (let path of contour.paths) {
        context.moveTo(path.points[0].x, path.points[0].y)
        for (let point of path.points) {
          context.lineTo(point.x, point.y)
        }
      }
      context.closePath()
      // context.fillStyle = path.solid ? highlighted ? modelFocusedColor : modelColor
      //                                 : backgroundColor
      context.fillStyle = highlighted ? modelFocusedColor : modelColor
      context.fill()
    }

    context.textAlign = "center"
    context.textBaseline = "middle"
    context.font = "18pt bold sans-serif"
    for (let contour of _contours) {
      if (root.enableChangeOrder) {
        context.fillStyle = contour.focused || contour.hovered ? textFocusedColor : textColor
        context.fillText(String(contour.order), contour.example.x, contour.example.y)

        // context.fillStyle = "red"
        // context.beginPath()
        // context.arc(contour.example.x, contour.example.y, 2, 0, 2 * Math.PI)
        // context.closePath()
        // context.fill()
      }
    }
  }

  MouseArea {
    id: mouse

    anchors.fill: parent
    enabled: root.enableChangeOrder

    hoverEnabled: enabled
    acceptedButtons: Qt.LeftButton

    onClicked: function(mouse) {
      let mouse_position = Qt.point(mouse.x, mouse.y)

      let focuesd_indexs = []
      for (let index = 0; index < _contours.length; ++index) {
        if (isPointInContour(mouse_position, _contours[index])) {
          _contours[index].focused = !_contours[index].focused
        }

        if (_contours[index].focused) {
          focuesd_indexs.push(index)
        }
      }

      if (focuesd_indexs.length >= 2) {
        let temp_order = _contours[focuesd_indexs[0]].order
        _contours[focuesd_indexs[0]].order = _contours[focuesd_indexs[1]].order
        _contours[focuesd_indexs[1]].order = temp_order

        for (let index of focuesd_indexs) {
          _contours[index].focused = false
        }
      }
    }

    onPositionChanged: function(mouse) {
      let mouse_position = Qt.point(mouse.x, mouse.y)
      for (let contour of _contours) {
        contour.hovered = isPointInContour(mouse_position, contour)
      }
    }
  }

  Timer {
    repeat: true
    running: root.visible
    interval: 1000 / root.fps
    onTriggered: root.requestPaint()
  }
}
