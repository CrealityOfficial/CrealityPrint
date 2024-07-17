import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

import CrealityUI 1.0

import ".."
import "../qml"
import "qrc:/CrealityUI"

Item {
    property int btnRadius: 0
    property int btnBorderW: 2
    property var keystr: 0
    property var id: ""
    property string btnImgUrl: ""
    property bool btnSelect: false
    signal sigBtnClicked(var key)

    id: basicButton
    implicitWidth: 60
    implicitHeight: 60

    Button {
        id : propertyButton
        anchors.fill: parent

        readonly property bool focused: hovered || btnSelect

        background: Rectangle {
            anchors.fill: parent
            radius: btnRadius
            border.width: btnBorderW
            border.color: propertyButton.focused ? Constants.themeGreenColor : "#DBDBDB"
            color: propertyButton.focused ? "#E9E9E9" : "#DBDBDB"

            Image {
                anchors.fill: parent
                opacity: propertyButton.focused ? 1 : 0.3
                mipmap: true
                smooth: true
                cache: false
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                source: btnImgUrl
            }
        }

        onClicked: {
            sigBtnClicked(keystr)
        }
    }
}

