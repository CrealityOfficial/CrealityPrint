import QtQuick 2.10
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"
import "../components"

Window
{
    property var tipDialog: null

    id: root
    color: "transparent"
    width: 600 * screenScaleFactor
    height: 450 * screenScaleFactor
    flags: Qt.FramelessWindowHint | Qt.Dialog

    enum LoginType {
        Phone_Type = 1,
        Email_Type = 2,
        Qrcode_Type = 3
    }

    property int timeCnt: 60
    property int curLoginType: -1
    property bool showQuickLogin: false
    property bool globalAutoLogin: true
    property string phoneRegExp: "^[0-9]{1,15}$"
    property string emailRegExp: "^([a-zA-Z0-9_\\-\\.]+)@([a-zA-Z0-9_\\-\\.]+)\\.([a-zA-Z]{2,30})$"
    property string passwordExp: showQuickLogin ? "^[\\d]{4}$" : "^.{6,32}$"

    onCurLoginTypeChanged: {
        showQuickLogin = false
        itemSwipeView.setCurrentIndex(curLoginType - 1)
    }

    onVisibleChanged: {
        if (visible) {
            cxkernel_cxcloud.accountService.startAutoRefreshQrcode()
        } else {
            cxkernel_cxcloud.accountService.stopAutoRefreshQrcode()
        }
    }

    Connections {
        target: cxkernel_cxcloud.accountService

        onLoginFailed: function(message) {
            showErrorMsg(message)
        }

        onQrcodeRefreshed: {
            idQrCodeImage.source = ""
            idQrCodeImage.source = "image://login_qrcode_image_provider"
        }
    }

    function resetBtnSelect() {
        if(serverControl.currentIndex) emailBtn.checked = true
        else if(!serverControl.currentIndex) phoneBtn.checked = true
    }

    function showErrorMsg(errormsg) {
        idErrorMsg.text = qsTr(errormsg)
    }

    function phoneLoginClicked() {
        var account = idAccountPhoneEdit.text
        var password = idPasswordPhoneEdit.text
        var areaCode = idAreaCodeCombox.displayText.substring(1)

        if(showQuickLogin)
        {
            cxkernel_cxcloud.accountService.loginByMobileVerifyCode(
                account, areaCode, password, globalAutoLogin)
        }
        else {
            cxkernel_cxcloud.accountService.loginByMobilePassword(
                account, areaCode, password, globalAutoLogin)
        }
    }

    function emailLoginClicked() {
        cxkernel_cxcloud.accountService.loginByEmailPassword(
            idAccountEmailEdit.text, idPasswordEmailEdit.text, globalAutoLogin)
    }

    Timer {
        id: countDown
        repeat: true
        interval: 1000
        triggeredOnStart: true

        onTriggered: {
            timeCnt -= 1
            getCodeButton.enabled = false
            getCodeText.text = "(" + timeCnt + "s)"

            if(timeCnt < 0)
            {
                countDown.stop()
                getCodeButton.enabled = true
                getCodeText.text = qsTr("Get Code")
            }
        }
    }
    ListModel {
        id: serverList
    }
    ListModel {
        id:areaCodeList
        ListElement{key:"93";modelData : qsTr("Afghanistan");}
        ListElement{key:"335";modelData : qsTr("Albania");}
        ListElement{key:"213";modelData : qsTr("Algeria");}
        ListElement{key:"376";modelData : qsTr("Andorra");}
        ListElement{key:"0244";modelData : qsTr("Angola");}
        ListElement{key:"1254";modelData : qsTr("Anguilla");}
        ListElement{key:"1268";modelData : qsTr("Antigua and Barbuda");}
        ListElement{key:"54";modelData : qsTr("Argentina");}
        ListElement{key:"374";modelData : qsTr("Armenia");}
        ListElement{key:"247";modelData : qsTr("Ascension");}
        ListElement{key:"61";modelData : qsTr("Australia");}
        ListElement{key:"43";modelData : qsTr("Austria");}
        ListElement{key:"994";modelData : qsTr("Azerbaijan");}
        ListElement{key:"1242";modelData : qsTr("Bahamas");}
        ListElement{key:"973";modelData : qsTr("Bahrain");}
        ListElement{key:"880";modelData : qsTr("Bangladesh");}
        ListElement{key:"1246";modelData : qsTr("Barbados");}
        ListElement{key:"375";modelData : qsTr("Belarus");}
        ListElement{key:"32";modelData : qsTr("Belgium");}
        ListElement{key:"501";modelData : qsTr("Belize");}
        ListElement{key:"229";modelData : qsTr("Benin");}
        ListElement{key:"1441";modelData : qsTr("Bermuda Is");}
        ListElement{key:"591";modelData : qsTr("Bolivia");}
        ListElement{key:"267";modelData : qsTr("Botswana");}
        ListElement{key:"55";modelData : qsTr("Brazil");}
        ListElement{key:"673";modelData : qsTr("Brunei");}
        ListElement{key:"359";modelData : qsTr("Bulgaria");}
        ListElement{key:"226";modelData : qsTr("Burkina Faso");}
        ListElement{key:"95";modelData : qsTr("Burma");}
        ListElement{key:"257";modelData : qsTr("Burundi");}
        ListElement{key:"237";modelData : qsTr("Cameroon");}
        ListElement{key:"1";modelData : qsTr("Canada");}
        ListElement{key:"1345";modelData : qsTr("Cayman Is");}
        ListElement{key:"236";modelData : qsTr("Central African Republic");}
        ListElement{key:"235";modelData : qsTr("Chad");}
        ListElement{key:"56";modelData : qsTr("Chile");}
        ListElement{key:"86";modelData : qsTr("China");}
        ListElement{key:"57";modelData : qsTr("Colombia");}
        ListElement{key:"242";modelData : qsTr("Congo");}
        ListElement{key:"682";modelData : qsTr("Cook Is");}
        ListElement{key:"506";modelData : qsTr("Costa Rica");}
        ListElement{key:"53";modelData : qsTr("Cuba");}
        ListElement{key:"357";modelData : qsTr("Cyprus");}
        ListElement{key:"420";modelData : qsTr("Czech Republic");}
        ListElement{key:"45";modelData : qsTr("Denmark");}
        ListElement{key:"253";modelData : qsTr("Djibouti");}
        ListElement{key:"1890";modelData : qsTr("Dominica Rep");}
        ListElement{key:"503";modelData : qsTr("EI Salvador");}
        ListElement{key:"593";modelData : qsTr("Ecuador");}
        ListElement{key:"20";modelData : qsTr("Egypt");}
        ListElement{key:"372";modelData : qsTr("Estonia");}
        ListElement{key:"251";modelData : qsTr("Ethiopia");}
        ListElement{key:"679";modelData : qsTr("Fiji");}
        ListElement{key:"358";modelData : qsTr("Finland");}
        ListElement{key:"33";modelData : qsTr("France");}
        ListElement{key:"594";modelData : qsTr("French Guiana");}
        ListElement{key:"689";modelData : qsTr("French Polynesia");}
        ListElement{key:"241";modelData : qsTr("Gabon");}
        ListElement{key:"220";modelData : qsTr("Gambia");}
        ListElement{key:"995";modelData : qsTr("Georgia");}
        ListElement{key:"49";modelData : qsTr("Germany");}
        ListElement{key:"233";modelData : qsTr("Ghana");}
        ListElement{key:"350";modelData : qsTr("Gibraltar");}
        ListElement{key:"30";modelData : qsTr("Greece");}
        ListElement{key:"1809";modelData : qsTr("Grenada");}
        ListElement{key:"1671";modelData : qsTr("Guam");}
        ListElement{key:"502";modelData : qsTr("Guatemala");}
        ListElement{key:"224";modelData : qsTr("Guinea");}
        ListElement{key:"592";modelData : qsTr("Guyana");}
        ListElement{key:"509";modelData : qsTr("Haiti");}
        ListElement{key:"504";modelData : qsTr("Honduras");}
        ListElement{key:"852";modelData : qsTr("Hong Kong");}
        ListElement{key:"36";modelData : qsTr("Hungary");}
        ListElement{key:"354";modelData : qsTr("Iceland");}
        ListElement{key:"91";modelData : qsTr("India");}
        ListElement{key:"62";modelData : qsTr("Indonesia");}
        ListElement{key:"98";modelData : qsTr("Iran");}
        ListElement{key:"964";modelData : qsTr("Iraq");}
        ListElement{key:"353";modelData : qsTr("Ireland");}
        ListElement{key:"972";modelData : qsTr("Israel");}
        ListElement{key:"39";modelData : qsTr("Italy");}
        ListElement{key:"225";modelData : qsTr("Ivory Coast");}
        ListElement{key:"1876";modelData : qsTr("Jamaica");}
        ListElement{key:"81";modelData : qsTr("Japan");}
        ListElement{key:"962";modelData : qsTr("Jordan");}
        ListElement{key:"855";modelData : qsTr("Kampuchea (Cambodia )");}
        ListElement{key:"327";modelData : qsTr("Kazakstan");}
        ListElement{key:"254";modelData : qsTr("Kenya");}
        ListElement{key:"82";modelData : qsTr("Korea");}
        ListElement{key:"965";modelData : qsTr("Kuwait");}
        ListElement{key:"331";modelData : qsTr("Kyrgyzstan");}
        ListElement{key:"856";modelData : qsTr("Laos");}
        ListElement{key:"371";modelData : qsTr("Latvia");}
        ListElement{key:"961";modelData : qsTr("Lebanon");}
        ListElement{key:"266";modelData : qsTr("Lesotho");}
        ListElement{key:"231";modelData : qsTr("Liberia");}
        ListElement{key:"218";modelData : qsTr("Libya");}
        ListElement{key:"4175";modelData : qsTr("Liechtenstein");}
        ListElement{key:"370";modelData : qsTr("Lithuania");}
        ListElement{key:"352";modelData : qsTr("Luxembourg");}
        ListElement{key:"853";modelData : qsTr("Macao");}
        ListElement{key:"261";modelData : qsTr("Madagascar");}
        ListElement{key:"265";modelData : qsTr("Malawi");}
        ListElement{key:"60";modelData : qsTr("Malaysia");}
        ListElement{key:"960";modelData : qsTr("Maldives");}
        ListElement{key:"223";modelData : qsTr("Mali");}
        ListElement{key:"356";modelData : qsTr("Malta");}
        ListElement{key:"1670";modelData : qsTr("Mariana Is");}
        ListElement{key:"596";modelData : qsTr("Martinique");}
        ListElement{key:"230";modelData : qsTr("Mauritius");}
        ListElement{key:"52";modelData : qsTr("Mexico");}
        ListElement{key:"373";modelData : qsTr("Moldova");}
        ListElement{key:"377";modelData : qsTr("Monaco");}
        ListElement{key:"976";modelData : qsTr("Mongolia");}
        ListElement{key:"1664";modelData : qsTr("Montserrat Is");}
        ListElement{key:"212";modelData : qsTr("Morocco");}
        ListElement{key:"258";modelData : qsTr("Mozambique");}
        ListElement{key:"264";modelData : qsTr("Namibia");}
        ListElement{key:"674";modelData : qsTr("Nauru");}
        ListElement{key:"977";modelData : qsTr("Nepal");}
        ListElement{key:"599";modelData : qsTr("Netheriands Antilles");}
        ListElement{key:"31";modelData : qsTr("Netherlands");}
        ListElement{key:"64";modelData : qsTr("New Zealand");}
        ListElement{key:"505";modelData : qsTr("Nicaragua");}
        ListElement{key:"227";modelData : qsTr("Niger");}
        ListElement{key:"234";modelData : qsTr("Nigeria");}
        ListElement{key:"850";modelData : qsTr("North Korea");}
        ListElement{key:"47";modelData : qsTr("Norway");}
        ListElement{key:"968";modelData : qsTr("Oman");}
        ListElement{key:"92";modelData : qsTr("Pakistan");}
        ListElement{key:"507";modelData : qsTr("Panama");}
        ListElement{key:"675";modelData : qsTr("Papua New Cuinea");}
        ListElement{key:"595";modelData : qsTr("Paraguay");}
        ListElement{key:"51";modelData : qsTr("Peru");}
        ListElement{key:"63";modelData : qsTr("Philippines");}
        ListElement{key:"48";modelData : qsTr("Poland");}
        ListElement{key:"351";modelData : qsTr("Portugal");}
        ListElement{key:"1787";modelData : qsTr("Puerto Rico");}
        ListElement{key:"974";modelData : qsTr("Qatar");}
        ListElement{key:"262";modelData : qsTr("Reunion");}
        ListElement{key:"40";modelData : qsTr("Romania");}
        ListElement{key:"7";modelData : qsTr("Russia");}
        ListElement{key:"1758";modelData : qsTr("Saint Lueia");}
        ListElement{key:"1784";modelData : qsTr("Saint Vincent");}
        ListElement{key:"684";modelData : qsTr("Samoa Eastern");}
        ListElement{key:"685";modelData : qsTr("Samoa Western");}
        ListElement{key:"378";modelData : qsTr("San Marino");}
        ListElement{key:"239";modelData : qsTr("Sao Tome and Principe");}
        ListElement{key:"966";modelData : qsTr("Saudi Arabia");}
        ListElement{key:"221";modelData : qsTr("Senegal");}
        ListElement{key:"248";modelData : qsTr("Seychelles");}
        ListElement{key:"232";modelData : qsTr("Sierra Leone");}
        ListElement{key:"65";modelData : qsTr("Singapore");}
        ListElement{key:"421";modelData : qsTr("Slovakia");}
        ListElement{key:"386";modelData : qsTr("Slovenia");}
        ListElement{key:"677";modelData : qsTr("Solomon Is");}
        ListElement{key:"252";modelData : qsTr("Somali");}
        ListElement{key:"27";modelData : qsTr("South Africa");}
        ListElement{key:"34";modelData : qsTr("Spain");}
        ListElement{key:"94";modelData : qsTr("SriLanka");}
        ListElement{key:"1758";modelData : qsTr("St.Lucia");}
        ListElement{key:"1784";modelData : qsTr("St.Vincent");}
        ListElement{key:"249";modelData : qsTr("Sudan");}
        ListElement{key:"597";modelData : qsTr("Suriname");}
        ListElement{key:"268";modelData : qsTr("Swaziland");}
        ListElement{key:"46";modelData : qsTr("Sweden");}
        ListElement{key:"41";modelData : qsTr("Switzerland");}
        ListElement{key:"963";modelData : qsTr("Syria");}
        ListElement{key:"886";modelData : qsTr("Taiwan");}
        ListElement{key:"992";modelData : qsTr("Tajikstan");}
        ListElement{key:"255";modelData : qsTr("Tanzania");}
        ListElement{key:"66";modelData : qsTr("Thailand");}
        ListElement{key:"228";modelData : qsTr("Togo");}
        ListElement{key:"676";modelData : qsTr("Tonga");}
        ListElement{key:"1809";modelData : qsTr("Trinidad and Tobago");}
        ListElement{key:"216";modelData : qsTr("Tunisia");}
        ListElement{key:"90";modelData : qsTr("Turkey");}
        ListElement{key:"993";modelData : qsTr("Turkmenistan");}
        ListElement{key:"256";modelData : qsTr("Uganda");}
        ListElement{key:"380";modelData : qsTr("Ukraine");}
        ListElement{key:"971";modelData : qsTr("United Arab Emirates");}
        ListElement{key:"44";modelData : qsTr("United Kingdom");}
        ListElement{key:"1";modelData : qsTr("United States of America");}
        ListElement{key:"598";modelData : qsTr("Uruguay");}
        ListElement{key:"233";modelData : qsTr("Uzbekistan");}
        ListElement{key:"58";modelData : qsTr("Venezuela");}
        ListElement{key:"84";modelData : qsTr("Vietnam");}
        ListElement{key:"967";modelData : qsTr("Yemen");}
        ListElement{key:"381";modelData : qsTr("Yugoslavia");}
        ListElement{key:"243";modelData : qsTr("Zaire");}
        ListElement{key:"260";modelData : qsTr("Zambia");}
        ListElement{key:"263";modelData : qsTr("Zimbabwe");}
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 5 * screenScaleFactor

        border.width: Constants.currentTheme ? 0 : 1
        border.color: Constants.currentTheme ? "transparent" : "#262626"
        color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
        radius: 5

        CusRoundedBg {
            radius: 5
            leftTop: true
            rightTop: true
            allRadius: false
            clickedable: false
            color: Constants.currentTheme ? "#E5E5E9" : "#6E6E73"

            width: parent.width - 2 * parent.border.width
            height: 30 * screenScaleFactor - parent.border.width

            anchors.top: parent.top
            anchors.topMargin: parent.border.width
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 8 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter

                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                color: Constants.themeTextColor
                text: qsTr("Login")
            }

            CusButton {
                radius: 5
                rightTop: true
                allRadius: false
                anchors.right: parent.right
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor
                color: isHovered ? "#FF365C" : "transparent"

                Image {
                    anchors.centerIn: parent
                    sourceSize: Qt.size(8, 8)
                    source: parent.isHovered ? "qrc:/UI/photo/closeBtn_d.svg" : "qrc:/UI/photo/closeBtn.svg"
                }

                onClicked: root.close()
            }

            MouseArea {
                anchors.fill: parent
                anchors.rightMargin: 30 * screenScaleFactor
                property point clickPos: "0,0"

                onPressed: clickPos = Qt.point(mouse.x, mouse.y)

                onPositionChanged: {
                    var cursorPos = WizardUI.cursorGlobalPos()
                    root.x = cursorPos.x - clickPos.x
                    root.y = cursorPos.y - clickPos.y
                }
            }
        }

        Item {
            width: parent.width - 2 * parent.border.width
            height: parent.height - 30 * screenScaleFactor

            anchors.top: parent.top
            anchors.topMargin: 30 * screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter

            Column {
                spacing: 0
                anchors.fill: parent

                Row {
                    height: 75 * screenScaleFactor
                    spacing: 6 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/UI/photo/CloudLogo.png"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft

                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color: Constants.themeTextColor
                        text: qsTr("Creality Cloud")
                    }

                    Item {
                        height: parent.height
                        width: 180 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight

                        font.weight: Font.Normal
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color: Constants.themeTextColor
                        text: qsTr("Server")
                    }
                    CustomComboBox {
                        id: serverControl
                        enabled: !showQuickLogin
                        opacity: enabled ? 1.0 : 0.7
                        width: 120 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        currentIndex: cxkernel_cxcloud.serverType
                        model: cxkernel_cxcloud.serverListModel
                        translater: source => qsTr(source)
                        textRole: "model_name"
                        displayText: qsTr(currentText)
                        onActivated: {
                            if (currentIndex === -1) {
                                return
                            }

                            const old_type = cxkernel_cxcloud.toRealServerType(cxkernel_cxcloud.serverType)
                            const new_type = cxkernel_cxcloud.toRealServerType(currentIndex)

                            if (old_type == new_type) {
                                cxkernel_cxcloud.serverType = currentIndex
                                currentIndex = Qt.binding(_ => cxkernel_cxcloud.serverType)
                                return
                            }

                            tipDialog.tipRestartAppliction(
                                onAccepted => {
                                    cxkernel_cxcloud.serverType = currentIndex
                                    kernel_kernel.restartApplication()
                                },
                                onCanceled => {
                                    currentIndex = Qt.binding(_ => cxkernel_cxcloud.serverType)
                                }
                            )
                        }
                    }

                    // ComboBox {
                    //     id: serverControl
                    //     enabled: !showQuickLogin
                    //     opacity: enabled ? 1.0 : 0.7
                    //     width: 120 * screenScaleFactor
                    //     height: 28 * screenScaleFactor
                    //     anchors.verticalCenter: parent.verticalCenter

                    //     currentIndex: cxkernel_cxcloud.serverType
                    //     onCurrentIndexChanged: {
                    //         if (currentIndex != -1) {
                    //             cxkernel_cxcloud.serverType = currentIndex
                    //             if (curLoginType == LoginDlg.LoginType.Qrcode_Type) {
                    //                 cxkernel_cxcloud.accountService.stopAutoRefreshQrcode()
                    //                 cxkernel_cxcloud.accountService.startAutoRefreshQrcode()
                    //             } else {
                    //                 resetBtnSelect()
                    //             }
                    //         }
                    //     }

                    //     model: cxkernel_cxcloud.serverListModel

                    //     textRole: "model_name"
                    //     displayText: qsTr(currentText)

                    //     delegate: ItemDelegate {
                    //         id: server_item
                    //         property string modelUid: model_uid
                    //         property int modelIndex: model_index
                    //         property string modelName: model_name

                    //         width: serverControl.width
                    //         height: serverControl.height

                    //         contentItem: Rectangle {
                    //             anchors.fill: parent
                    //             color: (serverControl.highlightedIndex == index) ? Constants.themeGreenColor : Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"

                    //             Text {
                    //                 padding: 8 * screenScaleFactor
                    //                 anchors.verticalCenter: parent.verticalCenter

                    //                 font.weight: Font.Medium
                    //                 font.family: Constants.mySystemFont.name
                    //                 font.pointSize: Constants.labelFontPointSize_9
                    //                 verticalAlignment: Text.AlignVCenter
                    //                 elide: Text.ElideRight
                    //                 color: Constants.themeTextColor
                    //                 text: qsTr(server_item.modelName)
                    //             }
                    //         }
                    //     }

                    //     background: Rectangle {
                    //         color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                    //         border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                    //         border.width: 1
                    //     }

                    //     indicator: Image {
                    //         width: 7 * screenScaleFactor
                    //         height: 4 * screenScaleFactor
                    //         source: "qrc:/UI/photo/downBtn.svg"

                    //         anchors.right: serverControl.right
                    //         anchors.verticalCenter: parent.verticalCenter
                    //         anchors.rightMargin: 10 * screenScaleFactor
                    //     }

                    //     contentItem: Text {
                    //         padding: 8 * screenScaleFactor
                    //         anchors.verticalCenter: parent.verticalCenter

                    //         font.weight: Font.Medium
                    //         font.family: Constants.mySystemFont.name
                    //         font.pointSize: Constants.labelFontPointSize_9
                    //         verticalAlignment: Text.AlignVCenter
                    //         elide: Text.ElideRight
                    //         color: Constants.themeTextColor
                    //         text: serverControl.displayText
                    //     }

                    //     popup: Popup {
                    //         padding: 1
                    //         y: serverControl.height
                    //         width: serverControl.width
                    //         implicitHeight: serverControl.count * serverControl.height + 2 * padding

                    //         contentItem: ListView {
                    //             clip: true
                    //             implicitHeight: contentHeight
                    //             model: serverControl.delegateModel
                    //         }

                    //         background: Rectangle {
                    //             color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                    //             border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                    //             border.width: 1
                    //         }
                    //     }
                    // }
                }

                Rectangle {
                    width: parent.width
                    height: 1 * screenScaleFactor
                    visible: !Constants.currentTheme
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#3A3A3C"
                }

                Rectangle {
                    width: parent.width
                    height: 1 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: Constants.currentTheme ? "#D6D6DC" : "#5E5E64"
                }

                Item {
                    width: parent.width
                    height: 40 * screenScaleFactor
                }

                Row {
                    height: 24 * screenScaleFactor
                    spacing: 13 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    RadioButton {
                        id: phoneBtn
                        checked: false
                        indicator: null
                        background: null
                        height: parent.height
                        width: 120 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter

                        onCheckedChanged: curLoginType = LoginDialog.LoginType.Phone_Type

                        Component.onCompleted: checked = true

                        contentItem: Text {
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            property color normalTextColor: Constants.currentTheme ? "#999999" : "#76767A"
                            property color hoveredTextColor: Constants.currentTheme ? "#333333" : "#FFFFFF"

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_14
                            color: (parent.hovered || parent.checked) ? hoveredTextColor : normalTextColor
                            text: qsTr("Mobile Login")
                        }
                    }

                    Rectangle {
                        width: 1 * screenScaleFactor
                        height: 16 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        color: Constants.currentTheme ? "#D6D6DC" : "#666666"
                    }

                    RadioButton {
                        id: emailBtn
                        checked: false
                        indicator: null
                        background: null
                        height: parent.height
                        width: 120 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter

                        onCheckedChanged: curLoginType = LoginDialog.LoginType.Email_Type

                        contentItem: Text {
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            property color normalTextColor: Constants.currentTheme ? "#999999" : "#76767A"
                            property color hoveredTextColor: Constants.currentTheme ? "#333333" : "#FFFFFF"

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_14
                            color: (parent.hovered || parent.checked) ? hoveredTextColor : normalTextColor
                            text: qsTr("Email Login")
                        }
                    }

                    Rectangle {
                        width: 1 * screenScaleFactor
                        height: 16 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        color: Constants.currentTheme ? "#D6D6DC" : "#666666"
                    }

                    RadioButton {
                        id: quickBtn
                        checked: false
                        indicator: null
                        background: null
                        height: parent.height
                        width: 120 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter

                        onCheckedChanged: {
                            curLoginType = LoginDialog.LoginType.Qrcode_Type
                        }

                        contentItem: Text {
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            property color normalTextColor: Constants.currentTheme ? "#999999" : "#76767A"
                            property color hoveredTextColor: Constants.currentTheme ? "#333333" : "#FFFFFF"

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_14
                            color: (parent.hovered || parent.checked) ? hoveredTextColor : normalTextColor
                            text: qsTr("Scan QR Code")
                        }
                    }
                }

                Item {
                    width: parent.width
                    height: 30 * screenScaleFactor
                }

                Text {
                    id: idErrorMsg
                    anchors.left: itemSwipeView.left
                    anchors.bottom: itemSwipeView.top
                    anchors.leftMargin: 96 * screenScaleFactor
                    anchors.bottomMargin: 4 * screenScaleFactor

                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Text.AlignVCenter
                    color: "#FF365C"
                }

                SwipeView {
                    id: itemSwipeView
                    width: parent.width
                    height: 250 * screenScaleFactor
                    interactive: false

                    Item {
                        id: phoneItem

                        Column {
                            id: columnPhoneRect
                            spacing: 10 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            Rectangle {
                                id: idAccountPhoneRect
                                width: 408 * screenScaleFactor
                                height: 36 * screenScaleFactor

                                radius: 5
                                border.width: 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                color: "transparent"

                                Row {
                                    height: parent.height
                                    spacing: 1 * screenScaleFactor

                                    ComboBox {
                                        id: idAreaCodeCombox
                                        width: 105 * screenScaleFactor
                                        height: parent.height - 2 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter

                                        onCurrentIndexChanged: idAreaCodeCombox.displayText = "+"+areaCodeList.get(currentIndex).key

                                        Component.onCompleted: idAreaCodeCombox.displayText = "+"+areaCodeList.get(currentIndex).key

                                        background: null
                                        currentIndex: 36
                                        model: areaCodeList

                                        delegate: ItemDelegate {
                                            width: 200 * screenScaleFactor
                                            height: 24 * screenScaleFactor

                                            contentItem: Rectangle {
                                                anchors.fill: parent
                                                color: (idAreaCodeCombox.highlightedIndex == index) ? Constants.themeGreenColor : "transparent"

                                                Text {
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 10 * screenScaleFactor
                                                    anchors.verticalCenter: parent.verticalCenter

                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    verticalAlignment: Text.AlignVCenter
                                                    elide: Text.ElideRight
                                                    color: Constants.themeTextColor
                                                    text: key
                                                }

                                                Text {
                                                    anchors.right: parent.right
                                                    anchors.rightMargin: 10 * screenScaleFactor
                                                    anchors.verticalCenter: parent.verticalCenter

                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    verticalAlignment: Text.AlignVCenter
                                                    elide: Text.ElideRight
                                                    color: Constants.themeTextColor
                                                    text: modelData
                                                }
                                            }
                                        }

                                        indicator: Image {
                                            width: 7 * screenScaleFactor
                                            height: 4 * screenScaleFactor
                                            source: "qrc:/UI/photo/downBtn.svg"

                                            anchors.right: idAreaCodeCombox.right
                                            anchors.rightMargin: 10 * screenScaleFactor
                                            anchors.verticalCenter: parent.verticalCenter
                                        }

                                        contentItem: Item {
                                            Image {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 14 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter
                                                source: "qrc:/UI/photo/phoneImg.png"
                                                sourceSize: Qt.size(9, 16)
                                            }

                                            Text {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 33 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                verticalAlignment: Text.AlignVCenter
                                                elide: Text.ElideRight
                                                color: Constants.themeTextColor
                                                text: idAreaCodeCombox.displayText
                                            }
                                        }

                                        popup: Popup {
                                            padding: 0
                                            y: idAreaCodeCombox.height
                                            width: 200 * screenScaleFactor
                                            height: 200 * screenScaleFactor

                                            contentItem: ListView {
                                                clip: true
                                                id: listViewAreaCode
                                                implicitHeight: contentHeight
                                                model: idAreaCodeCombox.delegateModel
                                                ScrollBar.vertical: ScrollBar {
                                                    snapMode: ScrollBar.SnapOnRelease
                                                    visible: listViewAreaCode.contentHeight > listViewAreaCode.height
                                                    contentItem: Rectangle {
                                                        opacity: 0.8
                                                        radius: width / 2
                                                        implicitWidth: 6 * screenScaleFactor
                                                        implicitHeight: 55 * screenScaleFactor
                                                        color: Constants.currentTheme ? "#D4D4DA" : "#7E7E84"
                                                    }
                                                }
                                            }

                                            background: Rectangle {
                                                color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                                                border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                                border.width: 1
                                            }
                                        }
                                    }

                                    Rectangle {
                                        width: 1
                                        height: parent.height - 2 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                    }

                                    TextField {
                                        id: idAccountPhoneEdit
                                        width: 300 * screenScaleFactor
                                        height: parent.height - 2 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter

                                        topPadding: 0
                                        bottomPadding: 0
                                        leftPadding: 20 * screenScaleFactor
                                        rightPadding: 20 * screenScaleFactor

                                        background: null
                                        selectByMouse: true
                                        selectedTextColor: color
                                        validator: RegExpValidator{regExp: new RegExp(phoneRegExp)}
                                        placeholderText: qsTr("Please enter your phone number")
                                        selectionColor: Constants.currentTheme ? "#98DAFF" : "#1E9BE2"
                                        placeholderTextColor: Constants.currentTheme ? "#999999" : "#CBCBCC"
                                        verticalAlignment: TextInput.AlignVCenter
                                        horizontalAlignment: TextInput.AlignLeft
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.themeTextColor

                                        onTextChanged: idErrorMsg.text = ""
                                    }
                                }
                            }

                            Row {
                                spacing: 8 * screenScaleFactor

                                Rectangle {
                                    id: idPassWordPhoneRect
                                    property bool showPassword: false
                                    property string normalImg: showPassword ? Constants.showPWNormalImg : Constants.hidePWNormalImg
                                    property string hoveredImg: showPassword ? Constants.showPWHoveredImg : Constants.hidePWHoveredImg

                                    width: (showQuickLogin ? 290 : 408) * screenScaleFactor
                                    height: 36 * screenScaleFactor

                                    radius: 5
                                    border.width: 1
                                    border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                    color: "transparent"

                                    Image {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 14 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: "qrc:/UI/photo/passwordImg.png"
                                        sourceSize: Qt.size(13, 16)
                                    }

                                    TextField {
                                        id: idPasswordPhoneEdit
                                        width: 300 * screenScaleFactor
                                        height: parent.height - 2 * screenScaleFactor

                                        anchors.left: parent.left
                                        anchors.leftMargin: 36 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter

                                        padding: 0
                                        background: null
                                        selectByMouse: true
                                        selectedTextColor: color
                                        validator: RegExpValidator{regExp: new RegExp(passwordExp)}
                                        placeholderText: showQuickLogin ? qsTr("Please enter verification code") : qsTr("Please enter password")
                                        selectionColor: Constants.currentTheme ? "#98DAFF" : "#1E9BE2"
                                        placeholderTextColor: Constants.currentTheme ? "#999999" : "#CBCBCC"
                                        echoMode: idPassWordPhoneRect.showPassword ? TextInput.Normal : TextInput.Password
                                        verticalAlignment: TextInput.AlignVCenter
                                        horizontalAlignment: TextInput.AlignLeft
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.themeTextColor

                                        onTextChanged: idErrorMsg.text = ""
                                        Keys.onEnterPressed: if(phoneLoginBtn.enabled) phoneLoginClicked()
                                        Keys.onReturnPressed: if(phoneLoginBtn.enabled) phoneLoginClicked()
                                    }

                                    Image {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 14 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: visualPhone.containsMouse ? idPassWordPhoneRect.hoveredImg : idPassWordPhoneRect.normalImg
                                        //sourceSize: Qt.size(14, 10)

                                        MouseArea {
                                            id: visualPhone
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: idPassWordPhoneRect.showPassword = !idPassWordPhoneRect.showPassword
                                        }
                                    }
                                }

                                Rectangle {
                                    id: getCodeButton
                                    width: 110 * screenScaleFactor
                                    height: 36 * screenScaleFactor

                                    visible: showQuickLogin
                                    opacity: enabled ? 1.0 : 0.7
                                    enabled: idAccountPhoneEdit.text.match(phoneRegExp)

                                    radius: 5
                                    border.width: 1
                                    border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                                    color: parent.hovered ? "#1E9BE2" : (Constants.currentTheme ? "#FFFFFF" : "#636367")

                                    Text {
                                        id: getCodeText
                                        anchors.centerIn: parent
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.themeTextColor
                                        text: qsTr("Get Code")
                                    }

                                    MouseArea {
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                                        onClicked: {
                                            var area_code = idAreaCodeCombox.displayText.substring(1)
                                            var phone_number = idAccountPhoneEdit.text
                                            timeCnt = 60
                                            countDown.start()
                                            cxkernel_cxcloud.accountService.requestVerifyCode(phone_number, area_code)
                                        }
                                    }
                                }
                            }
                        }

                        Button {
                            id: phoneLoginBtn
                            width: 408 * screenScaleFactor
                            height: 48 * screenScaleFactor

                            opacity: enabled ? 1.0 : 0.7
                            enabled: idAccountPhoneEdit.text.match(phoneRegExp) && idPasswordPhoneEdit.text.match(passwordExp)

                            anchors.top: columnPhoneRect.bottom
                            anchors.topMargin: 42 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            background: Rectangle {
                                radius: 5
                                border.width: 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                                color: parent.hovered ? "#1E9BE2" : (Constants.currentTheme ? "#FFFFFF" : "#636367")
                            }

                            contentItem: Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_12
                                color: Constants.themeTextColor
                                text: qsTr("Log In")
                            }

                            onClicked: phoneLoginClicked()
                        }

                        Text {
                            anchors.left: phoneLoginBtn.left
                            anchors.bottom: phoneLoginBtn.top
                            anchors.bottomMargin: 8 * screenScaleFactor
                            visible: !serverControl.currentIndex

                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            color: Constants.themeTextColor
                            text: "<font color='#17CC5F'>" + (showQuickLogin ? qsTr("Password Login") : qsTr("Instant Login")) +  "</font>"

                            MouseArea {
                                hoverEnabled: true
                                anchors.fill: parent
                                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                                onClicked: {
                                    idPasswordPhoneEdit.clear()
                                    showQuickLogin = !showQuickLogin
                                }
                            }
                        }

                        Text {
                            anchors.right: phoneLoginBtn.right
                            anchors.bottom: phoneLoginBtn.top
                            anchors.bottomMargin: 8 * screenScaleFactor
                            visible: !showQuickLogin

                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            color: Constants.themeTextColor
                            text: "<font color='#17CC5F'>" + qsTr("Forgot Password?") +  "</font>"

                            MouseArea {
                                hoverEnabled: true
                                anchors.fill: parent
                                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    let type = "1"
                                    Qt.openUrlExternally("%1/?resetpassword=%2&channel=creality-print"
                                        .arg(cxkernel_cxcloud.webUrl).arg(type))
                                }
                            }
                        }

                        Row {
                            anchors.top: phoneLoginBtn.bottom
                            anchors.left: phoneLoginBtn.left
                            anchors.topMargin: 10 * screenScaleFactor
                            spacing: 8 * screenScaleFactor
                            visible: !showQuickLogin

                            Rectangle {
                                id: autoLoginPhoneCheckBox
                                width: 18 * screenScaleFactor
                                height: 18 * screenScaleFactor

                                radius: 3
                                border.width: globalAutoLogin ? 0 : 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                color: globalAutoLogin ? Constants.themeGreenColor : "transparent"

                                Image {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.leftMargin: 5 * screenScaleFactor
                                    anchors.topMargin: 6 * screenScaleFactor

                                    source:  "qrc:/UI/images/check2.png"  // Constants.checkBtnImg
                                    visible: globalAutoLogin
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    onClicked: globalAutoLogin = !globalAutoLogin
                                }
                            }

                            Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignLeft

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: qsTr("Auto login")
                            }
                        }

                        Row {
                            anchors.top: phoneLoginBtn.bottom
                            anchors.right: phoneLoginBtn.right
                            anchors.topMargin: 10 * screenScaleFactor
                            spacing: 8 * screenScaleFactor

                            Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: qsTr("No account yet") + "?"
                            }

                            Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: "<font color='#17CC5F'>" + qsTr("Create Account") +  "</font>"

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    onClicked: {
                                        let type = "1"
                                        Qt.openUrlExternally("%1/?signup=%2&channel=creality-print"
                                            .arg(cxkernel_cxcloud.webUrl).arg(type))
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        id: emailItem

                        Column {
                            id: columnEmailRect
                            spacing: 10 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            Rectangle {
                                id: idAccountEmailRect
                                width: 408 * screenScaleFactor
                                height: 36 * screenScaleFactor

                                radius: 5
                                border.width: 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                color: "transparent"

                                Image {
                                    width: 15 * screenScaleFactor
                                    height: 16 * screenScaleFactor

                                    anchors.left: parent.left
                                    anchors.leftMargin: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: "qrc:/UI/photo/userImg_normal.svg"
                                }

                                TextField {
                                    id: idAccountEmailEdit
                                    width: 300 * screenScaleFactor
                                    height: parent.height - 2 * screenScaleFactor

                                    anchors.left: parent.left
                                    anchors.leftMargin: 36 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    padding: 0
                                    background: null
                                    selectByMouse: true
                                    selectedTextColor: color
                                    validator: RegExpValidator{regExp: new RegExp(emailRegExp)}
                                    placeholderText: qsTr("Please enter your email address")
                                    selectionColor: Constants.currentTheme ? "#98DAFF" : "#1E9BE2"
                                    placeholderTextColor: Constants.currentTheme ? "#999999" : "#CBCBCC"
                                    verticalAlignment: TextInput.AlignVCenter
                                    horizontalAlignment: TextInput.AlignLeft
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: Constants.themeTextColor

                                    onTextChanged: idErrorMsg.text = ""
                                }
                            }

                            Rectangle {
                                id: idPassWordEmailRect
                                property bool showPassword: false
                                property string normalImg: showPassword ? Constants.showPWNormalImg : Constants.hidePWNormalImg
                                property string hoveredImg: showPassword ? Constants.showPWHoveredImg : Constants.hidePWHoveredImg

                                width: 408 * screenScaleFactor
                                height: 36 * screenScaleFactor

                                radius: 5
                                border.width: 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                color: "transparent"

                                Image {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: "qrc:/UI/photo/passwordImg.png"
                                    sourceSize: Qt.size(13, 16)
                                }

                                TextField {
                                    id: idPasswordEmailEdit
                                    width: 300 * screenScaleFactor
                                    height: parent.height - 2 * screenScaleFactor

                                    anchors.left: parent.left
                                    anchors.leftMargin: 36 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    padding: 0
                                    background: null
                                    selectByMouse: true
                                    selectedTextColor: color
                                    validator: RegExpValidator{regExp: new RegExp(passwordExp)}
                                    placeholderText: qsTr("Please enter password")
                                    selectionColor: Constants.currentTheme ? "#98DAFF" : "#1E9BE2"
                                    placeholderTextColor: Constants.currentTheme ? "#999999" : "#CBCBCC"
                                    echoMode: idPassWordEmailRect.showPassword ? TextInput.Normal : TextInput.Password
                                    verticalAlignment: TextInput.AlignVCenter
                                    horizontalAlignment: TextInput.AlignLeft
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: Constants.themeTextColor

                                    onTextChanged: idErrorMsg.text = ""
                                    Keys.onEnterPressed: if(emailLoginBtn.enabled) emailLoginClicked()
                                    Keys.onReturnPressed: if(emailLoginBtn.enabled) emailLoginClicked()
                                }

                                Image {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: visualEmail.containsMouse ? idPassWordEmailRect.hoveredImg : idPassWordEmailRect.normalImg
                                    //sourceSize: Qt.size(14, 10)

                                    MouseArea {
                                        id: visualEmail
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: idPassWordEmailRect.showPassword = !idPassWordEmailRect.showPassword
                                    }
                                }
                            }
                        }

                        Button {
                            id: emailLoginBtn
                            width: 408 * screenScaleFactor
                            height: 48 * screenScaleFactor
                            property bool emailValid: idAccountEmailEdit.text.match(emailRegExp) && idPasswordEmailEdit.text.match(passwordExp)

                            opacity: enabled ? 1.0 : 0.7
                            enabled: emailValid

                            anchors.top: columnEmailRect.bottom
                            anchors.topMargin: 42 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            background: Rectangle {
                                radius: 5
                                border.width: 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                                color: parent.hovered ? "#1E9BE2" : (Constants.currentTheme ? "#FFFFFF" : "#636367")
                            }

                            contentItem: Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_12
                                color: Constants.themeTextColor
                                text: qsTr("Log In")
                            }

                            onClicked: emailLoginClicked()
                        }

                        Text {
                            anchors.right: emailLoginBtn.right
                            anchors.bottom: emailLoginBtn.top
                            anchors.bottomMargin: 8 * screenScaleFactor

                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            color: Constants.themeTextColor
                            text: "<font color='#17CC5F'>" + qsTr("Forgot Password?") +  "</font>"

                            MouseArea {
                                hoverEnabled: true
                                anchors.fill: parent
                                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    let type = "0"
                                    Qt.openUrlExternally("%1/?resetpassword=%2&channel=creality-print"
                                        .arg(cxkernel_cxcloud.webUrl).arg(type))
                                }
                            }
                        }

                        Row {
                            anchors.top: emailLoginBtn.bottom
                            anchors.left: emailLoginBtn.left
                            anchors.topMargin: 10 * screenScaleFactor
                            spacing: 8 * screenScaleFactor

                            Rectangle {
                                id: autoLoginEmailCheckBox
                                width: 18 * screenScaleFactor
                                height: 18 * screenScaleFactor

                                radius: 3
                                border.width: globalAutoLogin ? 0 : 1
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#7A7A7F"
                                color: globalAutoLogin ? Constants.themeGreenColor : "transparent"

                                Image {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.leftMargin: 5 * screenScaleFactor
                                    anchors.topMargin: 6 * screenScaleFactor

                                    source:  "qrc:/UI/images/check2.png"//Constants.checkBtnImg
                                    visible: globalAutoLogin
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                                    onClicked: globalAutoLogin = !globalAutoLogin
                                }
                            }

                            Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignLeft

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: qsTr("Auto login")
                            }
                        }

                        Row {
                            anchors.top: emailLoginBtn.bottom
                            anchors.right: emailLoginBtn.right
                            anchors.topMargin: 10 * screenScaleFactor
                            spacing: 8 * screenScaleFactor

                            Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: qsTr("No account yet") + "?"
                            }

                            Text {
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignRight

                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: "<font color='#17CC5F'>" + qsTr("Create Account") +  "</font>"

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    onClicked: {
                                        let type = "0"
                                        Qt.openUrlExternally("%1/?signup=%2&channel=creality-print"
                                            .arg(cxkernel_cxcloud.webUrl).arg(type))
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        id: qrcodeItem

                        Row {
                            spacing: 12 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            Rectangle {
                                width: 200 * screenScaleFactor
                                height: 200 * screenScaleFactor

                                Image {
                                    cache: false
                                    id: idQrCodeImage
                                    anchors.centerIn: parent
                                    width: parent.width - 20 * screenScaleFactor
                                    height: parent.height - 20 * screenScaleFactor
									source: "image://login_qrcode_image_provider"
                                }
                            }

                            Rectangle {
                                width: 216 * screenScaleFactor
                                height: 200 * screenScaleFactor
                                color: "transparent"

                                Text {
                                    width: parent.width
                                    lineHeight: 24 * screenScaleFactor
                                    visible: !serverControl.currentIndex

                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.topMargin: 10 * screenScaleFactor
                                    verticalAlignment: Text.AlignVCenter

                                    wrapMode: Text.Wrap
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    lineHeightMode: Text.FixedHeight
                                    color: Constants.themeTextColor
                                    text: qsTr("Me section in app > Scan icon on top") + "<br>" +
                                          qsTr("You are browsing") + " " + "<font color='#17CC5F'>" + qsTr("crealitycloud.cn") + ", " + "</font>" +
                                          qsTr("Please use the") + " " + "<font color='#17CC5F'>" + qsTr("Creality Cloud App (CN version)") + "</font>" + " " +
                                          qsTr("to scan the QR code")
                                }

                                Text {
                                    width: parent.width
                                    lineHeight: 24 * screenScaleFactor
                                    visible: serverControl.currentIndex

                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.topMargin: 10 * screenScaleFactor
                                    verticalAlignment: Text.AlignVCenter

                                    wrapMode: Text.Wrap
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    lineHeightMode: Text.FixedHeight
                                    color: Constants.themeTextColor
                                    text: qsTr("Me section in app > Scan icon on top") + "<br>" +
                                          qsTr("You are browsing") + " " + "<font color='#17CC5F'>" + qsTr("crealitycloud.com") + ", " + "</font>" +
                                          qsTr("Please use the") + " " + "<font color='#17CC5F'>" + qsTr("Creality Cloud App") + "</font>" + " " +
                                          qsTr("to scan the QR code")
                                }

                                Row {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 10 * screenScaleFactor
                                    spacing: 8 * screenScaleFactor

                                    Text {
                                        verticalAlignment: Text.AlignVCenter

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.themeTextColor
                                        text: qsTr("No account yet") + "?"
                                    }

                                    Text {
                                        verticalAlignment: Text.AlignVCenter

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.themeTextColor
                                        text: "<font color='#17CC5F'>" + qsTr("Create Account") +  "</font>"

                                        MouseArea {
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                                            onClicked: {
                                                let type = "1"
                                                Qt.openUrlExternally("%1/?signup=%2&channel=creality-print"
                                                    .arg(cxkernel_cxcloud.webUrl).arg(type))
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        layer.enabled: true

        layer.effect: DropShadow {
            radius: 8
            spread: 0.2
            samples: 17
            color: Constants.currentTheme ? "#BBBBBB" : "#333333"
        }
    }
}
