import QtQuick 2.0
import QtQuick 2.0
import QtQuick.Controls 2.12
BasicDialog
{
    id: idDialog
    width: 300
    height: 220
    //titleHeight : 30
    property alias text: element.text
    signal accepted()
    signal accepted_cancel()
    //
    title: qsTr("Message")

    Rectangle
    {
        x:15
        y: 52
        width: 271
        height: 117
        border.width: 1
        radius: 5
        color: "#303030"//"transparent"
        Text {//by TCJ
            id: element
            /*anchors.centerIn: parent*/
            anchors.fill: parent
            //anchors.leftMargin: 5
            verticalAlignment: Text.AlignVCenter//anchors.topMargin: 5
            horizontalAlignment:Text.AlignHCenter//anchors.rightMargin: 5
            font.pointSize: Constants.labelFontPointSize_16
            wrapMode: Text.WordWrap
            color: "white"
        }

    }

    BasicDialogButton
    {
        id : idADD
        x: 41
        y: 180
        width: 100
        height: 32
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12/*8*/
        anchors.right: parent.right
        anchors.rightMargin: 159
        text: qsTr("Confirm")
        onSigButtonClicked:
        {
            accepted();
            //idDialog.close();
        }
    }
    BasicDialogButton
    {
        id : idclose
        x: 147
        y: 180
        width: 100
        height: 32
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12/*8*/
        anchors.right: parent.right
        anchors.rightMargin: 53
        text: qsTr("Cancle")
        onSigButtonClicked:
        {
            //accepted();
            accepted_cancel()
            idDialog.close();
        }
    }
}
