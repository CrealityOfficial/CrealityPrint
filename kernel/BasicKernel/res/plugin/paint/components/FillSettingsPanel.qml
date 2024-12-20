import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Rectangle {
    id: root
    color: "transparent"
    // color: "red"

    height: (forceOpen ? 30 : (30 + checkable * 35)) * screenScaleFactor 

    property var forceOpen: false
    property var value: 0
    property var checkable: true

    onValueChanged: {
        if (value != idSmartFillAngle.value)
            idSmartFillAngle.value = value
    }
    
    StyleCheckBox {
        id: idEnableSmartFill
        height: 30 * screenScaleFactor
        width: 145 * screenScaleFactor
        textColor: Constants.textColor
        text: qsTr("Enable Smart Fill") 
        font.pointSize: Constants.labelFontPointSize_9
        checked: root.checkable
        visible: !forceOpen

        onCheckedChanged: {
            if (root.checkable != checked)
                root.checkable = checked;
        }

    }

    SliderWithSpinBox {
        id: idSmartFillAngle
        anchors.top: idEnableSmartFill.visible ? idEnableSmartFill.bottom : parent.top
        width: parent.width
        visible: root.checkable || forceOpen
        // visible: idEnableSmartFill.checked || !root.checkable
        // anchors.topMargin: 30
        height: 30 * screenScaleFactor
        title: qsTr("Smart fill angle")

        from: 0
        to: 90
        value: 30

        onValueChanged: {
            root.value = value
        }
    }
}