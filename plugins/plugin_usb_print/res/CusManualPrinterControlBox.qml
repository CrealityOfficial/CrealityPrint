// Copyright (c) 2019 Ultimaker B.V.
// Cura is released under the terms of the LGPLv3 or higher.

import ".."
import "../qml"
import "../secondqml"
import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3

Item {
    property var printerModel: null
    property var activePrintJob: printerModel != null ? printerModel.activePrintJob : null
    property var currentDistance: 1

    implicitWidth: parent.width
    implicitHeight: 140

    Column {
        x: 15
        enabled: {
            if (printerModel == null)
                return false;

            //Not allowed to do anything.
            if (activePrintJob == null)
                return true;

            if (activePrintJob.state == "printing" || activePrintJob.state == "resuming" || activePrintJob.state == "pausing" || activePrintJob.state == "error" || activePrintJob.state == "offline")
                return false;

            //Printer is in a state where it can't react to manual control
            return true;
        }

        Row {
            height: 140
            spacing: 10

            GridLayout {
                columns: 3
                rows: 3
                rowSpacing: 10
                columnSpacing: 10

                CusSkinButton_Image {
                    Layout.row: 0
                    Layout.column: 1
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height
                    btnImgNormal: Constants.printMoveAxisYUpImg //"qrc:/UI/photo/print_move_axis_y+.png"
                    btnImgHovered: Constants.printMoveAxisYUp_HImg //"qrc:/UI/photo/print_move_axis_y+_h.png"
                    btnImgPressed: Constants.printMoveAxisYUp_CImg //"qrc:/UI/photo/print_move_axis_y+_c.png"
                    btnImgDisbaled: Constants.printMoveAxisYUpImg //"qrc:/UI/photo/print_move_axis_y+.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.moveHead(0, currentDistance, 0);
                    }
                }

                CusSkinButton_Image {
                    Layout.row: 1
                    Layout.column: 0
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height
                    btnImgNormal: Constants.printMoveAxisXDownImg //"qrc:/UI/photo/print_move_axis_x-.png"
                    btnImgHovered: Constants.printMoveAxisXDown_HImg //"qrc:/UI/photo/print_move_axis_x-_h.png"
                    btnImgPressed: Constants.printMoveAxisXDown_CImg //"qrc:/UI/photo/print_move_axis_x-_c.png"
                    btnImgDisbaled: Constants.printMoveAxisXDownImg //"qrc:/UI/photo/print_move_axis_x-.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.moveHead(-currentDistance, 0, 0);
                    }
                }

                CusSkinButton_Image {
                    Layout.row: 1
                    Layout.column: 2
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height
                    btnImgNormal: Constants.printMoveAxisXUpImg //"qrc:/UI/photo/print_move_axis_x+.png"
                    btnImgHovered: Constants.printMoveAxisXUp_HImg //"qrc:/UI/photo/print_move_axis_x+_h.png"
                    btnImgPressed: Constants.printMoveAxisXUp_CImg //"qrc:/UI/photo/print_move_axis_x+_c.png"
                    btnImgDisbaled: Constants.printMoveAxisXUpImg //"qrc:/UI/photo/print_move_axis_x+.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.moveHead(currentDistance, 0, 0);
                    }
                }

                CusSkinButton_Image {
                    Layout.row: 2
                    Layout.column: 1
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height
                    btnImgNormal: Constants.printMoveAxisYDownImg //"qrc:/UI/photo/print_move_axis_y-.png"
                    btnImgHovered: Constants.printMoveAxisYDown_HImg //"qrc:/UI/photo/print_move_axis_y-_h.png"
                    btnImgPressed: Constants.printMoveAxisYDown_CImg //"qrc:/UI/photo/print_move_axis_y-_c.png"
                    btnImgDisbaled: Constants.printMoveAxisYDownImg //"qrc:/UI/photo/print_move_axis_y-.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.moveHead(0, -currentDistance, 0);
                    }
                }

                CusSkinButton_Image {
                    Layout.row: 1
                    Layout.column: 1
                    Layout.preferredWidth: width
                    Layout.preferredHeight: height
                    btnImgNormal: Constants.printMoveAxisZeroImg //"qrc:/UI/photo/print_move_axis_zero.png"
                    btnImgHovered: Constants.printMoveAxisZero_HImg //"qrc:/UI/photo/print_move_axis_zero_h.png"
                    btnImgPressed: Constants.printMoveAxisZero_CImg //"qrc:/UI/photo/print_move_axis_zero_c.png"
                    btnImgDisbaled: Constants.printMoveAxisZeroImg //"qrc:/UI/photo/print_move_axis_zero.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.homeHead();
                    }
                }

            }

            Column {
                spacing: 10

                CusSkinButton_Image {
                    btnImgNormal: Constants.printMoveAxisZUpImg //"qrc:/UI/photo/print_move_axis_z+.png"
                    btnImgHovered: Constants.printMoveAxisZUp_HImg //"qrc:/UI/photo/print_move_axis_z+_h.png"
                    btnImgPressed: Constants.printMoveAxisZUp_CImg //"qrc:/UI/photo/print_move_axis_z+_c.png"
                    btnImgDisbaled: Constants.printMoveAxisZUpImg //"qrc:/UI/photo/print_move_axis_z+.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.moveHead(0, 0, currentDistance);
                    }
                }

                CusSkinButton_Image {
                    btnImgNormal: Constants.printMoveAxisZeroImg //"qrc:/UI/photo/print_move_axis_zero.png"
                    btnImgHovered: Constants.printMoveAxisZero_HImg //"qrc:/UI/photo/print_move_axis_zero_h.png"
                    btnImgPressed: Constants.printMoveAxisZero_CImg //"qrc:/UI/photo/print_move_axis_zero_c.png"
                    btnImgDisbaled: Constants.printMoveAxisZeroImg //"qrc:/UI/photo/print_move_axis_zero.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.homeBed();
                    }
                }

                CusSkinButton_Image {
                    btnImgNormal: Constants.printMoveAxisZDownImg //"qrc:/UI/photo/print_move_axis_z-.png"
                    btnImgHovered: Constants.printMoveAxisZDown_HImg //"qrc:/UI/photo/print_move_axis_z-_h.png"
                    btnImgPressed: Constants.printMoveAxisZDown_CImg //"qrc:/UI/photo/print_move_axis_z-_c.png"
                    btnImgDisbaled: Constants.printMoveAxisZDownImg //"qrc:/UI/photo/print_move_axis_z-.png"
                    width: height
                    height: 40
                    onClicked: {
                        printerModel.moveHead(0, 0, -currentDistance);
                    }
                }

            }

            ColumnLayout {
                Layout.leftMargin: 10
                spacing: 10
                height: 140

                Item {
                    Layout.fillHeight: true
                }

                BasicDialogButton {
                    text: "0.1mm"
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 40
                    selectedBtnBgColor:"#1E9BE2"
                    bottonSelected : currentDistance === 0.1
                    onClicked: {
                        currentDistance = 0.1;
                    }
                }

                BasicDialogButton {
                    text: "10mm"
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 40
                    selectedBtnBgColor:"#1E9BE2"
                    bottonSelected : currentDistance === 10
                    onClicked: {
                        currentDistance = 10;
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

            }

            ColumnLayout {
                spacing: 10
                height: 140

                Item {
                    Layout.fillHeight: true
                }

                BasicDialogButton {
                    text: "1mm"
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 40
                    selectedBtnBgColor:"#1E9BE2"
                    bottonSelected : currentDistance === 1
                    onClicked: {
                        currentDistance = 1;
                    }
                }

                BasicDialogButton {
                    text: "100mm"
                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 40
                    selectedBtnBgColor:"#1E9BE2"
                    bottonSelected : currentDistance === 100
                    onClicked: {
                        currentDistance = 100;
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

            }

        }

    }

}
