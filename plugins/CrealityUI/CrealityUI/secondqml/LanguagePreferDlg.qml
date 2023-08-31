import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import ".."
import "../qml"
//import CrealityUI 1.0
//import "qrc:/CrealityUI"
BasicDialog
{
    id: idDialog
    width: 450
    height: 200//250
    titleHeight : 30
    property int spacing: 5
    title: qsTr("Prefer Set")
    //property alias currentLanguageIndex: idLanguage.currentIndex
    property alias timeInterval : idTimeInterval.text

    property alias cmbMoneyCurrentIndex: idMonetary.currentIndex
    //signal sigValChanged(var key,var value)
    signal saveLanguageConfig()
    //signal sigCurrentIndex(int index)
    Item
    {
        id: idGroup
        x:20
        y:titleHeight + 20
//        title: qsTr("General parameters")
        width: 400
        height:80//130
        Grid
        {
            columns: 2
            rows: 3
            rowSpacing: 10
            columnSpacing: 10
            width: 350
            height: parent.height
            anchors.horizontalCenter: parent.horizontalCenter
//            Row
//            {
                /*StyledLabel {
//                    Layout.column: 1
                    width: 180
                    height : 30
                    text: qsTr("Language: ")
                    horizontalAlignment: Qt.AlignRight

                }
                BasicCombobox {
                    id:idLanguage
//                    Layout.column: 2
                    height: 30
                    width: 160
                    currentIndex: 1
                    model: ListModel {
                        id: cbItems
//                        ListElement { text: qsTr("العربية/Arab"); }  //ar.ts
//                        ListElement { text: qsTr("Deutsch/German");}  //de.ts
//                        ListElement { text: qsTr("English");}    //en.ts
//                        ListElement { text: qsTr("Français/French");}   //fr.ts
//                        ListElement { text: qsTr("Hrvatska/Croatian");} //hr.ts
//                        ListElement { text: qsTr("Italiano/Italian");}  //it.ts
//                        ListElement { text: qsTr("日本語/Japanese");}   //jp.ts
//                        ListElement { text: qsTr("한국어/Korean");}  //ko.ts
//                        ListElement { text: qsTr("Portugal/Portugal");}  //pl.ts
//                        ListElement { text: qsTr("Pусский язык/Russian");} //ru.ts
//                        ListElement { text: qsTr("Türkçe/Turkish");}    //tr.ts
//                        ListElement { text: qsTr("简体中文/Simplified Chinese");}  //zh_CN.ts
//                        ListElement { text: qsTr("繁体中文/Traditional Chinese");}  //zh_Tw.ts
                        ListElement { text: qsTr("English");}    //en.ts
                        ListElement { text: qsTr("简体中文");}  //zh_CN.ts
                        ListElement { text: qsTr("繁体中文");} //zh_TW.ts
                    }
                    onCurrentIndexChanged:
                    {
                        //currentText return the last text
                        console.log("idLanguage =" + idLanguage.textAt(currentIndex))
                        sigCurrentIndex(currentIndex);
                    }
                }*/
//            }
//            Row
//            {
                StyledLabel {
                    width: 180
                    text: qsTr("Monetary Unit") + ": "
                    verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignRight
                    height : 30
                }
                BasicCombobox {
                    id:idMonetary
//                    implicitHeight:30
//                    implicitWidth: 120
                    height: 30
                    width: 160
                    currentIndex: 0
                    model: ListModel {
                        id: cbItems2
                        ListElement { text: "¥";}
                        ListElement { text: "$"; }
                    }
                    onCurrentIndexChanged:
                    {
                        console.log("idMonetary =" + idMonetary.textAt(currentIndex))
                    }
                }
//            }
//            Row
//            {
//                spacing: 20
                StyledLabel {

                    text: qsTr("Save Project Time Interval") + ": "
//                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Qt.AlignRight
                    verticalAlignment: Qt.AlignVCenter
                    height : 30
                    width : 180
                }
                BasicDialogTextField
                {
                    id :idTimeInterval
                    height : 30
                    width : 160
                    text: "10.0"
                    unitChar: "min"
                }
            }

    }
    Grid
    {
        columns: 2
        anchors.top: idGroup.bottom
        anchors.topMargin: 10
        spacing: 10
        height: 40
        width: 230
        anchors.horizontalCenter: parent.horizontalCenter
        BasicDialogButton
        {
            id : idSave
            width:110
            height: 30
            text: qsTr("Save")
            btnRadius:3
            btnBorderW:0
            defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
            onSigButtonClicked:
            {
                console.log("idTimeInterval.text = " + idTimeInterval.text)
                if(idTimeInterval.text === "")
                {
                    warringdlgname.text = qsTr("Save Project Time Interval can not be empty")
                    messageDialog.visible = true
                    return
                }
                else if(idTimeInterval.text === "0")
                {
                    warringdlgname.text = qsTr("Save Project Time Interval can not be 0")
                    messageDialog.visible = true
                    return
                }
                saveLanguageConfig();
                idDialog.close();
            }
        }
        BasicDialogButton
        {
            id : idCancel
            width:110
            height: 30
            text: qsTr("Cancel")
            btnRadius:3
            btnBorderW:0
            defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
            onSigButtonClicked:
            {
                idDialog.close();
            }
        }
    }

    BasicDialog
    {
        id: messageDialog
        width: 400
        height: 200
        titleHeight : 30
        title: qsTr("Message")
        visible: false
        Rectangle{
            anchors.centerIn: parent
            width: parent.width/2
            height: parent.height/2
            color: "#535353"
            Text {
                id: warringdlgname
                anchors.centerIn: parent
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                text: qsTr("Save Project Time Interval can not be empty")
                font.pointSize: Constants.labelFontPointSize_6
                color:"white"
            }
        }
    }
}

