import QtQuick.Layouts 1.13
import QtQuick 2.15
import QtQuick.Controls 1.4 as TMControl
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.5

import "../secondqml"
import "../qml"

Item {
    id: root
    readonly property string title: kernel_parameter_manager.currentMachineObject.extruderCount() === 1 ? qsTr("Material") : qsTr("Nozzle")
    property int nozzlenum: 1

    TMControl.TableView {
        property var radiusArray: [0.1, 0.3, 0.3, 0.3]
        property real itemWidth: root.width/tableView.columnCount
        property real itemHeight: 64 * screenScaleFactor
        id: tableView
        anchors.fill: parent
        backgroundVisible: false
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
        verticalScrollBarPolicy: contentHeight > height ? Qt.ScrollBarAlwaysOn : Qt.ScrollBarAlwaysOff
        clip: true
        style: TableViewStyle {
            frame: Rectangle {
                color: "transparent"
            }
        }

        headerDelegate: Rectangle{
//            color: Constants.right_panel_item_default_color
            width: tableView.itemWidth
            height: 40 * screenScaleFactor
            clip: true
            color: Constants.currentTheme ?"#79797a": "#1b1b1c"
            CusText {
                fontColor: "#FFFFFF"//Constants.textColor
                anchors.centerIn: parent
                fontWidth: tableView.itemWidth
                fontPointSize: Constants.labelFontPointSize_9
                fontText: styleData.value
            }
        }

        rowDelegate: Item {
            height: tableView.itemHeight
//            color: styleData.row % 2 == 0 ?  Constants.profileBtnColor : Constants.right_panel_item_default_color
        }

        TMControl.TableViewColumn {
            role: "ctColor"
            title: qsTr("Material")
            width: tableView.itemWidth - 10
            resizable: false
            movable: false
        }
        TMControl.TableViewColumn {
            role: "ctModel"
            title: qsTr("Model")
            width: tableView.itemWidth
            resizable: false
            movable: false
        }
        TMControl.TableViewColumn {
            role: "ctFlush"
            title: qsTr("Flush")
            width: tableView.itemWidth
            visible: !kernel_slice_model.extruderTableModel.isSingleColor
            resizable: false
            movable: false
        }
        TMControl.TableViewColumn {
            role: "ctFilamentTower"
            title: qsTr("FilamentTower")
            width: tableView.itemWidth
            visible: !kernel_slice_model.extruderTableModel.isSingleColor
            resizable: false
            movable: false
        }
        TMControl.TableViewColumn {
            role: "ctTotal"
            title: qsTr("Total")
            width: tableView.itemWidth
            resizable: false
            movable: false
        }
        model: kernel_slice_model.extruderTableModel
        itemDelegate: newCom
    }
    

    Component{
        id: newCom
        Item{
            width: tableView.itemWidth
            height: tableView.itemHeight

            ColorRectangle{
                width: 20 * screenScaleFactor
                height: 20 * screenScaleFactor
                color: styleData.value.getMaterialColor()
                visible: styleData.column === 0
                anchors.centerIn: parent

                bgColor: styleData.value.getMaterialColor()
                contentText: styleData.value.getMaterialIndex() + 1
            }

            Column{
                visible: styleData.column !== 0
                anchors.centerIn: parent
                CusText {
                    fontColor: "#FFFFFF"
                    fontWidth: tableView.itemWidth
                    fontPointSize: Constants.labelFontPointSize_9
                    fontText: styleData.value.getLength().toFixed(2) + "m"
                }

                CusText {
                    fontColor: "#FFFFFF"
                    fontWidth: tableView.itemWidth
                    fontPointSize: Constants.labelFontPointSize_9
                    fontText: styleData.value.getWeight().toFixed(2) + "g"
                }
            }
        }
    }
}
