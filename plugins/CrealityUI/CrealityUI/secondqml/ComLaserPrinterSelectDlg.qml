import QtQuick 2.10
import QtQuick.Controls 2.12
import "../qml"
BasicDialog
{
    id: idDialog
    width: 290
    height: 150 //+ idOnLineItem.height
    titleHeight : 30
    property int spacing: 5
    title: qsTr("Print Select")
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
        leftPadding: 20
        topPadding: 50
        width: parent.width - 40
        height: parent.height-50
        spacing: 10
        Item
        {

            width: parent.width
            height:  30
            Row
            {
                anchors.fill: parent
                spacing: 10
                StyledLabel
                {
                    anchors.verticalCenter: parent.verticalCenter
                    text : qsTr("Export to")
                    onTextChanged:
                    {
                        idExportSelectCmb.displayText = idCmb_export.get(idExportSelectCmb.currentIndex).name
                    }
                }
                BasicCombobox
                {
                    id : idExportSelectCmb
                    model: idCmb_export
                    width: 180
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
                    ListElement{name:qsTr("Local")}
                    ListElement{name:qsTr("Printer")}
                }
            }

        }
        Row
        {
            width: 250
            height: 32
            spacing: 10
            BasicDialogButton
            {
                width : 120
                height : 32
                text: qsTr("OK")
                btnRadius:3
				btnBorderW:0
				defaultBtnBgColor: "#787878"
                onClicked:
                {
                    accept()
                }
            }
            BasicDialogButton
            {
                width : 120
                height : 32
                text: qsTr("Cancel")
                btnRadius:3
				btnBorderW:0
				defaultBtnBgColor: "#787878"
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
