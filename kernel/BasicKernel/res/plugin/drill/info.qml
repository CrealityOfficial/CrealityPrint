import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5

import CrealityUI 1.0

import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"

LeftPanelDialog {
    id: root

    width: 300 * screenScaleFactor
    height: 230 * screenScaleFactor
    title: qsTr("Drill")

    property var com: null

    function execute() {
        console.log("drill panel execute");
    }

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
        hoverEnabled: true
        property bool isPanelArea: false
        onClicked:
        {
            focus = true
        }
        onEntered:
        {
            isPanelArea = true
        }
        onExited:
        {
            if(isPanelArea && __radisBox.textObj.activeFocus)
                __radisBox.textObj.accepted()
            isPanelArea = false
        }
    }
    Item {
        anchors.fill: root.panelArea

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20 * screenScaleFactor
            spacing: 5 * screenScaleFactor

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30 * screenScaleFactor

                CusText {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignHCenter
                    Layout.fillWidth: true

                    fontText: qsTr("Shape:")
                    fontColor: Constants.textColor
                    hAlign: 0
                }

                CustomComboBox {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.preferredWidth: 153 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    model: ListModel {
                        ListElement { text: qsTr("Circle") }
                        ListElement { text: qsTr("Triangle") }
                        ListElement { text: qsTr("Square") }
                    }
                    currentIndex: com.shape
                    onCurrentIndexChanged: {
                        com.shape = this.currentIndex;
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30 * screenScaleFactor

                CusText {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    Layout.fillWidth: true

                    fontText: qsTr("Radius:")
                    fontColor: Constants.textColor
                    hAlign: 0
                }
                LeftToolSpinBox {
                    id : __radisBox
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.preferredWidth: 153 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    realStepSize:0.05
                    realFrom: 1
                    realTo: 20
                    font.pointSize: Constants.labelFontPointSize_9
                    realValue: com ? com.radius.toFixed(2) : 5
                    decimals : 2
                    bResetOtgValue : true
                    orgValue : com ? com.radius.toFixed(2): 5
                    property var posVal : com ? com.radius.toFixed(2) : 1
                    onValueEdited:{
                        if(com)
                        {
                            console.log("onValueEdited =",realValue)
                            com.radius = (realValue)
                        }
                    }
                    onPosValChanged:
                    {
                         realValue = posVal
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30 * screenScaleFactor

                CusText {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    Layout.fillWidth: true

                    enabled: !one_layer_only_box.checked
                    opacity: enabled ? 1 : 0.8

                    fontText: qsTr("Depth:")
                    fontColor: Constants.textColor
                    hAlign: 0
                }

                LeftToolSpinBox {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.preferredWidth: 153 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    enabled: !one_layer_only_box.checked
                    opacity: enabled ? 1 : 0.8
                    realStepSize:0.01
                    realFrom: 1
                    realTo: 100
                    font.pointSize: Constants.labelFontPointSize_9
                    realValue: com.radius
                    decimals : 2
                    bResetOtgValue : true
                    orgValue : com.depth.toFixed(2)
                    property var posVal : com ? com.depth.toFixed(2) : 1
                    onValueEdited:{
                        if(com)
                        {
                            com.depth = (realValue)
                        }
                    }
                    onPosValChanged:
                    {
                         realValue = posVal
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30 * screenScaleFactor

                CusText {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    Layout.fillWidth: true

                    fontText: qsTr("Direction:")
                    fontColor: Constants.textColor
                    hAlign: 0
                }

                CustomComboBox {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.preferredWidth: 153 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    model: ListModel {
                        ListElement { text: qsTr("Normal Direction") }
                        ListElement { text: qsTr("Parallel to Platform") }
                        ListElement { text: qsTr("Perpendicular to Screen") }
                    }
                    currentIndex: com.direction
                    onCurrentIndexChanged: {
                        com.direction = this.currentIndex;
                    }
                }
            }

            StyleCheckBox {
                id: one_layer_only_box

                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.topMargin: 10 * screenScaleFactor

                checked: com.oneLayerOnly
                text: qsTr("Pierce One Layer Only")

                onClicked: {
                    com.oneLayerOnly = this.checked
                }
            }
        }
    }
}
