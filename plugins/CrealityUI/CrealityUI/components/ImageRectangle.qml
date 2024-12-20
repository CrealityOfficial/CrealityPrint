import QtQuick 2.15
import QtQuick.Controls 2.15

import QtGraphicalEffects 1.15

/// @brief 包装 Image 的 Rectangle 组件, 用于显示图片(支持 gif 格式), 并提供圆角与快速模糊功能.
/// @note 暂不支持带透明度的 color 值
Rectangle {
  // Image 相关参数
  property string imageSource: ''
  property bool imageCache: true
  property bool imageSmooth: true
  property int imageFillMode: Image.PreserveAspectCrop

  // 模糊相关参数
  property bool blurEnabled: false
  property real blurRadius: 128

  // Rectangle 相关参数
  color: '#FFFFFF'
  radius: 0
  property bool topLeftRadiusEnabled: true
  property bool topRightRadiusEnabled: true
  property bool bottomLeftRadiusEnabled: true
  property bool bottomRightRadiusEnabled: true

  id: root

  layer.enabled: radius > 0
  layer.effect: OpacityMask {
    maskSource: mask_rectangle
  }

  Rectangle {
    id: mask_rectangle

    anchors.fill: parent
    radius: parent.radius
    color: parent.color

    Rectangle {
      visible: !root.topLeftRadiusEnabled
      anchors.top: parent.top
      anchors.left: parent.left
      width: parent.radius
      height: parent.radius
      color: parent.color
    }

    Rectangle {
      visible: !root.topRightRadiusEnabled
      anchors.top: parent.top
      anchors.right: parent.right
      width: parent.radius
      height: parent.radius
      color: parent.color
    }

    Rectangle {
      visible: !root.bottomLeftRadiusEnabled
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      width: parent.radius
      height: parent.radius
      color: parent.color
    }

    Rectangle {
      visible: !root.bottomRightRadiusEnabled
      anchors.bottom: parent.bottom
      anchors.right: parent.right
      width: parent.radius
      height: parent.radius
      color: parent.color
    }
  }

  Rectangle {
    anchors.fill: parent
    radius: parent.radius
    color: parent.color

    layer.enabled: root.blurEnabled
    layer.effect: FastBlur {
      radius: root.blurRadius
    }

    Image {
      anchors.fill: parent
      enabled: !String(source).endWith('.gif')
      visible: enabled
      fillMode: root.imageFillMode
      source: root.imageSource
    }

    AnimatedImage {
      anchors.fill: parent
      enabled: String(source).endWith('.gif')
      visible: enabled
      fillMode: root.imageFillMode
      source: root.imageSource
    }
  }
}
