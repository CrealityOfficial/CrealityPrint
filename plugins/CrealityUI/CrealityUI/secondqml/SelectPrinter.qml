import QtQuick 2.0
import QtQuick.Controls 2.12
import "../qml"
BasicGroupBox {
    id: control
//    title: qsTr("select printer")
//    width: 500
//    height:100
    implicitWidth: 260
    implicitHeight:280
    property alias model: idCmb.model
    property var currentCmbIndex: idCmb.currentIndex
    signal addPrinter()
    signal managerPrinter()

    property alias cmbCount: idCmb.count
    visible: width > 10 ? true:false
//by TCJ
    PrinterCommbox
    {
        id:idCmb
        visible: control.visible
        font.pointSize: Constants.labelFontPointSize_12
        width: control.width - 20
        height: 28
//        y:-10
        objectName: "machineCommbox"
        //this is test data,plz set self data
        onSigAddPrinter:
        {
           addPrinter()
        }
        onSigManagerPrinter:
        {
            console.log("count =" + count)
           managerPrinter()
        }
    }
    BasicButton/*BasicComButton*/
    {
       id: idAddPrinter
       y: idCmb.height + 10
       width: (control.width - 30 ) /2
       height:28
       text: qsTr("Add Printer")

       btnRadius:14
       //strTooptip : qsTr("Add Printer tooltip")
       visible: control.visible
       onSigButtonClicked:
       {
           addPrinter() //sigAddPrinter();
       }
    }
    BasicButton/*BasicComButton*/
    {
       y: idCmb.height + 10
       anchors.left: idAddPrinter.right
       anchors.leftMargin: 10
       width: (control.width - 30 ) /2
       height:28
       text: qsTr("Manage printer")//
       btnRadius:14
       //strTooptip : qsTr("Manage Printer tooltip")//
       visible: control.visible
       onSigButtonClicked:
       {
           managerPrinter()//sigManagerPrinter();
       }
    }
}
