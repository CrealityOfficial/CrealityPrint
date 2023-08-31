import QtQuick.Controls 2.12
import QtQuick 2.0
import "../qml"
BasicGroupBox {
    id: control
    title: qsTr("Select Profile")
    implicitWidth: 260
    implicitHeight:330
    property alias model: idBasicView.myModel
    property alias currentItemindex: idBasicView.currentItem
    property var supportBtnEnable: false

    signal sigeditProfile()
    signal sigaddeditProfile()
    signal sigeditDelete()
    signal sigprofileModelChanged()
    signal siggotoSupportTab()
    visible: width > 10 ? true:false

    Grid
    {
        id: idGrid1
        anchors.fill: parent
        rows: 2
        spacing: 10
        PrinterModelListView/*参数配置*/
        {
            id: idBasicView
            width: parent.width
            height: parent.height-66
           // myModel: objModel
            border.width: 1
            border.color: "black"
			onSigDoubleClicked:
			{
				sigeditProfile()
			}
            onSigModelChanged:
            {
                sigprofileModelChanged()
            }
        }
        Grid
        {
            id:idGrid2
            width: idBasicView.width
            height:64
            rows: 2
            spacing: 10

            Row{
                spacing:10
                BasicButton/*BasicComButton*/
                {
                    id: idNewCopy
                    text:qsTr("New Profile")
                    width: (idBasicView.width - 10) / 2
                    height: 28
                    btnRadius:3
                    visible: control.width > 10 ?true : false
                    onSigButtonClicked:
                    {
                        sigaddeditProfile()
                    }

                }

                BasicButton/*BasicComButton*/
                {
                    id: idsupport
                    text:qsTr("Support")
                    width: (idBasicView.width - 10) / 2
                    height: 28
                    btnRadius:3
                    visible: false;//control.width > 10 ?true : false
                    enabled:supportBtnEnable
                    onSigButtonClicked:
                    {
                        siggotoSupportTab()
                    }
                }
            }


            Row
            {
                spacing : 10
                BasicDialogButton{
                    id: idEditProfile
                    width: 70
                    height: 28
                    btnRadius: 3
                    btnBorderW: 1
                    borderColor: Constants.rectBorderColor//"#2B2B2B"
                    defaultBtnBgColor: Constants.buttonColor//"#454545"
                    hoveredBtnBgColor: Constants.dialogBtnHoveredColor//"#5D5D5D"
                    pressedIconSource: btnHovred ? "qrc:/UI/photo/editProfile_h.png" : Constants.editProfileImg
                    strTooptip: qsTr("Edit Profile")
                    onSigButtonClicked:
                    {
                        sigeditProfile()
                    }
                    visible: control.width > 10 ?true : false
               }

               BasicDialogButton{
                    id: idUploadProfile
                    width: 70
                    height: 28
                    btnRadius: 3
                    btnBorderW: 1
                    borderColor: Constants.rectBorderColor//"#2B2B2B"
                    defaultBtnBgColor: Constants.buttonColor//"#454545"
                    hoveredBtnBgColor: Constants.dialogBtnHoveredColor//"#5D5D5D"
                    pressedIconSource: btnHovred ? "qrc:/UI/photo/upload_model_img_h.png" : Constants.uploadModelImg
                    strTooptip: qsTr("Upload Profile")
                    onSigButtonClicked:
                    {
                        //sigeditProfile()
                    }
                    visible: control.width > 10 ?true : false
                    enabled: false
               }

               BasicDialogButton{
                    id: idDelete
                    width: 70
                    height: 28
                    btnRadius: 3
                    btnBorderW: 1
                    borderColor: Constants.rectBorderColor//"#2B2B2B"
                    defaultBtnBgColor: Constants.buttonColor//"#454545"
                    hoveredBtnBgColor: Constants.dialogBtnHoveredColor//"#5D5D5D"
                    enabled:idBasicView.currentItem > 2
                    pressedIconSource: btnHovred ? "qrc:/UI/photo/delete_model_img_h.png" : Constants.deleteModelImg
                    strTooptip: qsTr("Delete Profile")
                    onSigButtonClicked:
                    {
                        sigeditDelete()
                    }
                    visible: control.width > 10 ?true : false
               }



//                 BasicButton/*BasicComButton*/
//                 {
//                     id : idAdd
//                     text:qsTr("Edit Profile")
//                     width: (idBasicView.width -10)/2
//                     height: 28
//                     btnRadius:3
// //                    enabledIconSource: "qrc:/UI/photo/editBtn.png"
// //                    pressedIconSource: "qrc:/UI/photo/editBtn.png"
//                     visible: control.width > 10 ?true : false
//                     //strTooptip : qsTr(" Edit Profile ")
//                     onSigButtonClicked:
//                     {
//                         sigeditProfile()
//                     }
//                 }

//                 BasicButton/*BasicComButton*/
//                 {
//                     id:idDelete
//                     text:qsTr("Delete Profile")
//                     width: (idBasicView.width -10)/2
//                     height: 28
//                     btnRadius:3
// //                    enabledIconSource: "qrc:/UI/photo/deleteBtn.png"
// //                    pressedIconSource: "qrc:/UI/photo/deleteBtn.png"
//                     enabled:idBasicView.currentItem > 2
//                     visible: control.width > 10 ?true : false
//                     //strTooptip : qsTr(" Delete Profile ")
//                     onSigButtonClicked:
//                     {
//                         sigeditDelete()
//                     }
//                 }

            }
        }
    }

    Connections {
        target: idAdd
        //onClicked: print("clicked")
    }
}
