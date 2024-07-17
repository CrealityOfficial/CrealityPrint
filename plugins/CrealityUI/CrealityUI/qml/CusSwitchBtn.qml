import QtQuick 2.0
import QtQml 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5



Button {
    id: fanSwitch
    checkable: true
    background: null
    opacity: enabled ? 1.0 : 0.7

    property var delayShow: false
    property bool fan_checked

    property color btnColor: "#FFFFFF" // Constants.typeBtnHoveredColor
    property color backGroundColor: Constants.themeGreenColor
    property var btnHeight: 20* screenScaleFactor
    Layout.preferredWidth: 44 * screenScaleFactor
    Layout.preferredHeight: 24 * screenScaleFactor

    Binding on checked {
        when: !fanSwitch.delayShow
        value: fanSwitch.fan_checked
    }

    Timer {
        repeat: false
        interval: 10000
        id: fanSwitchDelayShow
        onTriggered: fanSwitch.delayShow = false
    }

    indicator: Rectangle{
        radius: height / 2
        anchors.fill: parent

        border.width: 1
        border.color: "transparent"
        color: fanSwitch.checked ? backGroundColor: Constants.rectBorderColor

        Rectangle {
            radius: height / 2
            width: btnHeight
            height: btnHeight
            anchors.verticalCenter: parent.verticalCenter

            color: fanSwitch.checked ? btnColor : Constants.mainBackgroundColor
            x: fanSwitch.checked ? (parent.width - width - 1 * screenScaleFactor) : 1 * screenScaleFactor

            Behavior on x {
                NumberAnimation { duration: 100 }
            }
        }
    }
}
