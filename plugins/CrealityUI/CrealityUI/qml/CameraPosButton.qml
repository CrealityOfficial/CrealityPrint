import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
Button {
    id:control
    width: 48
    height: 48
    property var hoveredIconSource :""
    property var normalIconSource : ""
    property color bgColor: "transparent"
    property var imgWidth :32
    property var imgHieght :32
    signal buttonClicked()

    padding: 0
    contentItem:Image {
             id: icon_image
             //anchors.verticalCenter : enableButton.verticalCenter
             anchors.centerIn: parent
             width : sourceSize.width
             height: sourceSize.height
//             implicitWidth: 32 //图像宽度
//             implicitHeight: 32 //图像高度

             source: control.hovered?hoveredIconSource:normalIconSource
             //autoTransform: true //图像是否自动变换，默认为false
			 fillMode: Image.Pad
         }
    background: Rectangle {
        color:bgColor
    }
    onClicked: {
        buttonClicked()
    }

}
