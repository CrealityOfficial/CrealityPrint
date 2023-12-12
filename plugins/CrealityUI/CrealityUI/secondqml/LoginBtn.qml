import QtQuick 2.13
import QtQuick.Controls 2.5

import "../qml"
import "../secondqml"

Button {
    id: root
    width: 40 * screenScaleFactor
    height: 40 * screenScaleFactor

    property bool loginStatus: cxkernel_cxcloud.accountService.logined
    property string btnImgUrl: root.hovered && !Constants.currentTheme ? "qrc:/UI/photo/userImg_hover.svg" : "qrc:/UI/photo/userImg_normal.svg"

    function showLoginPanel() {
        idLoginDialog.show()
        idLoginDialog.raise()
    }

    function setCurrentServer(serverIndex) {
        idLoginDialog.setServerType(serverIndex)
    }

    onClicked: {
        if(loginStatus)
        {
            idUserInfoDlg.setUserInfoDlgShow("mySlice")
        }
        else {
            idLoginDialog.show()
        }
    }

    onLoginStatusChanged: {
        if(loginStatus && idLoginDialog.visible) idLoginDialog.visible = false
    }

    background: Rectangle {
        color: parent.hovered ? (Constants.currentTheme ? "#FFE0E1E5" : "#80000000") : "transparent"
        border.color: Constants.downloadbtn_finished_border_default_color
        border.width: parent.hovered ? 0 : 2 * screenScaleFactor
        radius: parent.width / 2

        BaseCircularImage {
            //id: sourceImage
            anchors.centerIn: parent
            width: (loginStatus ? 40 : 14) * screenScaleFactor
            height: (loginStatus ? 40 : 16) * screenScaleFactor
            img_src: loginStatus ? cxkernel_cxcloud.accountService.avatar : btnImgUrl
        }
    }

    LoginDlg {
        visible: false
        id: idLoginDialog
        objectName: "LoginDlg"
    }

    UserInfoDlg {
        visible: false
        id: idUserInfoDlg
      //  context: dock_context
        objectName: "UserInfoDlg"

        onQuitClicked: {
            cxkernel_cxcloud.accountService.logout()

            idLoginDialog.visible = true
            idUserInfoDlg.visible = false
        }
    }
}
