import QtQuick 2.0
import QtQuick.Layouts 1.13
import CrealityUI 1.0
import QtQml.Models 2.13
import "qrc:/CrealityUI"

BasicDialogV4
{
    width: 500* screenScaleFactor
    height: 400* screenScaleFactor
    maxBtnVis: false

    title: qsTr("Mouse Operation Function")
    bdContentItem:Item{
        id: mouseOptDialog
//        color: "#272a2d"
//        radius: 5* screenScaleFactor

        Column{
            id: col
            spacing: 20* screenScaleFactor
            anchors.centerIn: parent
            anchors.fill: parent
            anchors.margins: 32* screenScaleFactor
            Repeater{
                model: ListModel{
                    ListElement{imgSource:"qrc:/UI/photo/mouseHoldRight.png"; textContent:qsTr("Rotate operation: Press and hold the right mouse button")}
                    ListElement{imgSource:"qrc:/UI/photo/mouseHoldLeft.png"; textContent:qsTr("Move  operation: press and hold the middle mouse button")}
                    ListElement{imgSource:"qrc:/UI/photo/mouseClickRight.png"; textContent:qsTr("Zoom operation: Slide the mouse wheel")}
                    ListElement{imgSource:"qrc:/UI/photo/mouseScroll.png"; textContent:qsTr("Model radio selection operation: Click the left mouse button")}
                    ListElement{imgSource:"qrc:/UI/photo/mouseScroll.png"; textContent:qsTr("Model multiple selection operation: Ctrl+Click the left mouse button")}
                    ListElement{imgSource:"qrc:/UI/photo/mouseScroll.png"; textContent:qsTr("Model box selection operation: Press and hold the left mouse button")}
                    ListElement{imgSource:"qrc:/UI/photo/mouseScroll.png"; textContent:qsTr("Right-click menu operation: Click the right mouse button")}
                }
//                       ListModel{
//                    ListElement{imgSource:""; textContent:qsTr("Rotate operation: Press and hold the right mouse button")}
//                    ListElement{imgSource:""; textContent:qsTr("Move  operation: press and hold the middle mouse button")}
//                    ListElement{imgSource:""; textContent:qsTr("Zoom operation: Slide the mouse wheel")}
//                    ListElement{imgSource:""; textContent:qsTr("Model radio selection operation: Click the left mouse button")}
//                    ListElement{imgSource:""; textContent:qsTr("Model multiple selection operation: Ctrl+Click the left mouse button")}
//                    ListElement{imgSource:""; textContent:qsTr("Model box selection operation: Press and hold the left mouse button")}
//                    ListElement{imgSource:""; textContent:qsTr("Right-click menu operation: Click the right mouse button")}
//                }

                delegate: Item{
                    implicitWidth: col.width
                    implicitHeight: mouseImg.height
                    Image{
                        id:mouseImg
                        visible: false
                        sourceSize.width: 20* screenScaleFactor
                        sourceSize.height: 20* screenScaleFactor
                        anchors.left: parent.left
                        source: imgSource
                    }

                    CusText{
                        id:mouseOptDetail
                        hAlign: 0* screenScaleFactor
                        fontWidth: 450* screenScaleFactor
                        fontColor: Constants.right_panel_item_text_default_color
                        font.pointSize: Constants.labelFontPointSize_10
                        anchors.left: parent.left
                        anchors.leftMargin: 5* screenScaleFactor
                        fontText: textContent
                    }
                }
            }
        }
    }
}
