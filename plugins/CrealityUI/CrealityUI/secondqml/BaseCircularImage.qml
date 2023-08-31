import QtQuick 2.6
import QtGraphicalEffects 1.0

Rectangle {
    property var img_src: ""
    property var radiusImg: width / 2
    property var isPreserveAspectCrop: false
    property var isLeftTopRadius: true 
    property var isRightTopRadius: true 
    property var isLeftBottomRadius: true 
    property var isRightBottomRadius: true 

    radius: radiusImg
    color: "transparent"

    Image {
        id: _image
        mipmap: true
        smooth: true
        cache: false
        asynchronous: true
        fillMode: isPreserveAspectCrop ? Image.PreserveAspectCrop : Image.PreserveAspectFit
        visible: false
        anchors.fill: parent
        source: img_src
        sourceSize: Qt.size(parent.width, parent.height)
        antialiasing: true
    }
    Rectangle {
        id: _mask
        color: "black"
        anchors.fill: parent
        radius: radiusImg
        visible: false
        antialiasing: true
        smooth: true
        Rectangle{
            anchors.top: parent.top
            anchors.left: parent.left
            width: radiusImg
            height: radiusImg
            visible: !isLeftTopRadius
            antialiasing: true
            smooth: true
        }
        Rectangle{
            anchors.top: parent.top
            anchors.right: parent.right
            width: radiusImg
            height: radiusImg
            visible: !isRightTopRadius
            antialiasing: true
            smooth: true
        }
        Rectangle{
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: radiusImg
            height: radiusImg
            visible: !isLeftBottomRadius
            antialiasing: true
            smooth: true
        }
        Rectangle{
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: radiusImg
            height: radiusImg
            visible: !isRightBottomRadius
            antialiasing: true
            smooth: true
        }
    }
    OpacityMask {
        id: mask_image
        anchors.fill: _image
        source: _image
        maskSource: _mask
        visible: true
        antialiasing: true
    }
}