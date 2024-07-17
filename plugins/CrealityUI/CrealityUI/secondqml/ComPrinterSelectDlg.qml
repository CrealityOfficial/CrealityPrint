import QtQuick 2.10
import QtQuick.Controls 2.12
import "../qml"
BasicDialog
{
    id: idDialog
    width: 500
    height: 160 //+ idOnLineItem.height
    titleHeight : 30
    property int spacing: 5
    title: qsTr("Upload Select")
    enum Status {
                Print,
                OnLine
            }
    property var taskStatus: ComPrinterSelectDlg.Status.Print

    property var myTableModel
    signal searchWifi()
    signal sendFileFunction()
    signal accept()
    signal cancel()
    Column
    {
        //leftPadding: 20
        topPadding: 50
        width: parent.width - 40
        height: parent.height - 50
        spacing: 20
        Item
        {
            x: (idDialog.width - 80 - 280 - 20)/2
            width: parent.width
            height:  30
            Row
            {
                anchors.fill: parent
                spacing: 20
                StyledLabel
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text : qsTr("Upload to")
                    width: 70
                    height: 28
                    verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignLeft
                    onTextChanged:
                    {
                        idExportSelectCmb.displayText = idCmb_export.get(idExportSelectCmb.currentIndex).name
                    }
                }
                BasicCombobox
                {
                    id : idExportSelectCmb
                    model: idCmb_export
                    width: 280
                    height: 28
                    currentIndex: 0
                    anchors.verticalCenter: parent.verticalCenter
                    onCurrentIndexChanged:
                    {
                        displayText = idCmb_export.get(idExportSelectCmb.currentIndex).name
                        if(currentIndex === 1)
                        {
                            searchWifi()
//                            idOnLineWIfi.show()
                            currentIndex = 0
                        }

                    }
                }
                ListModel
                {
                    id : idCmb_export
                    ListElement{name:qsTr("CrealityCloud")}
//                    ListElement{name:qsTr("Printer")}
                }
            }

        }
        Row
        {
            width: 250
            height: 32
            spacing: 10
            x: (idDialog.width - 125 - 125 - 10)/2
            BasicDialogButton
            {
                width : 125
                height : 32
                text: qsTr("OK")
                btnRadius:3
				btnBorderW:0
				defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                onClicked:
                {
                    accept()
                }
            }
            BasicDialogButton
            {
                width : 125
                height : 32
                text: qsTr("Cancel")
                btnRadius:3
				btnBorderW:0
				defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                onClicked:
                {
                    cancel()
                }
            }
        }
    }

//    ComOnLineWifiDlg
//    {
//        id : idOnLineWIfi
//        tableModel : myTableModel
//        onCancel :
//        {
//            idExportSelectCmb.currentIndex = 0
//        }
//        onSendFileCmd:
//        {
//            sendFileFunction()
////            close()
//        }
//        onRefresh:
//        {
//            searchWifi()
//        }
//    }
}
