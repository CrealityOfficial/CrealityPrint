import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0 as QQC2
import CrealityUI 1.0
import "qrc:/CrealityUI"
import ".."
import "../qml"

BasicDialog{
    id: idLicenseDlg
    width: 760
    height: 520
    title: qsTr("Creality Cloud protect designer's copyrighted works")
    // titleColor: "#333333"
    // titleBackground: "#D7D7D7"
    contentBackground: "#FFFFFF"

    BasicScrollView{
        anchors.top: parent.top
        anchors.topMargin: 30+30+5
        anchors.left: parent.left
        anchors.leftMargin: 5+21
        width: parent.width-10 - 21
        height: parent.height-titleHeight-10-30-30
        clip : true
        Column{
            width: 708//parent.width-21
            spacing: 30
            StyledLabel{
                width:parent.width
                height:40
                wrapMode: Text.WordWrap
                color: "#333333"
                text: qsTr("In Creality Cloud, your original work (inc 3D prints & articles) is restrictedly protected under Creative Commons Licenses 4.0. If you find any breach of agreement, please contact us to discuss further action.")
                font.pointSize: Constants.labelFontPointSize_12
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC BY"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("This license allows reusers to distribute, remix, adapt, and build upon the material in any medium or format, so long as attribution is given to the creator. The license allows for commercial use.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by.png"
                }
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC BY-SA"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("This license allows reusers to distribute, remix, adapt, and build upon the material in any medium or format, so long as attribution is given to the creator. The license allows for commercial use. If you remix, adapt, or build upon the material, you must license the modified material under identical terms.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by_sa.png"
                }
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC BY-NC"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("This license allows reusers to distribute, remix, adapt, and build upon the material in any medium or format for noncommercial purposes only, and only so long as attribution is given to the creator.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by_nc.png"
                }
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC BY-NC-SA"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("This license allows reusers to distribute, remix, adapt, and build upon the material in any medium or format for noncommercial purposes only, and only so long as attribution is given to the creator. If you remix, adapt, or build upon the material, you must license the modified material under identical terms.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by_nc_sa.png"
                }
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC BY-ND"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("This license allows reusers to copy and distribute the material in any medium or format in unadapted form only, and only so long as attribution is given to the creator. The license allows for commercial use.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by_nd.png"
                }
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC BY-NC-ND"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("This license allows reusers to copy and distribute the material in any medium or format in unadapted form only, for noncommercial purposes only, and only so long as attribution is given to the creator.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by_nc_nd.png"
                }
            }
            Row{
                width: parent.width
                spacing: 20
                Column{
                    width: parent.width - 172 - 20
                    height: 60
                    spacing: 5
                    StyledLabel{
                        width: parent.width
                        height: 14
                        wrapMode: Text.WordWrap
                        color: "#333333"
                        text: "CC0"
                        font.pointSize: Constants.labelFontPointSize_16
                        font.weight: "Bold"
                    }
                    StyledLabel{
                        width: parent.width
                        height: parent.height-14-5
                        wrapMode: Text.WordWrap
                        color: "#666666"
                        text: qsTr("(aka CC Zero) is a public dedication tool, which allows creators to give up their copyright and put their works into the worldwide public domain.")
                        font.pointSize: Constants.labelFontPointSize_10
                    }
                }
                Image{
                    width: 172
                    height: 60
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_cc0.png"
                }
            }
        }
    }
}
