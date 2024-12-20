import QtQuick 2.10
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import QtQuick.Controls 1.4
DockItem
{
    id: idDialog
    width: 750* screenScaleFactor//440
    height: 450* screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30* screenScaleFactor
    title: qsTr("Portions Copyright")
    property var fontSize:12
    ListModel{
        id:tableData
        ListElement{title: "Openssl"; text: "Copyright© 1995-1998 Eric Young (eay@cryptsoft.com)All rights reserved."}
        ListElement{title: "Qhull"; text: "Copyright© 1993-2020  C.B. Barber, Arlington, MA"}
        ListElement{title: "Clipper"; text: "Copyright© 2010-2015 Angus Johnson"}
        ListElement{title: "Zlib"; text:"Copyright© 1995-2005 Jean-loup Gailly, Mark Adler" }
        ListElement{title: "Curl"; text:"Copyright© 1996 - 2022, Daniel Stenberg, daniel@haxx.se, and many contributors" }
        ListElement{title: "Boost"; text: "Copyright© 1998-2005 Beman Dawes, David Abrahams; 2004 - 2007 Rene Rivera"}
        ListElement{title: "Nlopt"; text: "Copyright© 2007-2014 Massachusetts Institute of Technology"}
        ListElement{title: "Opencv"; text: "Copyright© 2022 , OpenCV team"}
        ListElement{title: "Nest2d"; text: "Copyright© 2007 Free Software Foundation, Inc."}
        ListElement{title: "Eigen";text:"Copyright© 2008 Gael Guennebaud <gael.guennebaud@inria.fr> \nCopyright© 2008 Benoit Jacob <jacob.benoit.1@gmail.com>"}
        ListElement{title: "Zip";text:"Copyright© 1990-2008 Info-ZIP"}
    }
    Column
    {
        y:60* screenScaleFactor//50
        width: parent.width
        height: parent.height-50* screenScaleFactor
        spacing:20* screenScaleFactor
        StyledLabel
        {
            width:600* screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 25*screenScaleFactor
            horizontalAlignment: Text.LeftToRight
            text: qsTr("The license agreement for all of the following programs (libraries) is part of the Application License Agreement:")
            wrapMode: Label.WordWrap
            font.pointSize: Constants.labelFontPointSize_10
            font.family: Constants.labelFontFamilyBold
        }
        Rectangle{
            width: parent.width - 50*screenScaleFactor
            height: parent.height - 130*screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            color: "transparent"
            radius: 5*screenScaleFactor
            TableView{
                anchors.fill: parent
                model: tableData
                verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                TableViewColumn{
                    role: "title";
                    title:  qsTr("library name")
                    width: 150*screenScaleFactor;
                    delegate: Text {
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: styleData.value
                        color: Constants.themeTextColor
                        font.family: Constants.labelFontFamilyBold
                    }
                }
                TableViewColumn{
                    role: "text";
                    title: qsTr("copyright")
                    width: 450*screenScaleFactor;
                    delegate: Text {
                        verticalAlignment: Text.AlignVCenter
                        text: styleData.value
                        color: Constants.themeTextColor
                        font.family: Constants.labelFontFamilyBold
                        wrapMode: Label.WordWrap
                    }
                }
                headerVisible: true
                headerDelegate: Rectangle {
                    height: 30*screenScaleFactor
                    color:  Constants.dialogTitleColor
                    Text {
                        text: styleData.value
                        color: Constants.themeTextColor
                        font.family: Constants.labelFontFamilyBold
                        anchors.centerIn: parent
                    }
                }
                rowDelegate: Rectangle {
                    height: 50*screenScaleFactor
                    color: styleData.row%2!==0 ?  Constants.profileBtnColor : Constants.themeColor
                }
            }
        }
        Row
        {
            id: idBtnrow
            spacing: 10* screenScaleFactor
            width: 395* screenScaleFactor
            height: 30* screenScaleFactor
            x: 290* screenScaleFactor
            BasicButton {
                id: basicComButton1
                width: 125* screenScaleFactor
                height: 28* screenScaleFactor
                btnRadius:height/2
                btnBorderW:0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                text: qsTr("Close")
                onSigButtonClicked:
                {
                    idDialog.close()
                }
            }
        }
    }
}
