import QtQuick 2.12
import "../qml"

//QtQuick.control2带Icon的Label是个单独的控件
//但是自定义起来不大方便，比如没有renderType那就只能全局设置该属性
//而IconLabel源码中用的也比较多，我就单独组合一个
//Item可以替换为Rectangle以设置背景色
Item{
    id: control

    property alias source: _icon.source
    property alias text: _text.text
    property alias color: _text.color
    property alias font: _text.font
    property alias spacing: _row.spacing
    //懒得写，直接全部引出
    property alias controlImage: _icon
    property alias controlText: _text
    property alias controlRow: _row

    implicitWidth: ((_icon.source&&_text.implicitWidth)?_row.spacing:0)+_icon.implicitWidth+_text.implicitWidth
                   +_row.leftPadding+_row.rightPadding
    implicitHeight: 30
    Item {
        id: _row
        //图标和文本居中并排的话可以anchors.centerIn: parent
        height: parent.height
        width: parent.width
        property var spacing: 4
        Image {
            id: _icon
            anchors.centerIn: parent
            //  Distance between img and text  is 4px
            anchors.horizontalCenterOffset:_text.width ===0? 0 :  -(_text.width + spacing )/2
            width: 24
            height: 24
            fillMode: Image.Pad
        }
        Text {
            id: _text
            color: "black"
            anchors.centerIn: parent
            //  Distance between img and text  is 4px
            anchors.horizontalCenterOffset: (_icon.source.length > 0)? (_icon.width + spacing)/2 : 0
            elide: Text.ElideRight
            renderType: Text.NativeRendering
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font{
                family: "SimSun"
                pointSize: Constants.labelFontPointSize_12
            }
        }
    }
}
