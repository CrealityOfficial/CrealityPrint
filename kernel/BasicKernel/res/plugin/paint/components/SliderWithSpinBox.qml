import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Item {
    id: root
    height: 30
    property string title: ""
    property var value: 1000
    property var from: 100
    property var to: 5000

    onValueChanged: {
        if (value != idSlider.value)
            idSlider.value = value
    }


    //  {
    //     Layout.alignment: Qt.AlignLeft | Qt.AlignHCenter
    //     Layout.fillWidth: true

    //     fontText: qsTr("Shape:")
    //     fontColor: Constants.textColor
    //     hAlign: 0
    // }
    CusText {
        id: idTitle
        anchors.left: parent.left
        anchors.right: idSlider.left
        // width: 52 * screenScaleFactor
        height: parent.height
        fontText: title
        // verticalAlignment: Text.AlignVCenter
        // horizontalAlignment: Text.AlignLeft
        fontColor: Constants.textColor
        hAlign: 0
        fontPointSize: Constants.labelFontPointSize_9
    }

    Slider {
        id: idSlider
        height: parent.height
        anchors.right: idSpinBox.left
        // anchors.left: idTitle.right
        anchors.margins: 10
        width: 130 * screenScaleFactor

        from: root.from
        to: root.to
        value: root.value

        background: Rectangle {
            x: idSlider.leftPadding
            y: idSlider.topPadding + idSlider.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 4
            width: idSlider.availableWidth
            height: implicitHeight
            radius: 2
            color: Constants.lpw_BtnBorderColor

            Rectangle {
                width: idSlider.visualPosition * parent.width
                height: parent.height
                color: "#00A3FF"
                radius: 2
            }
        }

        handle: Rectangle {
            x: idSlider.leftPadding + idSlider.visualPosition * (idSlider.availableWidth - width)
            y: idSlider.topPadding + idSlider.availableHeight / 2 - height / 2
            implicitWidth: 18
            implicitHeight: 18
            radius: 9
            color: "#00A3FF"
            border.color: "#7DEEFF"
            border.width: 2
        }

        onValueChanged: {
            idSpinBox.value = value
            root.value = value
        }
    }

    CusStyledSpinBox {
        id: idSpinBox
        anchors.right: parent.right
        width: 60 * screenScaleFactor
        height: parent.height

        from: root.from
        to: root.to
        value: root.value

        onValueChanged: {
            idSlider.value = value
            root.value = value
        }
    }

}