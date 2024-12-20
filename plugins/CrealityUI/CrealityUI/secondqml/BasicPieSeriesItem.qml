import QtQuick 2.0
import QtCharts 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0 as QQC2
import CrealityUI 1.0
import "qrc:/CrealityUI"
import ".."
import "../qml"

Item {
    anchors.fill: parent
    property var firstLable: "first"
    property var secondLable: "second"
    property var firstValue: 5
    property var secondValue: 5
    property var bgColor: "white"
    property var titleLable: "title"

    ChartView {
        id: chart
        anchors.fill: parent

        legend.alignment: Qt.AlignBottom//Qt.AlignRight//Qt.AlignBottom
        antialiasing: true
        legend.visible: true
        legend.font.pointSize: Constants.labelFontPointSize_10
        backgroundColor: bgColor
        margins.left:0
        margins.bottom:0
        margins.top:0
        margins.right:0

        PieSeries {
            id: pieSeries
            size: 1
            PieSlice {
                label: firstLable
                value: firstValue
                color: "#1E9BE2"
            }
            PieSlice {
                label: secondLable
                value: secondValue
                color: "#C9C9C9"
            }
        }
    }

    Component.onCompleted: {
    }
}
