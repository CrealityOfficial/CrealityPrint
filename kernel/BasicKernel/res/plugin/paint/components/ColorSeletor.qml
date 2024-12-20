import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQuick.Controls.Styles 1.4
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Rectangle {
    id: root
    color: "transparent"
    property var selectedColor: Qt.rgba(0.0, 0.0, 0.0, 1.0)
    property var selectedColorIndex: 0
    property var title: qsTr("Choose Filament")
    property var colorCount: r.count

    signal updateColorsList()

    function setSelectedColorIndex(index) {
        let originIndex = selectedColorIndex
        if (originIndex == index) 
            return;

        selectedColorIndex = index

        if (selectedColorIndex >= colorCount) {
            selectedColorIndex = originIndex
        }

        if (selectedColorIndex >= colorCount) {
            selectedColorIndex = 0
        }
        idColorSeletor.update();
    }

    function update() {
        var button = r.itemAt(selectedColorIndex)
        if (!button.checked) {
            button.checked = true
        }
    }

    CusText {
        id: idTitle
        fontText: title
        hAlign: 0
        fontColor: Constants.textColor
        fontPointSize: Constants.labelFontPointSize_9
    }

    Flow {
        id: idFlow
        anchors.top: idTitle.bottom
        anchors.topMargin: 6
        width: parent.width
        spacing: 2
        // columns: 8 
        property bool complete: false
        
        onPositioningComplete: {
            complete = true
            updateColorsList()
        }

        Repeater {
            id: r
            // model: extrudersModel
            model: kernel_parameter_manager.currentMachineObject.extrudersModel

            delegate: CusImglButton {
                id: btn
                checkable: true
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor
                autoExclusive: true
                
                cusTooltip.text: (index < 9 ? index + 1 : "")

                function adapteTextColor(bgColor) {
                    var brightness = 0.299 * bgColor.r + 0.587 * bgColor.g + 0.114 * bgColor.b
                    return brightness > 0.5 ? "black" : "white"
                }

                onCheckedChanged: {
                    root.selectedColor = model.extruderColor
                    root.selectedColorIndex = index
                }

                background: Rectangle {
                    anchors.fill: parent
                
                    border.color: btn.checked ? Qt.rgba(0.09, 0.80, 0.37, 1.0) : "transparent"
                    border.width: 5
                    radius: 3
                    color: "transparent"

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 3
                        color: model.extruderColor
                        // color: extrudersModel[index].extruderColor
                        border.color: "#2B2B2D"
                        border.width: 1
                        radius: 3

                        onColorChanged: {
                            if (idFlow.complete) {
                                root.updateColorsList()
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: index + 1
                        color: adapteTextColor(model.extruderColor)
                        // color: adapteTextColor(extrudersModel[index].extruderColor)
                        font.pointSize: Constants.labelFontPointSize_9
                    }

                }

                Component.onCompleted: {
                    var data = kernel_parameter_manager.currentMachineObject.extrudersModel
                    let count = data.rowCount()
                    
                    if (root.selectedColorIndex >= count)
                        root.selectedColorIndex = count - 1

                    if (index === root.selectedColorIndex) {
                        root.selectedColor = model.extruderColor
                        btn.checked = true
                    }

                }
            }
        }
    }
}