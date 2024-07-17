import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import QtQuick.Templates 2.12 as T
T.TextField{
    id: control
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    padding: 0
    leftPadding: 10
    rightPadding: 10
    placeholderTextColor: Color.transparent(control.color, 0.5)
    selectionColor: control.palette.highlight
    selectedTextColor: control.palette.highlightedText
    verticalAlignment: TextField.AlignVCenter

    opacity : control.enabled ? 1 : 0.7
    color: Constants.textColor
//    selectByMouse  : true
    font.weight: Constants.labelFontWeight
    font.family: Constants.labelFontFamily
    readOnly : true
    property color defaultBackgroundColor: Constants.dialogItemRectBgColor
    property color  itemBorderColor: Constants.dialogItemRectBgBorderColor
    property alias image: __image
    property real radius : 3
    property real borderW: 1

    signal clicked()

    PlaceholderText {
        id: placeholder
        anchors.verticalCenter: parent.verticalCenter
        width: control.width - (control.leftPadding + control.rightPadding) - __image.width
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
        opacity : control.enabled ? 1 : 0.7
    }
    Image {
        id : __image
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 10
        source: "qrc:/UI/photo/model_library_file-default.svg"
        sourceSize.width: 16* screenScaleFactor
        sourceSize.height: 16* screenScaleFactor
        smooth:true
//        visible: source !== ""
        property bool isHovered: false
        Rectangle
        {
            anchors.fill: parent
            color: "transparent"
            radius: 5* screenScaleFactor
            border.color: "transparent"
            border.width: __image.isHovered ? 1 : 0

            MouseArea
            {
                anchors.fill: parent
                hoverEnabled : true
                cursorShape:Qt.PointingHandCursor
                onExited: {
                    __image.isHovered = false
                }
                onEntered: {
                    __image.isHovered = true
                }
                onClicked:
                {
                    control.clicked()
                }
            }
        }
    }

    background: Rectangle
    {
        border.width:borderW
        border.color:hovered ? Constants.textRectBgHoveredColor :  itemBorderColor
        radius : control.radius
        color : defaultBackgroundColor
        opacity : control.enabled ? 1 : 0.7
    }
}
