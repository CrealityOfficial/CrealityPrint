import QtQuick 2.12
import QtQuick.Controls 2.5
import "../qml"

Rectangle{
    property alias tabModel: repeater.model
    property real currentIndex: 0
    property real btnWidth: 80* screenScaleFactor
    property color btnNormalColor: "transparent"//Constants.profileBtnColor
    property color btnHoveredColor: "transparent"//Constants.profileBtnHoverColor
    property color btnPressedColor: "transparent"//Constants.profileBtnHoverColor
    property color btnBorderColor: btnHoveredColor
    property color btnBorderColorHovered: btnHoveredColor
    property color btnBorderColorChecked: btnHoveredColor
    property real btnBorderWidth

    property real txtPointSize: Constants.labelFontPointSize_12
    property string txtFamily: Constants.labelFontFamilyBold
    property bool txtBold: false

    property color txtColorNormal: Constants.textColor
    property color txtColorHovered: txtColorNormal
    property color txtColorPressed: Constants.textColor
    property bool allRadius: false
    property bool leftTop: allRadius
    property bool rightTop: allRadius
    property bool rightBottom: allRadius
    property bool leftBottom: allRadius
    id:rootRec
    Row{
        anchors.left: parent.left
        anchors.top: parent.top
        Repeater{
            id: repeater
            delegate: CusButton{
                allRadius: rootRec.allRadius
                leftTop: rootRec.leftTop
                rightTop: rootRec.rightTop
                rightBottom: rootRec.rightBottom
                leftBottom: rootRec.leftBottom
                txtPointSize: rootRec.txtPointSize
                txtFamily: rootRec.txtFamily
                txtBold: rootRec.txtBold
                normalColor: btnNormalColor
                hoveredColor: btnHoveredColor
                pressedColor: btnPressedColor
                txtContent: modelData
                borderWidth: isHovered ? 2 : 0//2* screenScaleFactor
                borderColor: isHovered ? rootRec.btnBorderColorHovered : rootRec.btnBorderColor
                txtColor: isChecked ? txtColorPressed : txtColorNormal
                width: rootRec.btnWidth
                height: rootRec.height
                isChecked: currentIndex == model.index
                onClicked: {
                    rootRec.currentIndex = model.index
                }
            }
        }
    }
}
