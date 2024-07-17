import "../secondqml"
import QtGraphicalEffects 1.12
import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

Button {
    id: propertyButton

    property string enabledIconSource
    property string highLightIconSource
    property string pressedIconSource
    property string disabledIconSource
    property bool btnHightLight: false
    property bool bottonSelected: false
    property bool allowTooltip: true
    property color defaultBtnBgColor: Constants.leftBtnBgColor_normal
    property color hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
    property color selectedBtnBgColor: Constants.leftBtnBgColor_selected
    property color btnTextColor: Constants.leftTextColor
    property color btnTextColor_d: Constants.leftTextColor_d
    property color borderColor: Constants.rectBorderColor
    property color hoverBorderColor: Constants.rectBorderColor
    property color selectBorderColor: Constants.rectBorderColor
    property var fontBold: false
    property var fontWeight: Font.bold
    property real borderWidth: 1 * screenScaleFactor
    property real leftMargin: 35 * screenScaleFactor
    property var btnRadius: 5 * screenScaleFactor
    property alias imgWidth: icon_image.width
    property alias imgHeight: icon_image.height
    property alias textWrapMode: idContentText.fontWrapMode
    property alias imgFillMode: icon_image.fillMode
    property alias textAlign: idContentText.hAlign
    //tooltip
    //property real pos: 0 //0: 左 1：上 2：右 3：下
    property var btnTipType: ""
    property alias cusTooltip: idTooptip
    property bool shadowEnabled: false
    focusPolicy: Qt.NoFocus

    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    font.pointSize: Constants.labelFontPointSize_9
    //opacity: enabled ? 1 : 0.3
    state: "imgOnly"
    padding: 0
    states: [
        State {
            name: "wordsOnly"

            AnchorChanges {
                target: idContentText
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PropertyChanges {
                target: icon_image
                visible: false
            }

            PropertyChanges {
                target: idContentText
                visible: true
            }

        },
        State {
            name: "imgOnly"

            PropertyChanges {
                target: icon_image
                visible: true
            }

            PropertyChanges {
                target: idContentText
                visible: false
            }

            AnchorChanges {
                target: icon_image
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

        },
        State {
            name: "imgLeft"

            AnchorChanges {
                target: icon_image
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            PropertyChanges {
                target: icon_image
                anchors.leftMargin: propertyButton.leftMargin
            }

            AnchorChanges {
                target: idContentText
                anchors.left: icon_image.right
                anchors.verticalCenter: parent.verticalCenter
            }

            PropertyChanges {
                target: idContentText
                anchors.leftMargin: 8 * screenScaleFactor
            }

        },
        State {
            name: "imgTop"

            AnchorChanges {
                target: icon_image
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PropertyChanges {
                target: icon_image
                // anchors.topMargin: (propertyButton.height - icon_image.height - idContentText.height - 4 * screenScaleFactor) / 2
                anchors.topMargin: 4 * screenScaleFactor
            }

            AnchorChanges {
                target: idContentText
                anchors.top: icon_image.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PropertyChanges {
                // anchors.topMargin: 4 * screenScaleFactor

                target: idContentText
            }

        },
        State {
            name: "imgBottom"

            AnchorChanges {
                target: icon_image
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PropertyChanges {
                target: icon_image
                // anchors.bottomMargin: (propertyButton.height - icon_image.height - idContentText.height - 4 * screenScaleFactor) / 2
                anchors.bottomMargin: 4 * screenScaleFactor
            }

            AnchorChanges {
                target: idContentText
                anchors.bottom: icon_image.top
                anchors.horizontalCenter: parent.horizontalCenter
            }

            PropertyChanges {
                // anchors.bottomMargin: 4 * screenScaleFactor

                target: idContentText
            }

        }
    ]

    BasicTooltip {
        id: idTooptip

        visible: text !== "" && propertyButton.hovered && allowTooltip
        text: propertyButton.text
        textWrap: false
    }

    contentItem: Item {
        clip: true

        Image {
            id: icon_image

            source: {
                if (!enabled && disabledIconSource)
                    return disabledIconSource;

                if (propertyButton.pressed || bottonSelected) {
                    return pressedIconSource;
                } else if (btnHightLight || propertyButton.hovered) {
                    if (highLightIconSource.length > 0)
                        return highLightIconSource;
                    else
                        return pressedIconSource;
                } else {
                    return enabledIconSource;
                }
            }
            visible: (enabledIconSource.length > 0 || pressedIconSource.length > 0 || highLightIconSource.length > 0) ? true : false
            fillMode: Image.PreserveAspectFit
            sourceSize.width: width
            sourceSize.height: height
        }

        CusText {
            id: idContentText

            fontColor: {
                if (propertyButton.down || bottonSelected)
                    return btnTextColor_d;

                return btnTextColor;
            }
            fontText: propertyButton.text
            font_font: propertyButton.font
            fontWidth: propertyButton.width - 20 * screenScaleFactor
            fontSizeMode: Text.Fit
        }

    }

    background: Rectangle {
        radius: btnRadius
        opacity: enabled ? 1 : 0.3
        border.color: {
            if (propertyButton.down || bottonSelected)
                return selectBorderColor;

            return propertyButton.hovered ? hoverBorderColor : borderColor;
        }
        border.width: propertyButton.borderWidth
        color: {
            if (propertyButton.down || bottonSelected)
                return selectedBtnBgColor;

            return propertyButton.hovered ? hoveredBtnBgColor : defaultBtnBgColor;
        }
        layer.enabled: shadowEnabled

        layer.effect: DropShadow {
            verticalOffset: 3
            color: Constants.dropShadowColor
        }

    }

}
